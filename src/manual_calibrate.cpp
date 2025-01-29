#include "manual_calibrate.hpp"
using namespace std;
using namespace boost::asio;
//-----------------------------
static auto initialTime = std::chrono::steady_clock::now();
io_service io; // iner loop ,fee foward
std::atomic<bool> keepRunning{false};
//----------------------------------------------------------------------------------------------------------------------------------
std::string formatTimestamp(long milliseconds) {
    long hours = milliseconds / 3600000;
    milliseconds %= 3600000;
    long minutes = milliseconds / 60000;
    milliseconds %= 60000;
    long seconds = milliseconds / 1000;
    long millis = milliseconds % 1000;

    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << hours << ":"
        << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << seconds << "::"
        << std::setw(3) << std::setfill('0') << millis;
    return oss.str();
}
//----------------------------------------------------------------------------------------------------------------------------------
ManualCalibrationDialog::ManualCalibrationDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Manual Calibration", wxDefaultPosition, wxSize(400, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    setpoint(0), timerRead(this, 1009),modbusCtx(nullptr), serialCtx(io),BLECtx() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(4, 3, 10, 10);
    // Set Flow Row
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Set Flow:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    setFlowInput = new wxTextCtrl(this, wxID_ANY);
    gridSizer->Add(setFlowInput, 1, wxEXPAND);
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "m3/h"), 0, wxALIGN_CENTER);

    // Ref. Flow Row
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Ref. Flow:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    refFlowInput = new wxTextCtrl(this, wxID_ANY);
    refFlowInput->SetEditable(false); // Make the field read-only
    gridSizer->Add(refFlowInput, 1, wxEXPAND);
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "m3/h"), 0, wxALIGN_CENTER_VERTICAL);
    // Act. Flow Row
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Act. Flow:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    actFlowInput = new wxTextCtrl(this, wxID_ANY);
    actFlowInput->SetEditable(false); // Make the field read-only
    gridSizer->Add(actFlowInput, 1, wxEXPAND);
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "m3/h"), 0, wxALIGN_CENTER_VERTICAL);

    // Error Row
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Error:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    errorInput = new wxTextCtrl(this, wxID_ANY);
    errorInput->SetEditable(false);
    gridSizer->Add(errorInput, 1, wxEXPAND);
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "%"), 0, wxALIGN_CENTER_VERTICAL);

    // Center Grid
    wxBoxSizer* gridCenterSizer = new wxBoxSizer(wxHORIZONTAL);
    gridCenterSizer->Add(gridSizer, 0, wxALIGN_CENTER);
    mainSizer->Add(gridCenterSizer, 0, wxALL | wxALIGN_CENTER, 20);

    // Control Buttons
    wxBoxSizer* controlButtonSizer = new wxBoxSizer(wxVERTICAL);

    // Row for Start and Stop Buttons
    wxBoxSizer* startStopRowSizer = new wxBoxSizer(wxHORIZONTAL);
    startButton = new wxButton(this, 1011, "Start");
    stopButton = new wxButton(this, 1012, "Stop");
    startStopRowSizer->Add(startButton, 0, wxALL | wxALIGN_CENTER, 5);
    startStopRowSizer->Add(stopButton, 0, wxALL | wxALIGN_CENTER, 5);
    controlButtonSizer->Add(startStopRowSizer, 0, wxALIGN_CENTER | wxALL, 5);

    // Row for Show Graph Button
    showGraphButton = new wxButton(this, 1013, "Graph Logging");
    controlButtonSizer->Add(showGraphButton, 2, wxALL | wxALIGN_CENTER, 5);

    // Disable Stop Button Initially
    stopButton->Disable();

    // Add Control Buttons to Main Sizer
    mainSizer->Add(controlButtonSizer, 0, wxALIGN_CENTER | wxALL, 10);
    // Done Button
    wxBoxSizer* doneButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    doneButton = new wxButton(this, 1010, "Done");
    doneButtonSizer->Add(doneButton, 0, wxALL | wxALIGN_CENTER, 5);
    mainSizer->Add(doneButtonSizer, 0, wxALIGN_CENTER | wxALL, 10);
    // ตั้งค่า sizer หลัก
    SetSizer(mainSizer);
    Layout();
    Center();
    // Load ports from file
    vector<string> selectedPorts;
    if (!CheckAndLoadPorts("properties.txt", selectedPorts)) {
        return; // หากไม่มีพอร์ตที่เลือกให้หยุดการทำงานของ dialog
    }
    if (selectedPorts.size() < 3) {
        wxMessageBox("Selected ports are insufficient.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    BLECtx = selectedPorts[0];
    serialCtx = InitialSerial(io, selectedPorts[2].c_str());
    modbusCtx = InitialModbus(selectedPorts[1].c_str());
    if (!modbusCtx) {
        wxMessageBox("Failed to initialize Modbus.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
}
ManualCalibrationDialog::~ManualCalibrationDialog() {
    keepRunning = false; // กันการรันต่อหลังจากปิด
    if (modbusThread.joinable()) modbusThread.join();
    if (bluetoothThread.joinable()) bluetoothThread.join();
    if (modbusCtx) {
        modbus_close(modbusCtx);
        modbus_free(modbusCtx);
    }
    if (serialCtx.is_open()) {
        serialCtx.close();
    }
 
}
//----------------------------------------------------------------------------------------------------------------------------------
serial_port ManualCalibrationDialog::InitialSerial(io_service& io, const string& port_name)
{
    serial_port serial(io, port_name);
    serial.set_option(serial_port_base::baud_rate(9600));
    serial.set_option(serial_port_base::character_size(8));
    serial.set_option(serial_port_base::parity(serial_port_base::parity::none));
    serial.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));
    serial.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));

    if (!serial.is_open())
    {
        // cerr << "Failed to open serial port!" << endl;
        throw runtime_error("Failed to open serial port");
    }
    // cout << "Successfully initialized Serial on port: " << port_name << endl;
    return serial;
}
modbus_t* ManualCalibrationDialog::InitialModbus(const char* modbus_port) {
    modbus_t* ctx = initialize_modbus(modbus_port);
    return ctx;
}
tuple<string,string,string> ReadPortsFromFile(const string& fileName) {
    ifstream file(fileName);
    if (!file.is_open()) {
        throw runtime_error("Unable to open the port configuration file."); // โยนข้อผิดพลาดหากเปิดไฟล์ไม่ได้
    }

    string BLEPort, modbusPort, serialPort;
	getline(file, BLEPort);
    getline(file, modbusPort);
    getline(file, serialPort);
    if ( BLEPort.empty() ||  modbusPort.empty() || serialPort.empty()) {
        throw runtime_error("The port configuration file must contain at least 3 lines."); // โยนข้อผิดพลาดหากข้อมูลไม่ครบ
    }
    return make_tuple(BLEPort,modbusPort, serialPort); // คืนค่าพอร์ตทั้งหมดในรูปแบบ tuple
}
bool ManualCalibrationDialog::CheckAndLoadPorts(const string& fileName, vector<string>& ports) {
    cout << "Attempting to open the file: " << fileName << endl;  // เพิ่ม log message
    ifstream file(fileName);
    if (!file.is_open()) {
        wxMessageBox("Unable to open the file properties", "Error", wxOK | wxICON_ERROR, this);
        // cerr << "Failed to open file: " << fileName << endl;  // เพิ่ม log message
        return false;
    }
    //cout << "File opened successfully: " << fileName << endl;  // เพิ่ม log message
    string port;
    while (getline(file, port)) {
        if (!port.empty()) {
            ports.push_back(port);
        }
    }
    if (ports.empty()) {
        wxMessageBox("You don't have selected ports", "Warning", wxOK | wxICON_WARNING, this);
        // cerr << "No ports selected in the file." << endl;  // เพิ่ม log message
        return false;
    }
    // cout << "Ports loaded successfully." << endl;  // เพิ่ม log message
    return true;
}
//----------------------------------------------------------------------------------------------------------------------------------
double ManualCalibrationDialog::calculatePID(double setpointValue, double currentValue) {
    // คำนวณ Error
    double errorValue = setpointValue - currentValue;
    // คำนวณ Derivative
    double derivativeValue = errorValue - previousError;
    // อัปเดต Error ก่อนหน้า
    previousError = errorValue;
    // คำนวณ PID Output
    double PID_output = (Kp * errorValue) + (Ki * integral) + (Kd * derivativeValue);
    return PID_output;
}
//----------------------------------------------------------------------------------------------------------------------------------
void ManualCalibrationDialog::OnDoneButtonClick(wxCommandEvent& event) {
    // ปิด dialog
    EndModal(wxID_OK);
}
void ManualCalibrationDialog::OnStartButtonClick(wxCommandEvent& event) {
    wxString inputStr = setFlowInput->GetValue();
    long value;
    if (inputStr.ToLong(&value)) {
        setpoint = static_cast<int>(value);
		keepRunning = true;
        initialTime = std::chrono::steady_clock::now();
        // เริ่ม Thread Modbus และ Bluetooth
        modbusThread = std::thread(&ManualCalibrationDialog::readModbusWorker, this);
        bluetoothThread = std::thread(&ManualCalibrationDialog::readBluetoothWorker, this);
        // เริ่ม Timer
        timerRead.Start(200);
        startButton->Disable();
        stopButton->Enable();
    }
    else if(value < MIN_SETPOINT || value > MAX_SETPOINT){
        wxMessageBox(wxString::Format("Please enter a value between %d and %d.", MIN_SETPOINT, MAX_SETPOINT), "Invalid Input", wxOK | wxICON_ERROR, this);
    }
    else {
        wxMessageBox("Invalid input. Please enter an integer value.", "Error", wxOK | wxICON_ERROR, this);
    }
}
void ManualCalibrationDialog::OnStopButtonClick(wxCommandEvent& event) {
	keepRunning = false;
    if (modbusThread.joinable()) modbusThread.join();
    if (bluetoothThread.joinable()) bluetoothThread.join();
    timerRead.Stop();
    stopButton->Disable();
    startButton->Enable();
    wxMessageDialog confirmDialog(this,
        "Do you want to save the log data?",
        "Confirm Save",
        wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
    if (confirmDialog.ShowModal() == wxID_YES) {
        wxFileDialog saveFileDialog(this,
            "Save Log File",
            "", "",
            "CSV files (*.csv)|*.csv",
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL) {
            return;
        }
        wxString filePath = saveFileDialog.GetPath();
        std::ofstream outFile(filePath.ToStdString());
        if (!outFile.is_open()) {
            wxMessageBox("Unable to open file for writing.", "Error", wxOK | wxICON_ERROR, this);
            return;
        }
        // Write CSV header
        outFile << "Timestamp (ms),Reference Flow (l/min),Active Flow (l/min),Error (%)\n";
        // Write data
        for (size_t i = 0; i < refData.size(); ++i) {
            float errorValue_percentage = (refData[i] != 0.0f) ?
                ((refData[i] - actData[i]) / refData[i]) * 100.0f : 0.0f;
            outFile << formatTimestamp(pushTimestamps[i]) << ","
                << refData[i] << ","
                << actData[i] << ","
                << errorValue_percentage << "\n";
        }

        outFile.close();
        wxMessageBox("Data exported successfully!", "Success", wxOK | wxICON_INFORMATION, this);
    }
}
//----------------------------------------------------------------------------------------------------------------------------------
// Thread สำหรับอ่านค่า Modbus
void ManualCalibrationDialog::readModbusWorker() {
    while (keepRunning) {
        float localRefFlowValue = 0.0f;
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            rc = modbus_read_registers(modbusCtx, 6, 2, refFlow);
            if (rc != -1) {
                memcpy(&localRefFlowValue, refFlow, sizeof(localRefFlowValue));
            }
        }
        refFlowBuffer.push({ localRefFlowValue });      
        std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Delay 200ms
    }
}
// Thread สำหรับอ่านค่า Bluetooth
void ManualCalibrationDialog::readBluetoothWorker() {
    while (keepRunning) {
        float localActFlowValue = 0.0f;
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            localActFlowValue = sendAndReceiveBetweenPorts(BLECtx);
        }
        actFlowBuffer.push({ localActFlowValue });
        std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Delay 200ms
    }
}
// Thread สำหรับ Timer (200ms)
void ManualCalibrationDialog::OnReadTimer(wxTimerEvent& event) {
    DataEntry actEntry, refEntry;

    while (!refFlowBuffer.empty() && !actFlowBuffer.empty()) {
        // ดึงข้อมูลจาก buffer (pop ข้อมูลแล้วออกจาก buffer)
        bool hasRef = refFlowBuffer.pop(refEntry);
        bool hasAct = actFlowBuffer.pop(actEntry);

        if (hasRef && hasAct) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - initialTime).count();

            // Push ข้อมูลลง Vector
            refData.push_back(refEntry.Flow);
            actData.push_back(actEntry.Flow);
            pushTimestamps.push_back(elapsedTime);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
// Bind Event Table
wxBEGIN_EVENT_TABLE(ManualCalibrationDialog, wxDialog)
    EVT_TIMER(1009, ManualCalibrationDialog::OnReadTimer)    // Event สำหรับ Timer อ่านค่า
    EVT_BUTTON(1010, ManualCalibrationDialog::OnDoneButtonClick)
    EVT_BUTTON(1011, ManualCalibrationDialog::OnStartButtonClick)
    EVT_BUTTON(1012, ManualCalibrationDialog::OnStopButtonClick)
wxEND_EVENT_TABLE()
//----------------------------------------------------------------------------------------------------------------------------------