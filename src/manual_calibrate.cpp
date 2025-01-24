#include "manual_calibrate.hpp"
using namespace std;
using namespace boost::asio;
//-----------------------------
vector<double> setpointData;
vector<double> actData;
vector<double> errorData;
vector<double> refData;
//-----------------------------
// เพิ่มค่าเริ่มต้นของ PID Controller 
//const double Kp = 0.007981535232; // for voltage control
const double Kp = 9.03223474630576e-4; // for current control
const double Ki = 0.0271542857142857; 
const double Kd = 0.0020869232374223;
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
void ManualCalibrationDialog::OnLoggingButtonClick(wxCommandEvent& event) {
    graphWindow->Show(); // แสดงกราฟ
    Layout(); // ปรับ layout ใหม่
}
void ManualCalibrationDialog::OnStartButtonClick(wxCommandEvent& event) {
    wxString inputStr = setFlowInput->GetValue();
    long value;
    if (inputStr.ToLong(&value)) {
        setpoint = static_cast<int>(value);
        // เริ่ม timer และเรียก OnTimer เป็นครั้งแรก
        if (!timer->IsRunning()) {
            timer->Start(200); // ทำงานทุก 200 ms
        }
		startButton->Disable();
		stopButton->Enable();
    }
    else {
        wxMessageBox("Invalid input. Please enter an integer value.", "Error", wxOK | wxICON_ERROR, this);
    }
    
}
void ManualCalibrationDialog::OnStopButtonClick(wxCommandEvent& event) {
    if (timer->IsRunning()) {
        timer->Stop();
    }
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
        ofstream outFile(filePath.ToStdString());  // เปิดไฟล์ใหม่ตามที่ผู้ใช้เลือก
        if (!outFile.is_open()) {
            wxMessageBox("Unable to open file for writing.", "Error", wxOK | wxICON_ERROR, this);
            return;
        }

        // เขียนหัวข้อ CSV
        outFile << "Timestamp,Reference Flow,Active Flow,Error\n";

        // เขียนข้อมูลที่สะสมจาก OnTimer
        for (size_t i = 0; i < refData.size(); ++i) {
            outFile << timestampData[i] << ","   // ใช้ Timestamp ที่เก็บไว้
                << std::fixed << std::setprecision(3)
                << refData[i] << ","
                << actData[i] << ","
                << errorData[i] << "\n";
        }

        outFile.close();
        wxMessageBox("Data exported successfully!", "Success", wxOK | wxICON_INFORMATION, this);
    }
}
//----------------------------------------------------------------------------------------------------------------------------------
ManualCalibrationDialog::ManualCalibrationDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Manual Calibration", wxDefaultPosition, wxSize(400, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    setpoint(0), timer(new wxTimer(this)),modbusCtx(nullptr), serialCtx(io), BLECtx(){
    
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
    startButton = new wxButton(this,1011, "Start");
    stopButton = new wxButton(this, 1012, "Stop");
    startStopRowSizer->Add(startButton, 0, wxALL | wxALIGN_CENTER, 5);
    startStopRowSizer->Add(stopButton, 0, wxALL | wxALIGN_CENTER, 5);
    controlButtonSizer->Add(startStopRowSizer, 0, wxALIGN_CENTER | wxALL, 5);

    // Row for Show Graph Button
    showGraphButton = new wxButton(this, wxID_ANY, "Graph Logging");
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
    if (modbusCtx) {
        modbus_close(modbusCtx);
        modbus_free(modbusCtx);
    }
    if (serialCtx.is_open()) {
        serialCtx.close();
    } 
    delete timer;
}
//----------------------------------------------------------------------------------------------------------------------------------
void ManualCalibrationDialog::OnShowGraphButtonClick(wxCommandEvent& event) {
    wxDialog* graphDialog = new wxDialog(this, wxID_ANY, "Graph", wxDefaultPosition, wxSize(800, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

    mpWindow* graphWindow = new mpWindow(graphDialog, wxID_ANY, wxDefaultPosition, wxSize(800, 400));
    mpFXYVector* graphLayer = new mpFXYVector(wxT("Flow Data"));
    graphLayer->SetContinuity(true);
    graphWindow->AddLayer(graphLayer);
    graphWindow->Fit();

    wxBoxSizer* graphSizer = new wxBoxSizer(wxVERTICAL);
    graphSizer->Add(graphWindow, 1, wxALL | wxEXPAND, 10);

    graphDialog->SetSizer(graphSizer);
    graphDialog->Layout();
    graphDialog->ShowModal(); // แสดง dialog แบบ modal
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
    double errorValue_percentage;
    uint16_t refFlow[4] ;
    int rc ;
    // ใช้ค่าก่อนหน้าหากการอ่านผิดพลาด
    static float refFlowValue = 0.0;  // ใช้ตัวแปร static เพื่อเก็บค่าเดิมหากการอ่านผิดพลาด
    rc = modbus_read_registers(modbusCtx, 6, 2, refFlow);
    if (rc != -1) {  // ตรวจสอบว่า rc ไม่เท่ากับ -1
        memcpy(&refFlowValue, refFlow, sizeof(refFlowValue));  // อัปเดตค่า refFlow ใหม่เมื่อสำเร็จ
    }
    refFlowInput->SetValue(wxString::Format("%.1f", refFlowValue)); // อัปเดตค่า refFlow
	float actFlowValue = sendAndReceiveBetweenPorts(BLECtx); // ส่งคำสั่งไปยัง BLE
	actFlowInput->SetValue(wxString::Format("%.1f", actFlowValue )); // อัปเดตค่า actFlow  
    errorValue_percentage = 100 - (refFlowValue*100)/actFlowValue;  // คำนวณ Error
    errorInput->SetValue(wxString::Format("%.1f", errorValue_percentage));
    pidOutput += calculatePID(setpoint, refFlowValue); // คำนวณค่า Act. Flow โดยใช้ PID
    // ป้องกันค่า PID Output เกินขอบเขตที่กำหนด
    if (pidOutput > 1.5) {
        pidOutput = 1.5;
	}
	else if (pidOutput < 0.3) {
		pidOutput = 0.3;
	}
    //------------------------------------------------
	set_current(serialCtx, pidOutput); // ส่งค่า PID Output ไปที่ Serial Port
	//------------------------------------------------
    valueBufferRefFlow.push_back(refFlowValue); // เพิ่มค่าใหม่ใน Buffer
    if (valueBufferRefFlow.size() > BUFFER_SIZE) {
        valueBufferRefFlow.pop_front();
    }
	//------------------------------------------------
    // คำนวณค่า Moving Average
    float sum = accumulate(valueBufferRefFlow.begin(), valueBufferRefFlow.end(), 0.0f);
    float movingAverage = sum / valueBufferRefFlow.size();
	//------------------------------------------------
    refData.push_back(refFlowValue); // เก็บค่า flow
	actData.push_back(actFlowValue); // เก็บค่า flow
	errorData.push_back(errorValue_percentage); // เก็บค่า error
    // เก็บ Timestamp
    wxDateTime now = wxDateTime::Now();
    wxString currentTime = now.Format("%Y-%m-%d %H:%M:%S");
    currentTime+= "S";
    timestampData.push_back(currentTime.ToStdString());
 
}
//----------------------------------------------------------------------------------------------------------------------------------
// Bind Event Table
wxBEGIN_EVENT_TABLE(ManualCalibrationDialog, wxDialog)
    EVT_TIMER(wxID_ANY, ManualCalibrationDialog::OnTimer)
    EVT_BUTTON(1010, ManualCalibrationDialog::OnDoneButtonClick)
    EVT_BUTTON(1011, ManualCalibrationDialog::OnStartButtonClick)
    EVT_BUTTON(1012, ManualCalibrationDialog::OnStopButtonClick)
wxEND_EVENT_TABLE()
//----------------------------------------------------------------------------------------------------------------------------------