#ifndef MAINWINDOW_PROPERTIES_HPP
#define MAINWINDOW_PROPERTIES_HPP

#include <wx/wx.h>
#include <wx/aboutdlg.h>  // สำหรับ wxAboutDialogInfo
#include <wx/button.h>
#include <wx/image.h> // จำเป็นสำหรับ wxInitAllImageHandlers()
#include <wx/sizer.h>
#include "manual_calibrate.hpp"
#include "comport_setting.hpp"
#include "ble_device_setting.hpp"

// สร้างคลาสหลักสำหรับหน้าต่าง
class MyFrame : public wxFrame {
public:
    MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size);

private:
    void SetupMainMenu();                // ฟังก์ชันสำหรับสร้างเมนู
    void OnAboutSoftware(wxCommandEvent& event); // ฟังก์ชันสำหรับจัดการคลิกเมนู About
    void OnComportSettings(wxCommandEvent& event); // ฟังก์ชันสำหรับจัดการคลิกเมนู Comport Settings
    void OnManualFlowsystem(wxCommandEvent& event); //
	void OnBLEDeviceSettings(wxCommandEvent& event); // ฟังก์ชันสำหรับจัดการคลิกเมนู BLEDevice Settings
    wxDECLARE_EVENT_TABLE();             // ประกาศ Event Table
};


#endif // MAINWINDOW_PROPERTIES_HPP