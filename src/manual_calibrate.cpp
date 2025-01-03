#include "manual_calibrate.hpp"
using namespace std;
using namespace boost::asio;
//-----------------------------
vector<double> setpointData;
vector<double> flowData;
//-----------------------------
// เพิ่มค่าเริ่มต้นของ PID Controller 
//const double Kp = 0.007981535232; // for voltage control
const double Kp = 8.2111224966416e-4; // for current control
const double Ki = 0.0271542857142857; // 0.000047 - 0.000049
const double Kd = 0.0019875459404022;
double integral = 0.0;
double previousError = 0.0;
double pidOutput = 0.3 ;
io_service io; // iner loop ,fee foward
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
tuple<string, string, string> ReadPortsFromFile(const string& fileName) {
    ifstream file(fileName);
    if (!file.is_open()) {
        throw runtime_error("Unable to open the port configuration file."); // โยนข้อผิดพลาดหากเปิดไฟล์ไม่ได้
    }

    string bluetoothPort, modbusPort, serialPort;
    getline(file, bluetoothPort);
    getline(file, modbusPort);
    getline(file, serialPort);
    if (bluetoothPort.empty() || modbusPort.empty() || serialPort.empty()) {
        throw runtime_error("The port configuration file must contain at least 3 lines."); // โยนข้อผิดพลาดหากข้อมูลไม่ครบ
    }
    return make_tuple(bluetoothPort, modbusPort, serialPort); // คืนค่าพอร์ตทั้งหมดในรูปแบบ tuple
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
    // สร้างไฟล์ CSV
    ofstream outFile("flow_data.csv");
    if (!outFile.is_open()) {
        wxMessageBox("Unable to open file for writing.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }

    // เขียนหัวข้อในไฟล์ CSV
    outFile << "Timestamp,Flow\n";

    // เขียนข้อมูล flow ลงในไฟล์ CSV
    double timestamp = 0;
    for (double flow : flowData) {
        outFile << timestamp++ << "," << flow << "\n";
    }

    outFile.close();
    wxMessageBox("Data exported successfully!", "Success", wxOK | wxICON_INFORMATION, this);
    // ปิด dialog
    EndModal(wxID_OK);
}
void ManualCalibrationDialog::OnSetButtonClick(wxCommandEvent& event) {
    wxString inputStr = setFlowInput->GetValue();
    long value;
    if (inputStr.ToLong(&value)) {
        setpoint = static_cast<int>(value);
        // เริ่มทำงาน timer สำหรับสุ่มค่า
        if (!timer->IsRunning()) {
            timer->Start(500); // ทำงานทุก 500 miliseconds
        }
    }
    else {
        wxMessageBox("Invalid input. Please enter an integer value.", "Error", wxOK | wxICON_ERROR, this);
    }
}
//----------------------------------------------------------------------------------------------------------------------------------
ManualCalibrationDialog::ManualCalibrationDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, "Manual Calibration", wxDefaultPosition, wxSize(800, 800), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      setpoint(0), timer(new wxTimer(this)), modbusReadTimer(new wxTimer(this)),
    updateDisplayTimer(new wxTimer(this)), modbusCtx(nullptr) , serialCtx(io)  // กำหนดค่าเริ่มต้นให้ timer
{
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer *gridSizer = new wxFlexGridSizer(4, 3, 10, 10);
    // Set Flow Row
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Set Flow:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    setFlowInput = new wxTextCtrl(this, wxID_ANY);
    gridSizer->Add(setFlowInput, 1, wxEXPAND);
    wxButton *setButton = new wxButton(this, wxID_ANY, "Set");
    gridSizer->Add(setButton, 0, wxALIGN_CENTER);
    // Bind event handler ให้กับปุ่ม set
    setButton->Bind(wxEVT_BUTTON, &ManualCalibrationDialog::OnSetButtonClick, this);
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
    wxBoxSizer *gridCenterSizer = new wxBoxSizer(wxHORIZONTAL);
    gridCenterSizer->Add(gridSizer, 0, wxALIGN_CENTER);
    mainSizer->Add(gridCenterSizer, 1, wxALL | wxALIGN_CENTER, 10);
    // Graph setup
    graphWindow = new mpWindow(this, wxID_ANY, wxDefaultPosition, wxSize(800, 400));
    graphLayer = new mpFXYVector(wxT("Flow Data"));
    graphLayer->SetContinuity(true);
    graphWindow->AddLayer(graphLayer);
    graphWindow->Fit();
    // Add Graph first
    mainSizer->Add(graphWindow, 2, wxALL | wxEXPAND, 10);
    // Done Button
    wxButton* doneButton = new wxButton(this, wxID_OK, "Done"); 
    doneButton->Bind(wxEVT_BUTTON, &ManualCalibrationDialog::OnDoneButtonClick, this);
    mainSizer->Add(doneButton, 0, wxALL | wxALIGN_CENTER, 10);
    SetSizer(mainSizer);
    Layout();
    Center();

    // Load ports from file
    vector<string> selectedPorts;
    if (!CheckAndLoadPorts("properties.txt", selectedPorts)) {
        return; // หากไม่มีพอร์ตที่เลือกให้หยุดการทำงานของ dialog
    }
    if (selectedPorts.size() < 2) {
        wxMessageBox("Selected ports are insufficient.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    // Initialize Modbus
    modbusCtx = InitialModbus(selectedPorts[1].c_str());
    if (!modbusCtx) {
        wxMessageBox("Failed to initialize Modbus.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    // Initialize Serial Port (เรียกใช้ฟังก์ชัน init_serial_port)
    serialCtx = InitialSerial(io, selectedPorts[2].c_str());
    // cout << "Modbus and Serial port are initialized successfully." << std::endl;
    // เริ่ม Timer
    modbusReadTimer->Start(50);  // อ่านค่าทุก 50 ms
    updateDisplayTimer->Start(500); // แสดงผลทุก 500 ms
}
ManualCalibrationDialog::~ManualCalibrationDialog() {
    if (timer->IsRunning()) {
        timer->Stop();
    }
    if (modbusCtx) {
        modbus_close(modbusCtx);
        modbus_free(modbusCtx);
    }
    delete modbusReadTimer;
    delete updateDisplayTimer;
    if (serialCtx.is_open()) {
        serialCtx.close();
    }
    delete timer;
}
//----------------------------------------------------------------------------------------------------------------------------------
void ManualCalibrationDialog::OnModbusReadTimer(wxTimerEvent& event) {
    uint16_t refFlow[4];
    int rc = modbus_read_registers(modbusCtx, 6, 2, refFlow);
    if (rc == -1) {
        return; // ถ้าอ่านค่าไม่ได้ให้ข้ามไป
    }
    float refFlowValue;
    memcpy(&refFlowValue, refFlow, sizeof(refFlowValue));
    // เพิ่มค่าใหม่ใน Buffer
    valueBuffer.push_back(refFlowValue);
    if (valueBuffer.size() > BUFFER_SIZE) {
        valueBuffer.pop_front(); // ลบค่าเก่าเมื่อ Buffer เต็ม
    }
}
void ManualCalibrationDialog::OnUpdateDisplayTimer(wxTimerEvent& event) {
    if (valueBuffer.empty()) {
        return;
    }
    // คำนวณค่าเฉลี่ย Moving Average
    float sum = accumulate(valueBuffer.begin(), valueBuffer.end(), 0.0f);
    float movingAverage = sum / valueBuffer.size();
    // แสดงผลค่าเฉลี่ยที่คำนวณได้
    refFlowInput->SetValue(wxString::Format("%.2f", movingAverage));
    UpdateGraph(movingAverage); // อัปเดตกราฟ
}
//----------------------------------------------------------------------------------------------------------------------------------
void ManualCalibrationDialog::UpdateGraph(float flow) {
    // Update X and Y data
    double currentTime = xData.empty() ? 0 : xData.back() + 1;
    double currentFlow = flow; // Replace with actual flow value
    xData.push_back(currentTime);
    yData.push_back(currentFlow);
    // กำหนด setpoint value ที่คงที่
    setpointData.push_back(setpoint);
    // Update graph layer with both refFlow and setpoint data
    graphLayer = new mpFXYVector();  // สำหรับ RefFlow
    setpointLayer = new mpFXYVector(); // สำหรับ Setpoin
    graphLayer->SetContinuity(true);
    setpointLayer->SetContinuity(true);
    graphWindow->AddLayer(graphLayer);     // เพิ่ม RefFlow layer
    graphWindow->AddLayer(setpointLayer);  // เพิ่ม Setpoint layer
    // เปลี่ยนสีของแต่ละเลเยอร์
    graphLayer->SetPen(wxPen(*wxBLUE, 2, wxPENSTYLE_SOLID));   // RefFlow เป็นสีน้ำเงิน
    setpointLayer->SetPen(wxPen(*wxRED, 2, wxPENSTYLE_SOLID));   // Setpoint เป็นสีแดงแบบจุด
    graphLayer->SetData(xData, yData);         // เส้น RefFlow
    setpointLayer->SetData(xData, setpointData); // เส้น Setpoint
    graphWindow->Fit(); // ปรับมุมมองกราฟให้พอดี
    graphWindow->Refresh();
}
void ManualCalibrationDialog::OnTimer(wxTimerEvent &event) {
    uint16_t refFlow[4] ;
    int rc ;
    do{
        rc = modbus_read_registers(modbusCtx, /*register address=*/6, /*num registers=*/2, refFlow);
        if (rc == -1) {
        return;
    }
    }while(rc == -1);
    float refFlowValue ;
    double errorValue_percentage;
    memcpy(&refFlowValue , refFlow, sizeof(refFlowValue));
    // อัปเดตค่า refFlow
    refFlowInput->SetValue(wxString::Format("%.1f", refFlowValue));
    // อัปเดตค่าที่คำนวณได้
    //actFlowInput->SetValue(wxString::Format("%.2f", pidOutput));
    // คำนวณ Error
    errorValue_percentage = 100 - (refFlowValue*100)/setpoint;
    errorInput->SetValue(wxString::Format("%.1f", errorValue_percentage));
    // คำนวณค่า Act. Flow โดยใช้ PID
    pidOutput += calculatePID(setpoint, refFlowValue);
    // ป้องกันค่า PID Output เกินขอบเขตที่กำหนด
    if (pidOutput > 1.5) {
        pidOutput = 1.5;
	}
	else if (pidOutput < 0.3) {
		pidOutput = 0.3;
	}

    set_current(serialCtx, pidOutput);
    // เพิ่มค่าใหม่ใน Buffer
    valueBuffer.push_back(refFlowValue);
    if (valueBuffer.size() > BUFFER_SIZE) {
        valueBuffer.pop_front();
    }

    // คำนวณค่า Moving Average
    float sum = accumulate(valueBuffer.begin(), valueBuffer.end(), 0.0f);
    float movingAverage = sum / valueBuffer.size();

    flowData.push_back(refFlowValue); // เก็บค่า flow
    ofstream outFile("flow_data.csv", ios::app); // Append ข้อมูลลงในไฟล์
    if (outFile.is_open()) {
        outFile << std::fixed << std::setprecision(1) << "Real-time," << refFlowValue << "\n";
        outFile.close();
    }

    // อัปเดตกราฟด้วยค่าเฉลี่ย
    UpdateGraph(movingAverage);
}


// Bind Event Table
wxBEGIN_EVENT_TABLE(ManualCalibrationDialog, wxDialog)
    EVT_TIMER(wxID_ANY, ManualCalibrationDialog::OnTimer)
    EVT_TIMER(wxID_ANY, ManualCalibrationDialog::OnModbusReadTimer)
    EVT_TIMER(wxID_ANY, ManualCalibrationDialog::OnUpdateDisplayTimer)
    EVT_BUTTON(wxID_OK, ManualCalibrationDialog::OnDoneButtonClick)
wxEND_EVENT_TABLE()
