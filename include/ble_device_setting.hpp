#ifndef BLE_DEVICE_SETTING_HPP
#define BLE_DEVICE_SETTING_HPP

#include <wx/wx.h>
#include <wx/aboutdlg.h>  // ����Ѻ wxAboutDialogInfo
#include <wx/button.h>
#include <wx/image.h> // ��������Ѻ wxInitAllImageHandlers()
#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <mutex>


class BLEDeviceSettingsDialog : public wxDialog {
public:
	BLEDeviceSettingsDialog(wxWindow* parent);
    ~BLEDeviceSettingsDialog() = default;

private:
    void fetchBLEDevices();  // �ѧ��ѹ�����֧������ BLE devices
    void onTimer(wxTimerEvent& event);  // �ѧ��ѹ������¡��ҹ�ء� 1 �Թҷ�
	void initBLEWatcher();  // �ѧ��ѹ�����������鹡���᡹ BLE devices

    std::mutex m_mutex;
    std::vector<std::tuple<std::string, std::string, std::string, int>> m_devices;
    winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher m_bleWatcher;
    std::unordered_map<std::string, std::tuple<std::string, std::string, std::string, int>> uniqueDevices;

    wxListCtrl* m_deviceList;  // wxListCtrl ����Ѻ�ʴ� BLE devices
    wxTimer m_timer;           // ��ǨѺ��������Ѻ���¡�ѧ��ѹ�ء� 1 �Թҷ�
    wxDECLARE_EVENT_TABLE();

};
#endif // BLE_DEVICE_SETTINGS_HPP