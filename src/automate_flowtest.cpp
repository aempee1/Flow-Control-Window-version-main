#include "automate_flowtest.hpp"
AutomateCheckpointDialog::AutomateCheckpointDialog(wxWindow* parent) 
    : wxDialog(parent, wxID_ANY, "Auto Mate Calibrate Test", wxDefaultPosition, wxSize(700, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), setpoint(0), modbusCtx(nullptr), serialCtx(io_serial), BLECtx(io_ble) {
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    // กำหนดค่า grid
    grid = new wxGridSizer(11, 5, 5, 5);
    // Header
    wxStaticText* title = new wxStaticText(panel, wxID_ANY, "Auto Flow Checkpoint", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
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
    grid->Add(new wxStaticText(panel, wxID_ANY, "Status", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
    vbox->Add(grid, 1, wxEXPAND | wxALL, 10);
    // ปุ่ม
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

    wxButton* loadFileBtn = new wxButton(panel, wxID_ANY, "Load File");
    loadFileBtn->Bind(wxEVT_BUTTON, &AutomateCheckpointDialog::OnLoadFile, this);

    wxButton* exportBtn = new wxButton(panel, wxID_ANY, "Export Log");
    wxButton* startBtn = new wxButton(panel, wxID_ANY, "Start");
    wxButton* stopBtn = new wxButton(panel, wxID_ANY, "Stop");
    wxButton* cancelBtn = new wxButton(panel, wxID_CANCEL, "Cancel");

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
void AutomateCheckpointDialog::LoadDataFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        wxMessageBox("Failed to open file", "Error", wxOK | wxICON_ERROR, this);
        return;
    }

    std::string line;
    std::string sensor;
    std::string pipeDiameter;
    std::vector<std::pair<int, int>> checkPoints;

    while (std::getline(file, line)) {
        if (line.find("Sensor:") == 0) {
            sensor = line.substr(8);
        }
        else if (line.find("Pipe Diameter:") == 0) {
            pipeDiameter = line.substr(14);
        }
        else if (isdigit(line[0])) {
            std::istringstream iss(line);
            int point, value;
            char comma;
            if (iss >> point >> comma >> value) {
                checkPoints.emplace_back(point, value);
            }
        }
    }

    file.close();

    // อัปเดต UI
    sensorText->SetLabel("Sensor: " + sensor);
    pipeText->SetLabel("Pipe Diameter: " + pipeDiameter);

    // ตรวจสอบว่า grid มีอยู่แล้วหรือไม่
    if (!grid || checkPoints.empty()) return;

    // ? 1. ลบ widget เก่าออกจาก `wxGridSizer`
    grid->Clear(true);

    // ? 2. ใส่ Header กลับเข้าไปใหม่
    grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "Point", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
    grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "Flow l/m", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
    grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "Act Flow", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
    grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "Ref Flow", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
    grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "Status", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);

    // ? 3. เติมข้อมูล Point และ Flow ลงไป
    for (const auto& [point, flow] : checkPoints) {
        wxStaticText* pointText = new wxStaticText(grid->GetContainingWindow(), wxID_ANY, std::to_string(point), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        wxStaticText* flowText = new wxStaticText(grid->GetContainingWindow(), wxID_ANY, std::to_string(flow), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

        grid->Add(pointText, 0, wxEXPAND);
        grid->Add(flowText, 0, wxEXPAND);
        grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
        grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
        grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
    }
    // ? 4. รีเฟรช UI ให้แสดงค่าที่เพิ่มเข้าไป
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