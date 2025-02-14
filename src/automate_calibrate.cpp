#include "automate_calibrate.hpp"

AutomateCheckpointDialog::AutomateCheckpointDialog(wxWindow* parent) : wxDialog(nullptr, wxID_ANY, "Auto Mate Calibrate Test", wxDefaultPosition, wxSize(700, 500)) {
        wxPanel* panel = new wxPanel(this);
        wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

        // Header
        wxStaticText* title = new wxStaticText(panel, wxID_ANY, "Auto Flow Checkpoint", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        title->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        vbox->Add(title, 0, wxALL | wxALIGN_CENTER, 10);

        wxStaticText* sensorText = new wxStaticText(panel, wxID_ANY, "Sensor : S451");
        sensorText->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        vbox->Add(sensorText, 0, wxLEFT | wxTOP, 10);

        wxStaticText* pipeText = new wxStaticText(panel, wxID_ANY, "Pipe Diameter (mm) : 55");
        pipeText->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        vbox->Add(pipeText, 0, wxLEFT | wxBOTTOM, 10);

        // Display Table
        wxGridSizer* grid = new wxGridSizer(11, 6, 5, 5);

        // Column Headers
        grid->Add(new wxStaticText(panel, wxID_ANY, "Point", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
        grid->Add(new wxStaticText(panel, wxID_ANY, "Flow l/m", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
        grid->Add(new wxStaticText(panel, wxID_ANY, "Act Flow", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
        grid->Add(new wxStaticText(panel, wxID_ANY, "Ref Flow", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
        grid->Add(new wxStaticText(panel, wxID_ANY, "Ref Flow Avg", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
        grid->Add(new wxStaticText(panel, wxID_ANY, "Status", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);

        // Sample Data
        wxString flowData[10][5] = {
            {"1", "10", "9.8", "9.9", "9.8"},
            {"2", "20", "20.1", "20.09", "20"},
            {"3", "30", "", "", ""},
            {"4", "40", "", "", ""},
            {"5", "50", "", "", ""},
            {"6", "60", "", "", ""},
            {"7", "70", "", "", ""},
            {"8", "80", "", "", ""},
            {"9", "90", "", "", ""},
            {"10", "100", "", "", ""}
        };

        for (int i = 0; i < 10; i++) {
            for (int j = 0; j < 5; j++) {
                grid->Add(new wxStaticText(panel, wxID_ANY, flowData[i][j], wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND);
            }

            if (i < 2) {
                wxBitmap checkIcon("check_icon.png", wxBITMAP_TYPE_PNG);
                wxStaticBitmap* status = new wxStaticBitmap(panel, wxID_ANY, checkIcon);
                grid->Add(status, 0, wxEXPAND);
            }
            else {
                wxAnimation loadAnimation("loading.gif");
                wxAnimationCtrl* animCtrl = new wxAnimationCtrl(panel, wxID_ANY, loadAnimation);
                animCtrl->Play();
                grid->Add(animCtrl, 0, wxEXPAND);
            }
        }

        vbox->Add(grid, 1, wxEXPAND | wxALL, 10);

        // Buttons
        wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
        wxButton* exportBtn = new wxButton(panel, wxID_ANY, "Export Log");
        wxButton* cancelBtn = new wxButton(panel, wxID_CANCEL, "Cancel");
        hbox->Add(exportBtn, 0, wxRIGHT, 10);
        hbox->Add(cancelBtn, 0);
        vbox->Add(hbox, 0, wxALIGN_RIGHT | wxALL, 10);

        panel->SetSizer(vbox);
    }
;

