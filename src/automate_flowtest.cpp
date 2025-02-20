﻿#include "automate_flowtest.hpp"
vector<int> setpoint = {};
AutomateCheckpointDialog::AutomateCheckpointDialog(wxWindow* parent) 
    : wxDialog(parent, wxID_ANY, "Auto Mate Calibrate Test", wxDefaultPosition, wxSize(700, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), modbusCtx(nullptr), serialCtx(io_serial), BLECtx(io_ble) {
    panel = new wxPanel(this);
    vbox = new wxBoxSizer(wxVERTICAL);
    // กำหนดค่า grid
    grid = new wxGridSizer(11, 5, 5, 5);
    // Header
    title = new wxStaticText(panel, wxID_ANY, "Auto Flow Checkpoint", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    title->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    vbox->Add(title, 0, wxALL | wxALIGN_CENTER, 10);
    sensorText = new wxStaticText(panel, wxID_ANY, "Sensor: ");
    sensorText->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    vbox->Add(sensorText, 0, wxLEFT | wxTOP, 10);
    pipeText = new wxStaticText(panel, wxID_ANY, "Pipe Diameter (mm): ");
    pipeText->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    vbox->Add(pipeText, 0, wxLEFT | wxBOTTOM, 10);
    // Column Headers
    grid->Add(new wxStaticText(panel, wxID_ANY, "Point", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
    grid->Add(new wxStaticText(panel, wxID_ANY, "Flow l/m", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
    grid->Add(new wxStaticText(panel, wxID_ANY, "Act Flow", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
    grid->Add(new wxStaticText(panel, wxID_ANY, "Ref Flow", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
    grid->Add(new wxStaticText(panel, wxID_ANY, "Error %", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
    vbox->Add(grid, 1, wxEXPAND | wxALL, 10);
    // ปุ่ม
    hbox = new wxBoxSizer(wxHORIZONTAL);
    loadFileBtn = new wxButton(panel, 1021, "Load File");
    exportBtn = new wxButton(panel, 1023, "Export Log");
    startBtn = new wxButton(panel, 1020, "Start");
    stopBtn = new wxButton(panel, 1024, "Stop");
    cancelBtn = new wxButton(panel, wxID_CANCEL, "Cancel");
    // ปุ่ม Load File และ Export Log อยู่ใน row เดียวกัน
    hbox->Add(loadFileBtn, 0, wxRIGHT, 10);
    hbox->Add(exportBtn, 0, wxRIGHT, 10);
    // ปุ่ม Start และ Stop อยู่มุมล่างซ้ายของ UI
    hbox->Add(startBtn, 0, wxRIGHT, 10);
    hbox->Add(stopBtn, 0, wxRIGHT, 10);
    hbox->AddStretchSpacer(); // ให้ปุ่ม Cancel ไปอยู่ทางขวา
    hbox->Add(cancelBtn, 0);
    vbox->Add(hbox, 0, wxEXPAND | wxALL, 10);
    panel->SetSizer(vbox);
    // Load ports from file
    vector<string> selectedPorts;
    if (!CheckAndLoadPorts("properties.txt", selectedPorts)) {
        return;
    }
    if (selectedPorts.size() < 3) {
        wxMessageBox("Selected ports are insufficient.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    BLECtx = InitialSerial(io_ble, selectedPorts[0].c_str(), 115200);
    serialCtx = InitialSerial(io_serial, selectedPorts[2].c_str(), 9600);
    modbusCtx = InitialModbus(selectedPorts[1].c_str());
    if (!modbusCtx) {
        wxMessageBox("Failed to initialize Modbus.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
}
AutomateCheckpointDialog::~AutomateCheckpointDialog() {
    if (modbusCtx) {
        modbus_close(modbusCtx);
        modbus_free(modbusCtx);
    }
    if (serialCtx.is_open()) {
        serialCtx.close();
    }
    if (BLECtx.is_open()) {
        BLECtx.close();
    }
}
//----------------------------------------------------------------------------------------------------------------------
double AutomateCheckpointDialog::calculatePID(double setpointValue, double currentValue) {
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
//----------------------------------------------------------------------------------------------------------------------
serial_port AutomateCheckpointDialog::InitialSerial(io_service& io, const string& port_name, unsigned int baudrate)
{
    serial_port serial(io, port_name);
    serial.set_option(serial_port_base::baud_rate(baudrate));
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
modbus_t* AutomateCheckpointDialog::InitialModbus(const char* modbus_port) {
    modbus_t* ctx = initialize_modbus(modbus_port);
    return ctx;
}
tuple<string, string, string> AutomateCheckpointDialog::ReadPortsFromFile(const string& fileName) {
    ifstream file(fileName);
    if (!file.is_open()) {
        throw runtime_error("Unable to open the port configuration file."); // โยนข้อผิดพลาดหากเปิดไฟล์ไม่ได้
    }

    string BLEPort, modbusPort, serialPort;
    getline(file, BLEPort);
    getline(file, modbusPort);
    getline(file, serialPort);
    if (BLEPort.empty() || modbusPort.empty() || serialPort.empty()) {
        throw runtime_error("The port configuration file must contain at least 3 lines."); // โยนข้อผิดพลาดหากข้อมูลไม่ครบ
    }
    return make_tuple(BLEPort, modbusPort, serialPort); // คืนค่าพอร์ตทั้งหมดในรูปแบบ tuple
}
bool AutomateCheckpointDialog::CheckAndLoadPorts(const string& fileName, vector<string>& ports) {
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
//----------------------------------------------------------------------------------------------------------------------
void AutomateCheckpointDialog::LoadDataFromFile(const string& filePath) {
    ifstream file(filePath);
    if (!file.is_open()) {
        wxMessageBox("Failed to open file", "Error", wxOK | wxICON_ERROR, this);
        return;
    }

    string line;
    string sensor;
    string pipeDiameter;
    setpoint.clear();
    actFlowCells.clear();
    refFlowCells.clear();
    errorCells.clear();

    vector<pair<int, int>> checkPoints;
    while (getline(file, line)) {
        // อ่านค่า Sensor
        if (line.find("Sensor:") == 0) {
            sensor = line.substr(8);
            sensor.erase(sensor.begin(), find_if(sensor.begin(), sensor.end(), [](unsigned char ch) { return !isspace(ch); })); // Trim leading spaces
        }
        // อ่านค่า Pipe Diameter
        else if (line.find("Pipe Diameter:") == 0) {
            pipeDiameter = line.substr(14);
            pipeDiameter.erase(pipeDiameter.begin(), find_if(pipeDiameter.begin(), pipeDiameter.end(), [](unsigned char ch) { return !isspace(ch); }));
        }
        // อ่าน Check Points
        else if (isdigit(line[0])) {
            istringstream iss(line);
            int point, value;
            char comma;
            if (iss >> point >> comma >> value) {
                checkPoints.emplace_back(point, value);
                setpoint.push_back(value);
            }
        }
    }

    file.close();

    // อัปเดตค่า Sensor และ Pipe Diameter ใน UI
    sensorText->SetLabel("Sensor: " + sensor);
    pipeText->SetLabel("Pipe Diameter: " + pipeDiameter);

    if (!grid || checkPoints.empty()) return;

    grid->Clear(true);

    // เพิ่ม Header
    grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "Point", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
    grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "Flow l/m", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
    grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "Act Flow", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
    grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "Ref Flow", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
    grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "Error (%)", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
    for (const auto& [point, flow] : checkPoints) {
        grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, to_string(point), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
        grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, to_string(flow), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);

        wxStaticText* actFlowText = new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        wxStaticText* refFlowText = new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        wxStaticText* errorText = new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

        actFlowCells.push_back(actFlowText);
        refFlowCells.push_back(refFlowText);
        errorCells.push_back(errorText);

        grid->Add(actFlowText, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
        grid->Add(refFlowText, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
        grid->Add(errorText, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
    }

    grid->Layout();
    grid->GetContainingWindow()->Layout();
}
void AutomateCheckpointDialog::OnLoadFile(wxCommandEvent& event) {
    wxFileDialog openFileDialog(this, "Open TXT file", "", "", "Text files (*.txt)|*.txt", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        return;
    }
    LoadDataFromFile(openFileDialog.GetPath().ToStdString());
}
//----------------------------------------------------------------------------------------------------------------------
void AutomateCheckpointDialog::OnUpdateFlowTimer(wxTimerEvent& event) {
    if (currentRowIndex >= setpoint.size()) {
        updateTimer.Stop();
        return;
    }
    TuneFlowByPID(setpoint[currentRowIndex]);
	double act_flow = avgActFlowdata();
	double ref_flow = avgRefFlowdata();
    double error = ((act_flow - ref_flow) / ref_flow) * 100 ;
	//-----------------------------------------------------------------------------------
    ostringstream refstream;
    refstream << fixed << setprecision(3) << error;
    string refFlowStr = refstream.str();
    ostringstream actstream;
    actstream << fixed << setprecision(3) << error;
    string actFlowStr = actstream.str();
    ostringstream stream;
    stream << fixed << setprecision(3) << error;
    string errorStr = stream.str();
	//-----------------------------------------------------------------------------------
    if (currentRowIndex < actFlowCells.size()) {
        actFlowCells[currentRowIndex]->SetLabel(refFlowStr);
        refFlowCells[currentRowIndex]->SetLabel(actFlowStr);
        errorCells[currentRowIndex]->SetLabel(errorStr);
    }
	//-----------------------------------------------------------------------------------
	// Clear the buffers for next checkpoint
    BufferactFlowValue.clear();
    BufferrefFlowValue.clear();
    grid->Layout();
    grid->GetContainingWindow()->Layout();
    currentRowIndex++;
}
void AutomateCheckpointDialog::StartUpdatingFlowValues(wxCommandEvent& event) {
    currentRowIndex = 0;
    updateTimer.SetOwner(this,1022);
    updateTimer.Start(1000);  // เรียก event ทุก 1 วินาที
}
void AutomateCheckpointDialog::StopUpdatingFlowValues(wxCommandEvent& event) {
	updateTimer.Stop();
}
//----------------------------------------------------------------------------------------------------------------------
double AutomateCheckpointDialog::avgActFlowdata() {
	double actFlowValue = 0.0;
	double avgActFlow = 0.0;
	while (BufferactFlowValue.size() < 24) {
		actFlowValue = sendAndReceiveBetweenPorts(BLECtx);
		BufferactFlowValue.push_back(actFlowValue);
		FullactBuffer.push_back(actFlowValue);
	}
	for (int i = 0; i < BufferactFlowValue.size(); i++) {
		avgActFlow += BufferactFlowValue[i];
	}
	avgActFlow = avgActFlow / BufferactFlowValue.size();
	return avgActFlow;
}
double AutomateCheckpointDialog::avgRefFlowdata() {
	double refFlowValue = 0.0;
    double avgRefFlow = 0.0;
    while(BufferrefFlowValue.size() < 24) {
        rc = modbus_read_registers(modbusCtx, 6, 2, refFlow);
        if (rc != -1) {
            memcpy(&refFlowValue, refFlow, sizeof(refFlowValue));
			refFlowValue = round(refFlowValue * 1000.0) / 1000.0;
        }
		BufferrefFlowValue.push_back(refFlowValue);
		FullrefBuffer.push_back(refFlowValue);
    }
    for (int i = 0; i < BufferrefFlowValue.size(); i++) {
		avgRefFlow += BufferrefFlowValue[i];
    }
	avgRefFlow = avgRefFlow / BufferrefFlowValue.size();
	return avgRefFlow;
}
//----------------------------------------------------------------------------------------------------------------------
void AutomateCheckpointDialog::ReadrefFlow() {
	rc = modbus_read_registers(modbusCtx, 6, 2, refFlow);
	if (rc != -1) {
		memcpy(&NRefFlow, refFlow, sizeof(NRefFlow));
        NRefFlow = round(NRefFlow * 1000.0) / 1000.0;
	}
}
void AutomateCheckpointDialog::TuneFlowByPID(int setpoint) {
    double error = 0;
    do {
        ReadrefFlow();
        double PID_output = calculatePID(setpoint, NRefFlow);
        set_current(serialCtx, PID_output);
		error = abs(setpoint - NRefFlow);
    } while (error <= 0.1);
}
//----------------------------------------------------------------------------------------------------------------------
void AutomateCheckpointDialog::OnExportLog(wxCommandEvent& event) {
    wxFileDialog saveFileDialog(this, "Save CSV file", "", "",
        "CSV files (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return;
    ExportDataToCSV(saveFileDialog.GetPath());
}
string AutomateCheckpointDialog::GetCurrentTime() {
    auto now = std::chrono::system_clock::now();
    time_t current_time = std::chrono::system_clock::to_time_t(now);
    tm* timeinfo = localtime(&current_time);
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%I:%M:%S", timeinfo);
    string meridiem = (timeinfo->tm_hour >= 12) ? " PM" : " AM";
    return string(buffer) + meridiem;
}
string AutomateCheckpointDialog::GetCurrentDate() {
    auto now = std::chrono::system_clock::now();
    time_t current_time = std::chrono::system_clock::to_time_t(now);
    tm* timeinfo = localtime(&current_time);
    char buffer[12];
    strftime(buffer, sizeof(buffer), "%d %b. %Y", timeinfo);
    return string(buffer);
}
void AutomateCheckpointDialog::ExportDataToCSV(const wxString& filePath) {
    ofstream file(filePath.ToStdString());
    if (!file.is_open()) {
        wxMessageBox("Failed to create CSV file", "Error", wxOK | wxICON_ERROR);
        return;
    }
    // Write header with current time and date
    file << "Time:," << GetCurrentTime() << "," << endl;
    file << "Date:," << GetCurrentDate() << "," << endl;

    // For each checkpoint
    size_t index = 0;
    for (size_t checkPoint = 0; checkPoint < setpoint.size(); checkPoint++) {
        file << "Check Point," << checkPoint + 1 << "," << endl;
        file << "Flow," << setpoint[checkPoint] << " m3/h," << endl;
        file << "Index,Ref. Flow (m3/h),Act. Flow (m3/h)" << endl;

        double sumActFlow = 0.0;
        int count = 0;
        // Write 24 values for each checkpoint
        for (int i = 0; i < 24; i++) {
            if (index < FullactBuffer.size() && index < FullrefBuffer.size()) {
                file << i + 1 << "," << fixed << setprecision(4) << FullrefBuffer[index] << "," << FullactBuffer[index] << endl;
                sumActFlow += FullactBuffer[index];
                count++;
                index++;
            }
        }
        // Calculate and write actual flow average
        if (count > 0) {
            double avgAct = sumActFlow / count;
            file << ",Act. Avg," << fixed << setprecision(4) << avgAct << endl;
        }
    }

    // Clear buffers after exporting
    FullactBuffer.clear();
    FullrefBuffer.clear();
    file.close();
    wxMessageBox("Data exported successfully", "Success", wxOK | wxICON_INFORMATION);
}
//----------------------------------------------------------------------------------------------------------------------
wxBEGIN_EVENT_TABLE(AutomateCheckpointDialog, wxDialog)
    EVT_BUTTON(1020, AutomateCheckpointDialog::StartUpdatingFlowValues)
    EVT_BUTTON(1021, AutomateCheckpointDialog::OnLoadFile)
	EVT_BUTTON(1023, AutomateCheckpointDialog::OnExportLog)
	EVT_BUTTON(1024, AutomateCheckpointDialog::StopUpdatingFlowValues)
    EVT_TIMER(1022, AutomateCheckpointDialog::OnUpdateFlowTimer)
END_EVENT_TABLE()
