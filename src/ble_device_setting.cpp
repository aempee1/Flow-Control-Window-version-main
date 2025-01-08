#include "ble_device_setting.hpp"
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <mutex>
#include <future>

BLEDeviceSettingsDialog::BLEDeviceSettingsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "BLE Device Settings", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    m_timer(this) // Create wxTimer
{
    // Create panel for layout management
    wxPanel* panel = new wxPanel(this);

    // Create ListCtrl to display Bluetooth devices
    m_deviceList = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(500, 300),
        wxLC_REPORT | wxLC_SINGLE_SEL);

    // Define columns in ListCtrl
    m_deviceList->InsertColumn(0, "Device Name");
    m_deviceList->InsertColumn(1, "Address");
    m_deviceList->InsertColumn(2, "UUID");
    m_deviceList->InsertColumn(3, "RSSI");

    // Initialize BLE watcher
    initBLEWatcher();

    // Start timer to trigger every second (1000ms)
    m_timer.Start(2000);

    // Define layout
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_deviceList, 1, wxEXPAND | wxALL, 10);
    panel->SetSizerAndFit(sizer);

    // Set dialog size
    this->SetSize(600, 400);
}

void BLEDeviceSettingsDialog::fetchBLEDevices()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // Clear old data from ListCtrl
    m_deviceList->DeleteAllItems();

    // Set column widths
    m_deviceList->SetColumnWidth(0, 200); // "Device Name"
    m_deviceList->SetColumnWidth(1, 200); // "Address"
    m_deviceList->SetColumnWidth(2, 100); // "UUID"
    m_deviceList->SetColumnWidth(3, 100); // "RSSI"

    for (const auto& newDevice : m_devices)
    {
        if (!std::get<0>(newDevice).empty()) // Only display devices with names
        {
            long itemIndex = m_deviceList->InsertItem(m_deviceList->GetItemCount(), std::get<0>(newDevice));
            m_deviceList->SetItem(itemIndex, 1, std::get<1>(newDevice));
            m_deviceList->SetItem(itemIndex, 2, std::get<2>(newDevice));
            m_deviceList->SetItem(itemIndex, 3, std::to_string(std::get<3>(newDevice)));
        }
    }

    m_devices.clear(); // Clear devices after updating
}

void BLEDeviceSettingsDialog::onTimer(wxTimerEvent& event)
{
    fetchBLEDevices(); // Fetch new BLE devices on timer
}

void BLEDeviceSettingsDialog::initBLEWatcher()
{
    try
    {
        m_bleWatcher = winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher();
        m_bleWatcher.ScanningMode(winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEScanningMode::Active);


        m_bleWatcher.Received([this](auto&&, auto&& args) {
            auto advertisement = args.Advertisement();
            auto localName = advertisement.LocalName();
            auto address = args.BluetoothAddress();
            int rssi = args.RawSignalStrengthInDBm();

            if (localName.empty() || localName == L"Unknown Device") {
                return; // Skip devices with no name
            }

            std::string addressStr = std::string(wxString::Format("%012llX", address).ToStdString());

            std::lock_guard<std::mutex> lock(m_mutex);

            // Check for duplicate devices
            auto it = std::find_if(m_devices.begin(), m_devices.end(), [&addressStr](const auto& device) {
                return std::get<1>(device) == addressStr;
                });

            if (it == m_devices.end()) // Add new device if not found
            {
                m_devices.emplace_back(
                    winrt::to_string(localName), // Convert to std::string
                    addressStr,
                    "N/A", // UUID placeholder
                    rssi);
            }
            });

        m_bleWatcher.Start();
    }
    catch (const std::exception& e)
    {
        wxLogError("Failed to initialize BLE watcher: %s", e.what());
    }
}

wxBEGIN_EVENT_TABLE(BLEDeviceSettingsDialog, wxDialog)
EVT_TIMER(wxID_ANY, BLEDeviceSettingsDialog::onTimer) // Connect timer event
wxEND_EVENT_TABLE()
