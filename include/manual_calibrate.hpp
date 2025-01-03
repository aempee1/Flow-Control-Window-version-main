#ifndef MANUAL_CALIBRATE_HPP
#define MANUAL_CALIBRATE_HPP

#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/valnum.h>
#include "mathplot.h"
#include "modbus_utils.hpp"
#include "serial_utils.hpp"
#include <fstream>
#include <string>
#include <tuple>
#include <stdexcept>
#include <iostream>
#include <numeric>
#include <iomanip>


using namespace std;
using namespace boost::asio;

tuple<string, string, string> ReadPortsFromFile(const string& fileName) ;
class ManualCalibrationDialog : public wxDialog {
public:
    ManualCalibrationDialog(wxWindow *parent);
    void OnDoneButtonClick(wxCommandEvent& event); 
    void OnModbusReadTimer(wxTimerEvent& event);
    void OnUpdateDisplayTimer(wxTimerEvent& event);
    ~ManualCalibrationDialog();
    void OnSetButtonClick(wxCommandEvent &event); // ฟังก์ชัน event handler
    double calculatePID(double setpointValue, double currentValue);
    wxDECLARE_EVENT_TABLE();

private:

    mpWindow* graphWindow; // Window for plotting the graph 
	mpFXYVector* setpointLayer; // Graph layer for setpoint data
    mpFXYVector* graphLayer; // Graph layer for X-Y data
    vector<double> xData;
    vector<double> yData;
    
    bool CheckAndLoadPorts(const string& fileName, vector<string>& ports);

    modbus_t* modbusCtx;
    serial_port serialCtx;

    serial_port InitialSerial(io_service& io, const string& port_name);
    modbus_t* InitialModbus(const char* modbus_port);

    void OnTimer(wxTimerEvent& event); // Handler สำหรับ Timer
    wxTimer* timer; // Timer

    void UpdateGraph(float flow); // Function to update graph data

protected:

    int setpoint ;
    wxTextCtrl *setFlowInput;
    wxTextCtrl *refFlowInput;
    wxTextCtrl *actFlowInput;
    wxTextCtrl *errorInput;
    wxTimer* modbusReadTimer; // Timer สำหรับอ่านค่า Modbus
    wxTimer* updateDisplayTimer; // Timer สำหรับแสดงผล
    std::deque<float> valueBuffer; // Buffer สำหรับเก็บค่า Modbus
    const int BUFFER_SIZE = 10; // เก็บค่าล่าสุด 10 ค่า (50 ms x 10 = 500 ms)

};

#endif // MANUAL_CALIBRATE_HPP
