#ifndef MANUAL_CALIBRATE_HPP
#define MANUAL_CALIBRATE_HPP

#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/valnum.h>
#include <wx/datetime.h>
#include <wx/thread.h>
#include <wx/progdlg.h>   // สำหรับ wxProgressDialog
#include "mathplot.h"
#include "modbus_utils.hpp"
#include "serial_utils.hpp"

#include <boost/lockfree/queue.hpp>
#include <boost/thread/thread.hpp>

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


using namespace std;
using namespace boost::asio;

std::string formatTimestamp(long milliseconds);
//----------------------------------------------------------------------------------------------------------------------------------
tuple<string,string,string> ReadPortsFromFile(const string& fileName) ;
//----------------------------------------------------------------------------------------------------------------------------------
class ManualCalibrationDialog : public wxDialog {
public:
    ManualCalibrationDialog(wxWindow *parent);
    ~ManualCalibrationDialog();
    //-------------------------------------------------------------------------------------------------
    void OnStartButtonClick(wxCommandEvent& event);
    void OnStopButtonClick(wxCommandEvent& event);
    void OnDoneButtonClick(wxCommandEvent& event); 
	//-------------------------------------------------------------------------------------------------
    void readModbusWorker();
    void readBluetoothWorker();
    void StartReadTimer();
    void StopReadTimer();
    //void readSensorsWorker();
    //------------------------------------------------------------------------------------------------
    double calculatePID(double setpointValue, double currentValue);
    wxDECLARE_EVENT_TABLE();

private:
    std::chrono::steady_clock::time_point initialTime;
    // Struct for data entries
    struct DataEntry {
        float Flow;
    };
    // variable in use in readValuesWorker function
    float refFlowValue = 0.0;
    float actFlowValue = 0.0;
    float errorValue_percentage = 0.0;

    deque<double> setpointData, actData, errorData, refData;
    deque<long> pushTimestamps; // Vector to store push timestamps

    boost::lockfree::queue<DataEntry> refFlowBuffer{ 10240 }; // กำหนดขนาด 1024
	boost::lockfree::queue<DataEntry> actFlowBuffer{ 10240 }; // กำหนดขนาด 1024
	//------------------------------------------------------------------------------------------------
    thread modbusThread;
    thread bluetoothThread;
    thread readTimerThread;
	thread displayTimerThread;
    thread pidCalculationThread;  // เพิ่ม Thread คำนวณ PID
	//----------------------------------------------------------------------------------------------------------------------------------
    mutex timerMutex;
	//----------------------------------------------------------------------------------------------------------------------------------
    condition_variable timerCv;
    //thread sensorThread;
    //----------------------------------------------------------------------------------------------------------------------------------
    //variable in use in ontime function
    const int MIN_SETPOINT = 0;
    const int MAX_SETPOINT = 50;
    //-----------------------------
    double integral = 0.0;
    double previousError = 0.0;
    double pidOutput = 0.3;
    //------------------------------------------------------------------------------------------------
    // เพิ่มค่าเริ่มต้นของ PID Controller 
    //const double Kp = 0.007981535232; // for voltage control
    const double Kp = 9.03223474630576e-4; // for current control
    const double Ki = 0.0271542857142857;
    const double Kd = 0.0020869232374223;
	//------------------------------------------------------------------------------------------------
    uint16_t refFlow[4];
    int rc;
	//------------------------------------------------------------------------------------------------
    std::mutex dataMutex;
	//------------------------------------------------------------------------------------------------
    std::vector<std::string> timestampData; // เก็บข้อมูล Timestamp
    std::vector<long> elapsedTimestamps; // เวลาที่ผ่านไปจากการอ่านครั้งแรก (ms)
    std::vector<long> pushIntervals;    // เก็บระยะห่างระหว่างการ Push (หน่วย: มิลลิวินาที)
	//------------------------------------------------------------------------------------------------
    wxTimer timerRead;   // Timer สำหรับการอ่านค่า
    wxDateTime initialReadTime; // เวลาเริ่มต้นสำหรับการคำนวณ Elapsed Time
    //------------------------------------------------------------------------------------------------
    // ประกาศตัวแปรของปุ่มที่นี่
    wxButton* startButton;
    wxButton* stopButton;
    wxButton* showGraphButton;
    wxButton* doneButton;
	//------------------------------------------------------------------------------------------------
    bool CheckAndLoadPorts(const string& fileName, vector<string>& ports);
    void OnReadTimer();    // ฟังก์ชันเรียกเมื่อ timerRead ทำงาน
    void OnDisplayTimer();
   	//------------------------------------------------------------------------------------------------
    serial_port serialCtx;
    modbus_t* modbusCtx;
    string BLECtx;
    serial_port InitialSerial(io_service& io, const string& port_name);
    modbus_t* InitialModbus(const char* modbus_port);

protected:
    int setpoint ;
	//------------------------------------------------------------------------------------------------
    wxTextCtrl *setFlowInput;
    wxTextCtrl *refFlowInput;
    wxTextCtrl *actFlowInput;
    wxTextCtrl *errorInput;
	//------------------------------------------------------------------------------------------------
};
#endif // MANUAL_CALIBRATE_HPP
