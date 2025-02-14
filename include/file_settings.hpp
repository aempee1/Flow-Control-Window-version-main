#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/filedlg.h>
#include <fstream>

class FileSettingDialog : public wxDialog {
    public:
    FileSettingDialog(wxWindow *parent);

    private:
        wxGrid* grid;
        wxTextCtrl* sensorCtrl;
        wxTextCtrl* diameterCtrl;
        void OnSave(wxCommandEvent& event);
    };
    