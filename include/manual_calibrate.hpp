#ifndef MANUAL_CALIBRATE_HPP
#define MANUAL_CALIBRATE_HPP

#include <wx/wx.h>
#include <wx/app.h> 
#include <wx/event.h>
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

#ifdef _WIN32
#include <windows.h>
#endif // 


using namespace std;
using namespace boost::asio;

std::string formatTimestamp(long milliseconds);


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
	//------------------------------------------------------------------------------------------------  
	void calculatePIDWorker();
    double calculatePID(double setpointValue, double currentValue);
	//------------------------------------------------------------------------------------------------
	void StartThread();
    void StopThread();
    //------------------------------------------------------------------------------------------------
    bool CheckAndLoadPorts(const string& fileName, vector<string>& ports);
    tuple<string, string, string> ReadPortsFromFile(const string& fileName);
    //------------------------------------------------------------------------------------------------
    void OnReadTimer();
    void OnDisplayTimer();
    //------------------------------------------------------------------------------------------------
    serial_port InitialSerial(io_service& io, const string& port_name ,unsigned int baudrate);
    modbus_t* InitialModbus(const char* modbus_port);
	//------------------------------------------------------------------------------------------------
    static const size_t DUMP_THRESHOLD = 500;
    std::string dumpFilePath;
    size_t totalEntriesWritten = 0;
    void dumpDataToFile();


private:
    wxDECLARE_EVENT_TABLE();
	//------------------------------------------------------------------------------------------------
    std::chrono::steady_clock::time_point initialTime;
	//------------------------------------------------------------------------------------------------
    struct DataEntry {
        float Flow;
    };
    
    boost::lockfree::queue<DataEntry> refFlowBuffer{ 10240 }; // กำหนดขนาด 1024
    boost::lockfree::queue<DataEntry> actFlowBuffer{ 10240 }; // กำหนดขนาด 1024
    //------------------------------------------------------------------------------------------------
    void clearQueue(boost::lockfree::queue<DataEntry>& queue) {
        DataEntry dummy;
        while (queue.pop(dummy)) {
            // ลบค่าทีละตัวจนกว่าคิวจะว่าง
        }
    }
	//------------------------------------------------------------------------------------------------
    float refFlowValue = 0.0 ;
    float actFlowValue = 0.0 ;
    float errorValue_percentage = 0.0  ;
	//------------------------------------------------------------------------------------------------
    deque<float> setpointData, actData, errorData, refData;
    deque<long> pushTimestamps; // Vector to store push timestamps
	//------------------------------------------------------------------------------------------------
    thread modbusThread;
    thread bluetoothThread;
    thread readTimerThread;
	thread displayTimerThread;
    thread pidCalculationThread; 
    thread clearData;   
	//----------------------------------------------------------------------------------------------------------------------------------
    mutex timerMutex;
    mutex dataMutex;
	//----------------------------------------------------------------------------------------------------------------------------------
    condition_variable timerCv;
    //----------------------------------------------------------------------------------------------------------------------------------
    //variable in use in ontime function
    const int MIN_SETPOINT = 0;
    const int MAX_SETPOINT = 50;
    //------------------------------------------------------------------------------------------------
    //const double Kp = 0.007981535232; // for voltage control
    const double Kp = 9.03223474630576e-4; // for current control
    const double Ki = 0.0271542857142857;
    const double Kd = 0.0020869232374223;
    double integral = 0.0;
    double previousError = 0.0;
    double pidOutput = 0.3;
    int setpoint;
	//------------------------------------------------------------------------------------------------
    vector<std::string> timestampData; // เก็บข้อมูล Timestamp
    vector<long> elapsedTimestamps; // เวลาที่ผ่านไปจากการอ่านครั้งแรก (ms)
    vector<long> pushIntervals;    // เก็บระยะห่างระหว่างการ Push (หน่วย: มิลลิวินาที)
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
    wxTextCtrl* setFlowInput;
    wxTextCtrl* refFlowInput;
    wxTextCtrl* actFlowInput;
    wxTextCtrl* errorInput;
	//------------------------------------------------------------------------------------------------
    serial_port serialCtx;
    modbus_t* modbusCtx;
    serial_port BLECtx;
    uint16_t refFlow[4];
    int rc;
	//------------------------------------------------------------------------------------------------
    uint32_t SerialNumber;
	//------------------------------------------------------------------------------------------------
};
#endif // MANUAL_CALIBRATE_HPP
