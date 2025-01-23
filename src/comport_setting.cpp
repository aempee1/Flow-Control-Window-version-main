#include "comport_setting.hpp"
#ifdef _WIN32
#include <windows.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Foundation.h>
#include <functional>
#endif
#ifdef __APPLE__
#include <dirent.h>
#include <regex>
#endif
#ifdef _WIN32
using namespace winrt;
using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Devices::Bluetooth::Advertisement;
using namespace winrt::Windows::Foundation;
#endif 
using namespace std;
using namespace boost::asio;

serial_port ComportSettingsDialog::InitialSerial(io_service& io, const string& port_name)
{
    serial_port serial(io, port_name);
    serial.set_option(serial_port_base::baud_rate(9600));
    serial.set_option(serial_port_base::character_size(8));
    serial.set_option(serial_port_base::parity(serial_port_base::parity::none));
    serial.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));
    serial.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));

    if (!serial.is_open())
    {
        //cerr << "Failed to open serial port!" << endl;
        throw runtime_error("Failed to open serial port");
    }

    // cout << "Successfully initialized Serial on port: " << port_name << endl;
    return serial;
}
serial_port ComportSettingsDialog::InitialVirtualSerial(io_service& io_v, const string& port_name)
{
    serial_port serial(io_v, port_name);
    serial.set_option(serial_port_base::baud_rate(115200));
    serial.set_option(serial_port_base::character_size(8));
    serial.set_option(serial_port_base::parity(serial_port_base::parity::none));
    serial.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));
    serial.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));
    if (!serial.is_open())
    {
        //cerr << "Failed to open serial port!" << endl;
        throw runtime_error("Failed to open serial port");
    }

    // cout << "Successfully initialized Serial on port: " << port_name << endl;
    return serial;
}
//----------------------------------------------------------------------------------------------------------------------------------
modbus_t* ComportSettingsDialog::InitialModbus(const char* modbus_port) {
    modbus_t* ctx = initialize_modbus(modbus_port);
    return ctx;
}
//----------------------------------------------------------------------------------------------------------------------------------
vector<string> ComportSettingsDialog::FetchAvailablePorts() {
    vector<string> ports;

#ifdef _WIN32
    char portName[10];
    for (int i = 1; i <= 256; ++i) {
        snprintf(portName, sizeof(portName), "COM%d", i);
        HANDLE hPort = CreateFileA(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPort != INVALID_HANDLE_VALUE) {
            ports.push_back(portName);
            CloseHandle(hPort);
        }
    }
#endif

#ifdef __APPLE__
    DIR* dir = opendir("/dev");
    if (dir) {
        struct dirent* entry;
        regex serialRegex("^tty\\..*"); // พอร์ต serial บน macOS มักจะขึ้นต้นด้วย "cu."
        while ((entry = readdir(dir)) != NULL) {
            if (regex_match(entry->d_name, serialRegex)) {
                ports.push_back("/dev/" + string(entry->d_name));
            }
        }
        closedir(dir);
    }
#endif

    return ports;
}
void ComportSettingsDialog::SaveSelectedPorts() {
    ofstream outFile("properties.txt");
	outFile << selectedBleAgentPort << endl;
    outFile << selectedModbusPort << endl;
    outFile << selectedPowerSupplyPort << endl;
    outFile.close();
}
void ComportSettingsDialog::LoadSelectedPorts() {
    ifstream inFile("properties.txt");
    if (inFile) {
		getline(inFile, selectedBleAgentPort);
        getline(inFile, selectedModbusPort);
        getline(inFile, selectedPowerSupplyPort);
        inFile.close();
    }
}
//----------------------------------------------------------------------------------------------------------------------------------
ComportSettingsDialog::ComportSettingsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Comport Settings", wxDefaultPosition, wxSize(500, 220), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    LoadSelectedPorts();
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(4, 3, 10, 10);
    auto availablePorts = FetchAvailablePorts();
    wxArrayString baudRates;
    baudRates.Add("9600");
    baudRates.Add("19200");
    baudRates.Add("38400");
    baudRates.Add("57600");
    baudRates.Add("115200");
    // Bluetooth Label และ Choice
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "BLE Virtual port:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL);
    wxChoice* BLEChoice = new wxChoice(this, wxID_ANY);
    for (const auto& port : availablePorts) {
        BLEChoice->Append(port);
    }
    BLEChoice->SetStringSelection(selectedBleAgentPort);
    gridSizer->Add(BLEChoice, 1, wxALIGN_CENTER_HORIZONTAL);
    wxChoice* BLEBaudChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, baudRates);
    BLEBaudChoice->SetStringSelection(selectedBleAgentBaudRate);
    gridSizer->Add(BLEBaudChoice, 1, wxALIGN_CENTER_HORIZONTAL);
    // Modbus Label และ Choice
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Modbus:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL);
    wxChoice* modbusChoice = new wxChoice(this, wxID_ANY);
    for (const auto& port : availablePorts) {
        modbusChoice->Append(port);
    }
    modbusChoice->SetStringSelection(selectedModbusPort);
    gridSizer->Add(modbusChoice, 1, wxALIGN_CENTER_HORIZONTAL);
    wxChoice* modbusBaudChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, baudRates);
    modbusBaudChoice->SetStringSelection(selectedModbusBaudRate);
    gridSizer->Add(modbusBaudChoice, 1, wxALIGN_CENTER_HORIZONTAL);
    // Power Supply Label และ Choice
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Power Supply:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL);
    wxChoice* powerSupplyChoice = new wxChoice(this, wxID_ANY);
    for (const auto& port : availablePorts) {
        powerSupplyChoice->Append(port);
    }
    powerSupplyChoice->SetStringSelection(selectedPowerSupplyPort);
    gridSizer->Add(powerSupplyChoice, 1, wxALIGN_CENTER_HORIZONTAL);
    wxChoice* powerSupplyBaudChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, baudRates);
    powerSupplyBaudChoice->SetStringSelection(selectedPowerSupplyBaudRate);
    gridSizer->Add(powerSupplyBaudChoice, 1, wxALIGN_CENTER_HORIZONTAL);
    mainSizer->Add(gridSizer, 1, wxALL | wxALIGN_CENTER, 10);
    // Create the OK and Cancel buttons in a horizontal row
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okButton = new wxButton(this, wxID_OK, "OK");
    buttonSizer->Add(okButton, 0, wxALL | wxALIGN_CENTER, 10);
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    buttonSizer->Add(cancelButton, 0, wxALL | wxALIGN_CENTER, 10);
    mainSizer->Add(buttonSizer, 0, wxALL | wxALIGN_CENTER, 10);
    okButton->Bind(wxEVT_BUTTON, [this,BLEBaudChoice,BLEChoice, modbusChoice, powerSupplyChoice, modbusBaudChoice, powerSupplyBaudChoice](wxCommandEvent& event) {
		selectedBleAgentPort = BLEChoice->GetStringSelection().ToStdString();
		selectedBleAgentBaudRate = BLEBaudChoice->GetStringSelection().ToStdString();
        selectedModbusPort = modbusChoice->GetStringSelection().ToStdString();
        selectedPowerSupplyPort = powerSupplyChoice->GetStringSelection().ToStdString();
        selectedModbusBaudRate = stoi(modbusBaudChoice->GetStringSelection().ToStdString());
        selectedPowerSupplyBaudRate = stoi(powerSupplyBaudChoice->GetStringSelection().ToStdString());
        SaveSelectedPorts();
        io_service io;
        io_service io_v;
        serial_port serial = InitialSerial(io, selectedPowerSupplyPort.c_str());
		serial_port serial_v = InitialVirtualSerial(io_v, selectedBleAgentPort.c_str());
		//--------------------------------------------------------------------------------
        modbus_t* modbusContext = InitialModbus(selectedModbusPort.c_str());
#ifdef _WIN32
		string cmd_ble_agent = "mode " + selectedBleAgentPort + ": baud=" + selectedBleAgentBaudRate;
        string cmd_modbus = "mode " + selectedModbusPort + ": baud=" + selectedModbusBaudRate;
        string cmd_power_supply = "mode " + selectedPowerSupplyPort + ": baud=" + selectedPowerSupplyBaudRate;
        // Function to execute command in background
        auto executeCommand = [](const string& command) {
            STARTUPINFOA si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            ZeroMemory(&pi, sizeof(pi));
            if (!CreateProcessA(NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                // cerr << "CreateProcess failed (" << GetLastError() << ")." << endl;
            }
            else {
                // Wait until child process exits.
                WaitForSingleObject(pi.hProcess, INFINITE);
                // Close process and thread handles.
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            };
		executeCommand(cmd_ble_agent);
        executeCommand(cmd_modbus);
        executeCommand(cmd_power_supply);
#endif 
        if (modbusContext) {
            wxLogMessage("Modbus and Serial initialized successfully.");
        }
        else {
            wxLogError("Failed to initialize Modbus.");
        }

        if (modbusContext) {
            modbus_close(modbusContext);
            modbus_free(modbusContext);
        }
        this->EndModal(wxID_OK);
        }
    );
    cancelButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        this->EndModal(wxID_CANCEL); // ปิดหน้าต่างโดยไม่บันทึกค่า
        });
    SetSizer(mainSizer);
    Layout();
}
