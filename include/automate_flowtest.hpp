#ifndef AUTOMATE_FLOWTEST_HPP
#define AUTOMATE_FLOWTEST_HPP

#include <wx/wx.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/animate.h>
#include <wx/filedlg.h>
#include <wx/statline.h>


#include "serial_utils.hpp"
#include "modbus_utils.hpp"

#include <sys/stat.h>
#include <atomic>
#include <fstream>
#include <string>
#include <tuple>
#include <stdexcept>
#include <iostream>
#include <numeric>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <thread>
#include <mutex>
#include <algorithm> 
#include <queue>
#include <optional>
#include <condition_variable>
#include <cmath>
#include <stop_token>

using namespace std;
using namespace boost::asio;

class AutomateCheckpointDialog : public wxDialog {
public:
    AutomateCheckpointDialog(wxWindow* parent);
    ~AutomateCheckpointDialog();
    double calculatePID(double setpointValue, double currentValue);
    //--------------------------------------------------------------------------------
    vector<wxStaticText*> actFlowCells;
    vector<wxStaticText*> refFlowCells;
    //--------------------------------------------------------------------------------
    serial_port InitialSerial(io_service& io, const string& port_name, unsigned int baudrate);
    modbus_t* InitialModbus(const char* modbus_port);
    tuple<string, string, string> ReadPortsFromFile(const string& fileName);
    bool CheckAndLoadPorts(const string& fileName, vector<string>& ports);
    //---------------------------------------------------------------------------------
    void LoadDataFromFile(const std::string& filePath);
    void OnLoadFile(wxCommandEvent& event);
    io_service io_serial;
    io_service io_ble;
    //----------------------------------------------------------------------------------
    const double Kp = 9.03223474630576e-4; // for current control
    const double Ki = 0.0271542857142857;
    const double Kd = 0.0020869232374223;
    double integral = 0.0;
    double pidOutput = 0.3;
    double previousError = 0.0;
    //------------------------------------------------------------------------------------
    void OnUpdateFlowTimer(wxTimerEvent& event);
    void StartUpdatingFlowValues(wxCommandEvent& event);
    wxTimer updateTimer;
    int currentRowIndex = 0;
    //------------------------------------------------------------------------------------
    wxDECLARE_EVENT_TABLE();
    //------------------------------------------------------------------------------------
private:
    //-----------------------------------------------------------------------------------
    serial_port serialCtx;
    modbus_t* modbusCtx;
    serial_port BLECtx;
    uint16_t refFlow[4];
    int rc;
    //------------------------------------------------------------------------------------
    wxStaticText* sensorText;
    wxStaticText* pipeText;
    wxGridSizer* grid;
    wxString flowData[10][5];
};

#endif // MAINWINDOW_PROPERTIES_HPP