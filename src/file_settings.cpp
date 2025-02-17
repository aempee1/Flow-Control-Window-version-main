#include "file_settings.hpp"

FileSettingDialog::FileSettingDialog(wxWindow* parent) : wxDialog(parent, wxID_ANY, "Sensor Configuration", wxDefaultPosition, wxSize(500, 400)) {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    // Sensor input
    wxBoxSizer* sensorSizer = new wxBoxSizer(wxHORIZONTAL);
    sensorSizer->Add(new wxStaticText(this, wxID_ANY, "Sensor"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    sensorCtrl = new wxTextCtrl(this, wxID_ANY, "S451");
    sensorSizer->Add(sensorCtrl, 1);
    mainSizer->Add(sensorSizer, 0, wxEXPAND | wxALL, 10);
    // Pipe Diameter input
    wxBoxSizer* diameterSizer = new wxBoxSizer(wxHORIZONTAL);
    diameterSizer->Add(new wxStaticText(this, wxID_ANY, "Pipe Diameter"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    diameterCtrl = new wxTextCtrl(this, wxID_ANY, "54");
    diameterSizer->Add(diameterCtrl, 1);
    diameterSizer->Add(new wxStaticText(this, wxID_ANY, "mm"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    mainSizer->Add(diameterSizer, 0, wxEXPAND | wxALL, 10);
    // Grid for Check Points
    wxStaticText* checkPointLabel = new wxStaticText(this, wxID_ANY, "Check point");
    mainSizer->Add(checkPointLabel, 0, wxLEFT | wxTOP, 10);
    grid = new wxGrid(this, wxID_ANY);
    grid->CreateGrid(10, 2);
    grid->SetColLabelValue(0, "Point");
    grid->SetColLabelValue(1, "Flow (M3/h)");
    for (int i = 0; i < 10; i++) {
        grid->SetCellValue(i, 0, wxString::Format("%d", i + 1));
        grid->SetCellValue(i, 1, wxString::Format("%d", (i < 5 ? (i + 1) * 10 : (i == 5 ? 80 : (i == 6 ? 130 : (i == 7 ? 150 : (i == 8 ? 180 : 200)))))));
    }
    grid->EnableEditing(true);
    grid->SetColFormatNumber(1);
    mainSizer->Add(grid, 1, wxEXPAND | wxALL, 10);
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* saveButton = new wxButton(this, wxID_OK, "Save");
    buttonSizer->Add(saveButton, 0, wxRIGHT, 10);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);
    saveButton->Bind(wxEVT_BUTTON, &FileSettingDialog::OnSave, this);
    SetSizer(mainSizer);
}
void FileSettingDialog::OnSave(wxCommandEvent& event) {
    wxFileDialog saveFileDialog(this, "Save Flow Data", "", "", "Text files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveFileDialog.ShowModal() == wxID_CANCEL) return;
    
    std::ofstream file(saveFileDialog.GetPath().ToStdString());
    if (file.is_open()) {
        file << "Sensor: " << sensorCtrl->GetValue().ToStdString() << "\n";
        file << "Pipe Diameter: " << diameterCtrl->GetValue().ToStdString() << " mm\n";
        file << "Check Points:\n";
        for (int i = 0; i < 10; i++) {
            file << grid->GetCellValue(i, 0).ToStdString() << ", " << grid->GetCellValue(i, 1).ToStdString() << "\n";
        }
        file.close();
    }
    // »Ô´ dialog
    EndModal(wxID_OK);
}