#ifndef BLE_DEVICE_SETTING_HPP
#define BLE_DEVICE_SETTING_HPP

#include <wx/wx.h>
#include <wx/aboutdlg.h>  // สำหรับ wxAboutDialogInfo
#include <wx/button.h>
#include <wx/image.h> // จำเป็นสำหรับ wxInitAllImageHandlers()
#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <mutex>


class BLEDeviceSettingsDialog : public wxDialog {
public:
	BLEDeviceSettingsDialog(wxWindow* parent);
    ~BLEDeviceSettingsDialog() = default;

private:
    void fetchBLEDevices();  // ฟังก์ชันที่ใช้ดึงข้อมูล BLE devices
    void onTimer(wxTimerEvent& event);  // ฟังก์ชันที่เรียกใช้งานทุกๆ 1 วินาที
	void initBLEWatcher();  // ฟังก์ชันที่ใช้เริ่มต้นการสแกน BLE devices

    std::mutex m_mutex;
    std::vector<std::tuple<std::string, std::string, std::string, int>> m_devices;
    winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher m_bleWatcher;
    std::unordered_map<std::string, std::tuple<std::string, std::string, std::string, int>> uniqueDevices;

    wxListCtrl* m_deviceList;  // wxListCtrl สำหรับแสดง BLE devices
    wxTimer m_timer;           // ตัวจับเวลาสำหรับเรียกฟังก์ชันทุกๆ 1 วินาที
    wxDECLARE_EVENT_TABLE();

};
#endif // BLE_DEVICE_SETTINGS_HPP