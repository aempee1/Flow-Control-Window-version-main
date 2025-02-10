#include "manual_calibrate.hpp"
using namespace std;
using namespace boost::asio;
//----------------------------------------------------------------------------------------------------------------------------------
static auto initialTime = std::chrono::steady_clock::now();
io_service io; 
std::atomic<bool> timerRunning{ false };
static bool startedTimer = false; // ตัวแปรใหม่เพื่อเก็บสถานะการเริ่มนับเวลา
//----------------------------------------------------------------------------------------------------------------------------------
string formatTimestamp(long milliseconds) {
    /*long hours = milliseconds / 3600000;
    milliseconds %= 3600000;
    long minutes = milliseconds / 60000;
    milliseconds %= 60000;*/
    long seconds = milliseconds / 1000.0;
    long millis = milliseconds % 1000;

    std::ostringstream oss;
    oss /*<< std::setw(2) << std::setfill('0') << hours << ":"
        << std::setw(2) << std::setfill('0') << minutes << ":"*/
        << std::setw(2) << std::setfill('0') << seconds << "."
        << std::setw(3) << std::setfill('0') << millis;
    return oss.str();
}
#include <Windows.h>
void SetThreadName(const std::string& name) {
    HRESULT hr = SetThreadDescription(GetCurrentThread(), std::wstring(name.begin(), name.end()).c_str());
    if (FAILED(hr)) {
        std::cerr << "Failed to set thread name: " << name << std::endl;
    }
}
//----------------------------------------------------------------------------------------------------------------------------------
ManualCalibrationDialog::ManualCalibrationDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Manual Calibration", wxDefaultPosition, wxSize(400, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    setpoint(0),modbusCtx(nullptr), serialCtx(io),BLECtx("") {
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
    timerRunning = false;
    StopThread();
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
tuple<string,string,string> ManualCalibrationDialog::ReadPortsFromFile(const string& fileName) {
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
    // ฟังก์ชันสำหรับรอ thread ด้วย timeout
    auto joinThreadWithTimeout = [](std::thread& t, int timeout_ms) {
        if (t.joinable()) {
            std::future<void> future = std::async(std::launch::async, &std::thread::join, &t);
            if (future.wait_for(std::chrono::milliseconds(timeout_ms)) == std::future_status::timeout) {
                // จัดการกรณี thread ค้าง
                wxMessageBox("Thread timeout occurred", "Warning", wxOK | wxICON_WARNING);
                return false;
            }
        }
        return true;
        };
    timerRunning = false;
    // รอ thread ด้วย timeout
    if (!joinThreadWithTimeout(modbusThread, 1000)) {
        // จัดการกรณี thread ค้าง
    }
    if (!joinThreadWithTimeout(bluetoothThread, 1000)) {
        // จัดการกรณี thread ค้าง
    }
    if (!joinThreadWithTimeout(pidCalculationThread, 1000)) {
        // จัดการกรณี thread ค้าง
    }
    // หยุด Thread Timer
    StopThread();
    stopButton->Disable();
    startButton->Enable();
    setFlowInput->SetEditable(true);
    // ลบไฟล์ dump หลังจากเขียนเสร็จ
    if (!dumpFilePath.empty()) {
        std::remove(dumpFilePath.c_str());
        dumpFilePath.clear();
    }
    pushTimestamps.clear();
    refData.clear();
    actData.clear();
    startedTimer = false;
    clearQueue(refFlowBuffer);
    clearQueue(actFlowBuffer);
    totalEntriesWritten = 0;
    // ปิด dialog
    EndModal(wxID_OK);

}
void ManualCalibrationDialog::OnStartButtonClick(wxCommandEvent& event) {
    wxString inputStr = setFlowInput->GetValue();
    long value;
    if (inputStr.ToLong(&value)) {
        setpoint = static_cast<int>(value);
        timerRunning = true;
		StartThread();
        // ปิดการใช้งานปุ่ม Start และเปิดใช้งานปุ่ม Stop
        startButton->Disable();
        stopButton->Enable();
        setFlowInput->SetEditable(false);
    }
    else if (value < MIN_SETPOINT || value > MAX_SETPOINT) {
        wxMessageBox(wxString::Format("Please enter a value between %d and %d.", MIN_SETPOINT, MAX_SETPOINT), "Invalid Input", wxOK | wxICON_ERROR, this);
    }
    else {
        wxMessageBox("Invalid input. Please enter an integer value.", "Error", wxOK | wxICON_ERROR, this);
    }
}
void ManualCalibrationDialog::OnStopButtonClick(wxCommandEvent& event) {
    // หยุด Thread Timer
    StopThread();
    stopButton->Disable();
    startButton->Enable();
    setFlowInput->SetEditable(true); 
	//----------------------------------------------------------------------------------------------------------------------------------
    // ดึงวันที่ปัจจุบัน
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d");  // yyyyMMdd
    std::string dateStr = oss.str();
    std::ostringstream fileNameStream;
    SerialNumber = sendRequestSerialNumber(BLECtx);
    if (SerialNumber == 0) {
        wxMessageBox("Failed to get Serial Number.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    fileNameStream << "log_SN" << std::setfill('0') << std::setw(8) << SerialNumber << "_" << dateStr << ".csv";
    string file_name = fileNameStream.str();
	//----------------------------------------------------------------------------------------------------------------------------------
    wxMessageDialog confirmDialog(this,
        "Do you want to save the log data?",
        "Confirm Save",
        wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
    if (confirmDialog.ShowModal() == wxID_YES) {
        wxFileDialog saveFileDialog(this,
            "Save Log File",
            "", file_name,  // ตั้งชื่อไฟล์เริ่มต้น
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
        outFile << "Time (s),Reference Flow (l/min),Active Flow (l/min),Error (%)\n";
        // ถ้ามีไฟล์ dump ให้อ่านข้อมูลจากไฟล์ dump ก่อน
        if (!dumpFilePath.empty()) {
            std::ifstream dumpFile(dumpFilePath);
            if (dumpFile.is_open()) {
                std::string line;
                while (std::getline(dumpFile, line)) {
                    std::stringstream ss(line);
                    std::string timestamp, ref, act;

                    std::getline(ss, timestamp, ',');
                    std::getline(ss, ref, ',');
                    std::getline(ss, act, ',');

                    float refValue = std::stof(ref);
                    float actValue = std::stof(act);
                    float errorValue_percentage = (actValue != 0.0f) ?
                        ((refValue - actValue) / refValue) * 100.0f : 0.0f;

                    outFile << formatTimestamp(std::stoll(timestamp)) << ","
                        << ref << ","
                        << act << ","
                        << abs(errorValue_percentage) << "\n";
                }
                dumpFile.close();
            }
        }
        // Write data
        for (size_t i = 0; i < refData.size(); ++i) {
            float errorValue_percentage = (refData[i] != 0.0f) ?
                ((refData[i] - actData[i]) / refData[i]) * 100.0f : 0.0f;
            outFile << formatTimestamp(pushTimestamps[i]) << ","
                << refData[i] << ","
                << actData[i] << ","
                << abs(errorValue_percentage) << "\n";
        }
		//----------------------------------------------------------------------------------------------------------------------------------
        outFile.close();
        // ลบไฟล์ dump หลังจากเขียนเสร็จ
        if (!dumpFilePath.empty()) {
            std::remove(dumpFilePath.c_str());
            dumpFilePath.clear();
        }
        pushTimestamps.clear();
        refData.clear();
        actData.clear();
        startedTimer = false;
		clearQueue(refFlowBuffer);
		clearQueue(actFlowBuffer);
        totalEntriesWritten = 0;
        wxMessageBox("Data exported successfully!", "Success", wxOK | wxICON_INFORMATION, this);
    }
}
//----------------------------------------------------------------------------------------------------------------------------------
void ManualCalibrationDialog::StartThread() {
    modbusThread = std::thread([this]() {
        SetThreadName("modbus thread");
        while (timerRunning) {
            readModbusWorker();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        });
    bluetoothThread = std::thread([this]() {
        SetThreadName("BLE thread");
        while (timerRunning) {
            readBluetoothWorker();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        });
    pidCalculationThread = std::thread([this]() {
        SetThreadName("PID thread");
        while (timerRunning) {
            calculatePIDWorker();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        });
    readTimerThread = std::thread([this]() {
        SetThreadName("Read thread");
        while (timerRunning) {
            OnReadTimer();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        });
    displayTimerThread = std::thread([this]() {
        SetThreadName("Display thread");
        while (timerRunning) {
            OnDisplayTimer();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        });
}
void ManualCalibrationDialog::StopThread() {
    timerRunning = false;
    timerCv.notify_all();  // ปลุก thread เพื่อให้ออกจาก wait
	if (bluetoothThread.joinable()) {
		bluetoothThread.join();  // รอให้ thread จบการทำงาน
	}
	if (modbusThread.joinable()) {
		modbusThread.join();  // รอให้ thread จบการทำงาน
	}
    if (readTimerThread.joinable()) {
        readTimerThread.join();  // รอให้ thread จบการทำงาน
    }
    if (displayTimerThread.joinable()) {
        displayTimerThread.join();  // รอให้ thread จบการทำงาน
    }
	if (pidCalculationThread.joinable()) {
		pidCalculationThread.join();  // รอให้ thread จบการทำงาน
	}
}
//----------------------------------------------------------------------------------------------------------------------------------
void ManualCalibrationDialog::readModbusWorker() {
        std::atomic<float> localRefFlowValue{ 0.0f };
        rc = modbus_read_registers(modbusCtx, 6, 2, refFlow);
        if (rc != -1) {
             memcpy(&localRefFlowValue, refFlow, sizeof(localRefFlowValue));
             localRefFlowValue.store(std::round(localRefFlowValue.load() * 1000.0) / 1000.0);
        }
        else {
             localRefFlowValue.store(0.0f);
        } 
        localRefFlowValue.store(refFlowValue.load());
        refFlowBuffer.push({ localRefFlowValue.load()});
}
void ManualCalibrationDialog::readBluetoothWorker() {
        atomic<float> localActFlowValue{ 0.0f };
        localActFlowValue.store(sendAndReceiveBetweenPorts(BLECtx));
        localActFlowValue.store(actFlowValue.load());
        actFlowBuffer.push({localActFlowValue.load()});
     /*  boost::this_thread::sleep(boost::posix_time::milliseconds(100));*/  
}
//----------------------------------------------------------------------------------------------------------------------------------
void ManualCalibrationDialog::dumpDataToFile() {
    if (refData.size() >= DUMP_THRESHOLD) {
        if (dumpFilePath.empty()) {
            // สร้างชื่อไฟล์ dump ชั่วคราว
            std::ostringstream oss;
            oss << "temp_dump_" << SerialNumber << "_" << std::time(nullptr) << ".tmp";
            dumpFilePath = oss.str();
        }
        std::ofstream dumpFile(dumpFilePath, std::ios::app);
        if (!dumpFile.is_open()) {
            wxMessageBox("Unable to open dump file for writing.", "Error", wxOK | wxICON_ERROR, this);
            return;
        }
        // เขียนข้อมูลลงในไฟล์ dump
        for (size_t i = 0; i < refData.size(); ++i) {
            dumpFile << pushTimestamps[i] << ","
                << refData[i] << ","
                << actData[i] << "\n";
        }
        totalEntriesWritten += refData.size();
        pushTimestamps.clear();
        refData.clear();
        actData.clear();
        dumpFile.close();
    }
}
//----------------------------------------------------------------------------------------------------------------------------------
void ManualCalibrationDialog::OnReadTimer() {
    DataEntry actEntry, refEntry;
    refFlowBuffer.pop(refEntry);
    actFlowBuffer.pop(actEntry);
    auto currentTime = std::chrono::steady_clock::now();
    if (!startedTimer) {
       initialTime = currentTime; 
       startedTimer = true; 
    }
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - initialTime).count();
    refData.push_back(refEntry.Flow);
    actData.push_back(actEntry.Flow);
    pushTimestamps.push_back(elapsedTime); 
    dumpDataToFile();
}
void ManualCalibrationDialog::OnDisplayTimer() {
    if (refFlowValue != 0) {
        errorValue_percentage = ((refFlowValue - actFlowValue) / refFlowValue) * 100.0f;
    }
    else {
        errorValue_percentage = 0.0f; // ป้องกันหารด้วย 0
    }
    refFlowInput->SetValue(wxString::Format("%.1f", refFlowValue.load()));
    actFlowInput->SetValue(wxString::Format("%.1f", actFlowValue.load()));
    errorInput->SetValue(wxString::Format("%.1f", errorValue_percentage.load()));
}
//----------------------------------------------------------------------------------------------------------------------------------
void ManualCalibrationDialog::calculatePIDWorker() {
    // ใช้ atomic สำหรับตัวแปรที่ใช้ร่วมกัน
    std::atomic<float> localRefFlowValue{0.0f};
    std::atomic<float> localActFlowValue{0.0f};
    std::atomic<float> localPIDOutput{0.0f};
    while (timerRunning) {
        float localRefFlowValue = refFlowValue.load();
        float localActFlowValue = actFlowValue.load();
        float localPIDOutput = calculatePID(setpoint, localRefFlowValue);
        localPIDOutput = std::clamp<float>(localPIDOutput, 0.3f, 1.5f);
        pidOutput = localPIDOutput;
        // เพิ่ม timeout สำหรับการส่งค่า
        set_current(serialCtx, localPIDOutput);
        /*std::this_thread::sleep_for(std::chrono::milliseconds(100));*/
    }
}
// Bind Event Table
wxBEGIN_EVENT_TABLE(ManualCalibrationDialog, wxDialog)
    EVT_BUTTON(1010, ManualCalibrationDialog::OnDoneButtonClick)
    EVT_BUTTON(1011, ManualCalibrationDialog::OnStartButtonClick)
    EVT_BUTTON(1012, ManualCalibrationDialog::OnStopButtonClick)
wxEND_EVENT_TABLE()
//lockfree 
//----------------------------------------------------------------------------------------------------------------------------------