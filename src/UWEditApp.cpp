/****************************************************************************
* Copyright (C) 2007-2010 by E.Heidt  http://code.google.com/p/uwedit/      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
****************************************************************************/

#include <wx/wx.h>
#include <wx/sysopt.h>
#include <wx/dnd.h>
#include <wx/mstream.h>
#include <wx/file.h>
#include <wx/html/htmlwin.h>
#include <wx/hyperlink.h>

#include "wx/ribbon/bar.h"
#include "wx/ribbon/buttonbar.h"
#include "wx/ribbon/gallery.h"
#include "wx/ribbon/toolbar.h"

#include <vector>
#include "WavePanel.h"

const char* CURRENT_VERSION = "0.9a";
const char* PROJECT_URL = "http://code.google.com/p/uwedit";

void LoadWaveFile(const char* filename, std::vector<short>& data);
bool SendMidiSDS(int midiport, std::vector<short>& data, unsigned int _loopStart, unsigned int _loopEnd, const char* sampleName, int samplePos, void (*progress)(float ));
bool SaveMidiSDS(int (*write)( unsigned char*, int ), void (*progress)(float), const std::vector<short>& data, unsigned int _loopStart, unsigned int _loopEnd, const char* sampleName, int samplePos);
bool LoadMidiSDS(const char* filename, std::vector<short>& data, unsigned int& loopStart, unsigned int& loopEnd, char* sampleName, int& samplePos);
bool SaveMidiSDS(const char* filename, const std::vector<short>& data, unsigned int _loopStart, unsigned int _loopEnd, const char* sampleName, int samplePos);
std::vector<std::string> ListMidiOutDevices();
bool SendDumpRequest(const int waveformId);

enum wxID
{
    wxID_FULLSCREEN = wxID_HIGHEST,
    wxID_SENDSYSEX,
    wxID_PLAYSTART,
    wxID_PLAYSTOP,
    wxID_MIDIOUT,
    wxEVT_UPDATEGAUGE,
    wxEVT_NEWVERSION,
};


bool LoadDataFromResource(char*& t_data, DWORD& t_dataSize, const wxString& t_name)
{
    bool     r_result    = false;
    HGLOBAL  a_resHandle = 0;
    HRSRC    a_resource;

    a_resource = FindResource(0, t_name, RT_RCDATA);

    if(0 != a_resource)
    {
        a_resHandle = LoadResource(NULL, a_resource);
        if (0 != a_resHandle)
        {
            t_data = (char*)LockResource(a_resHandle);
            t_dataSize = SizeofResource(NULL, a_resource);
            r_result = true;
        }
    }

    return r_result;
}


wxBitmap* GetBitmapFromMemory(const char* t_data, const DWORD t_size)
{
    wxMemoryInputStream a_is(t_data, t_size);
    return new wxBitmap(wxImage(a_is, wxBITMAP_TYPE_PNG, -1), -1);
}

wxBitmap& wxPng(const wxString& t_name)
{
    static wxBitmap*   r_bitmapPtr = 0;

    if(r_bitmapPtr)
        delete r_bitmapPtr;

    char*       a_data      = 0;
    DWORD       a_dataSize  = 0;

    if(LoadDataFromResource(a_data, a_dataSize, t_name))
    {
        r_bitmapPtr = GetBitmapFromMemory(a_data, a_dataSize);
    }

    return *r_bitmapPtr;
}

class MainFrame: public wxFrame, public wxThreadHelper
{
    void OnClose(wxCloseEvent& evt)
    {
        if (GetThread() && GetThread()->IsRunning())
            GetThread()->Wait();

        Destroy();
    }

    virtual wxThread::ExitCode Entry()
    {
        SendMidiSDS(m_cbMidiDevice->GetSelection(), m_wavePanel->m_wave, m_wavePanel->m_start, m_wavePanel->m_end, this->GetSampleName(), this->GetSamplePos(), &Progress);
        return (wxThread::ExitCode)0;
    }

    static void Progress(float v)
    {
        if(!m_pInstance->GetThread()->TestDestroy())
        {
            wxThreadEvent* te = new wxThreadEvent(wxEVT_THREAD, wxEVT_UPDATEGAUGE);
            te->SetPayload(v);
            wxQueueEvent(m_pInstance, te);
        }
        else
            m_pInstance->GetThread()->Kill();
    }

    void OnThreadUpdate(wxThreadEvent& evt)
    {
        this->OnProgress(evt.GetPayload<float>());
    }

    void OnNewVersion(wxThreadEvent& evt)
    {
        std::string version = evt.GetPayload<std::string>();

        wxSizer* a = this->GetStatusBar()->GetSizer();

        if(version != CURRENT_VERSION)
        {
            wxString txt = wxString::Format(wxT("New Version %s available"), wxString(version.c_str()));
            wxHyperlinkCtrl* hyperlink = new wxHyperlinkCtrl(this->GetStatusBar(), wxID_ANY, txt, wxT("http://code.google.com/p/uwedit/"));

            a->Add(hyperlink, 0, wxALL|wxEXPAND|wxALIGN_RIGHT, 5);
        }

        a->AddSpacer(10);
    }

    void OnAbout(wxCommandEvent& WXUNUSED(event))
    {
        /*
        wxDialog dlg(this, wxID_ANY, "About");

        wxBoxSizer a(wxVERTICAL);

        wxStaticText label(&dlg, wxID_ANY, wxT("UWEdit 0.9alpha"));
        wxStaticText label2(&dlg, wxID_ANY, wxT("Copyright (C) 2011 by E.Heidt"));

        a.Add(&label, 1, wxALL|wxEXPAND, 10);
        a.Add(&label2, 1, wxALL|wxEXPAND, 10);
        wxHyperlinkCtrl hyperlink(&dlg, wxID_ANY, "http://code.google.com/p/uwedit", wxT("http://code.google.com/p/uwedit/"));
        a.Add(&hyperlink, 1, wxALL|wxEXPAND, 10);
        wxButton btnOK(&dlg, wxID_OK, wxT("OK"));
        a.Add(&btnOK, 1, wxALL|wxEXPAND, 10);

        dlg.SetSizer(&a);
        dlg.ShowModal();
        */

        wxString about;
        about += wxString::Format(L"UWEdit %s\n\n", CURRENT_VERSION);
        about += wxString::Format(L"Copyright (C) 2011 by E.Heidt\n%s\n\n", PROJECT_URL);;
        wxMessageBox( about.c_str(), wxString::Format(L"UWEdit %s\n\n", CURRENT_VERSION) );
    }

    void OnSendSysEx(wxCommandEvent& WXUNUSED(event))
    {
        // we want to start a long task, but we don't want our GUI to block
        // while it's executed, so we use a thread to do it.
        if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR)
        {
            wxLogError("Could not create the worker thread!");
            return;
        }

        // go!
        if (GetThread()->Run() != wxTHREAD_NO_ERROR)
        {
            wxLogError("Could not run the worker thread!");
            return;
        }
    }

    void SaveSDS(const wxString& file)
    {
        SaveMidiSDS(file.ToAscii(), m_wavePanel->m_wave, m_wavePanel->m_start, m_wavePanel->m_end, this->GetSampleName(), this->GetSamplePos());
    }

    void LoadWave(const wxString& file)
    {
        if(wxFile::Exists(file))
        {
            m_wavePanel->PlayStop();

            if(file.EndsWith(wxT(".SYX")) || file.EndsWith(wxT(".syx")))
            {
                char sampleName[] = "----";
                int samplePos = 0;
                LoadMidiSDS(file.ToAscii(), m_wavePanel->m_wave, m_wavePanel->m_start, m_wavePanel->m_end, sampleName, samplePos);
                this->SetSamplePos(samplePos);
                this->SetSampleName(sampleName);
            }
            else
            {
                LoadWaveFile(file.ToAscii(), m_wavePanel->m_wave);
                m_wavePanel->m_start = m_wavePanel->m_end = m_wavePanel->m_wave.size();
                this->SetSamplePos(0);
                this->SetSampleName(wxFileName::FileName(file).GetName());
            }

            for(size_t i = 0; i < m_wavePanel->m_wave.size(); i++)
                m_wavePanel->m_wave[i] = m_wavePanel->m_wave[i] & 0xFFF0;   //12Bit

            m_wavePanel->Refresh();
        }
    }

    void Stop(wxCommandEvent& WXUNUSED(event))
    {
        m_wavePanel->PlayStop();
    }

    void Play(wxCommandEvent& WXUNUSED(event))
    {
        m_wavePanel->Play();
    }

    static MainFrame* m_pInstance;

    wxString GetSampleName()
    {
        return m_pTextCtrl->GetValue();
    }

    int GetSamplePos()
    {
        return m_cbSampleNum->GetSelection();
    }

    void SetSampleName(const char* name)
    {
        if(m_pTextCtrl)
            m_pTextCtrl->SetValue(wxString(name));
    }

    void SetSamplePos(int pos)
    {
        if(m_cbSampleNum)
            m_cbSampleNum->SetSelection(pos);
    }

    WavePanel* m_wavePanel;
    wxTextCtrl* m_pTextCtrl;
    wxComboBox* m_cbMidiDevice;
    wxComboBox* m_cbSampleNum;
    wxRibbonBar* m_ribbon;
    wxRibbonPage* m_ribbonHome;
    wxGauge* m_pGauge;
    wxRibbonToolBar* m_playToolbar;

public:

    MainFrame():
        wxFrame(NULL, wxID_ANY, wxT("UWEdit"), wxDefaultPosition, wxSize(640, 480)), m_wavePanel(new WavePanel(this)),m_pTextCtrl(NULL),m_cbSampleNum(NULL)
    {

        m_pInstance = this;

        this->SetIcon( wxIcon(wxT("APP_ICO")) );

        wxSystemOptions::SetOption(wxT("msw.remap"), 2);

        class DropTarget: public wxFileDropTarget
        {
            virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
            {
                MainFrame::m_pInstance->LoadWave( filenames[0] );
                return true;
            }
        };

        wxWindow::SetDropTarget(new DropTarget());

        Connect( wxID_OPEN, wxEVT_COMMAND_RIBBONTOOL_CLICKED, wxCommandEventHandler(MainFrame::OnOpenFile));
        Connect( wxID_SAVE, wxEVT_COMMAND_RIBBONTOOL_CLICKED, wxCommandEventHandler(MainFrame::OnSaveFile));
        Connect( wxID_PLAYSTART, wxEVT_COMMAND_RIBBONTOOL_CLICKED, wxCommandEventHandler(MainFrame::Play));
        Connect( wxID_PLAYSTOP, wxEVT_COMMAND_RIBBONTOOL_CLICKED, wxCommandEventHandler(MainFrame::Stop));
        Connect( wxID_SENDSYSEX, wxEVT_COMMAND_RIBBONTOOL_CLICKED, wxCommandEventHandler(MainFrame::OnSendSysEx));
        Connect( wxID_ABOUT, wxEVT_COMMAND_RIBBONTOOL_CLICKED, wxCommandEventHandler(MainFrame::OnAbout));

        //Ribbon

        m_ribbon = new wxRibbonBar(this, wxID_ANY); //, wxDefaultPosition, wxDefaultSize, wxRIBBON_BAR_DEFAULT_STYLE|wxRIBBON_BAR_ALWAYS_SHOW_TABS);

        m_ribbonHome = new wxRibbonPage(m_ribbon, wxID_ANY, wxT("Home"), wxNullBitmap);

        if(wxRibbonPanel *rpanel = new wxRibbonPanel(m_ribbonHome, wxID_ANY, wxT("File"), wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_NO_AUTO_MINIMISE))
        {
            //wxRibbonButtonBar *toolbar = new wxRibbonButtonBar(rpanel, wxID_ANY);
            //toolbar->AddButton(wxID_OPEN, wxT("Open"), wxPng(wxT("FILE_OPEN")));
            //toolbar->AddButton(wxID_SAVE, wxT("Save"), wxPng(wxT("FILE_SAVE")));

            wxRibbonToolBar *toolbar = new wxRibbonToolBar(rpanel, wxID_ANY);
            toolbar->AddTool(wxID_OPEN, wxPng(wxT("FILE_OPEN")));
            toolbar->AddTool(wxID_SAVE, wxPng(wxT("FILE_SAVE")));
        }

        if(wxRibbonPanel *rpanel = new wxRibbonPanel(m_ribbonHome, wxID_ANY, wxT("Play"), wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_NO_AUTO_MINIMISE))
        {
            m_playToolbar = new wxRibbonToolBar(rpanel, wxID_ANY);
            m_playToolbar->AddTool(wxID_PLAYSTART, wxPng(wxT("PLAY_PNG")));
            m_playToolbar->AddTool(wxID_PLAYSTOP, wxPng(wxT("STOP_PNG")));
        }

        if(wxRibbonPanel *rpanel = new wxRibbonPanel(m_ribbonHome, wxID_ANY, wxT("Name"), wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_NO_AUTO_MINIMISE))
        {
            m_pTextCtrl = new wxTextCtrl(rpanel, wxID_ANY, wxT("SNBD"), wxDefaultPosition, wxSize(40,20) );
            wxSizer* a = new wxBoxSizer(wxHORIZONTAL);
            a->Add(m_pTextCtrl, 0, wxALL|wxEXPAND, 1);
            rpanel->SetSizer(a);
        }

        if(wxRibbonPanel *rpanel = new wxRibbonPanel(m_ribbonHome, wxID_ANY, wxT("Position"), wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_NO_AUTO_MINIMISE))
        {
            wxString values[48];
            for(int i = 0; i < 48; i++)
                values[i] = wxString::Format(wxT("%d"), i+1);

            m_cbSampleNum = new wxComboBox(rpanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 48, values, wxCB_READONLY);
            m_cbSampleNum->SetSelection(0);
            wxSizer* a = new wxBoxSizer(wxHORIZONTAL);
            a->Add(m_cbSampleNum, 0, wxALL|wxEXPAND, 1);
            rpanel->SetSizer(a);
        }

        if(wxRibbonPanel *rpanel = new wxRibbonPanel(m_ribbonHome, wxID_ANY, wxT("MidiOut"), wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_NO_AUTO_MINIMISE))
        {

            std::vector<std::string> devices = ListMidiOutDevices();

            std::vector<wxString> values;
            for(size_t i = 0; i < devices.size(); i++)
                values.push_back( devices[i].c_str() );

            if(values.size() == 0)
                values.push_back( wxT("") );

            m_cbMidiDevice = new wxComboBox(rpanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, devices.size(), &values[0], wxCB_READONLY);

            m_cbMidiDevice->SetSelection(0);
            for(size_t i = 0; i < values.size(); i++)
                if(values[i] == "Elektron TM-1")
                    m_cbMidiDevice->SetSelection(i);

            wxSizer* a = new wxBoxSizer(wxHORIZONTAL);
            a->Add(m_cbMidiDevice, 0, wxALL|wxEXPAND, 1);
            rpanel->SetSizer(a);
        }

        if(wxRibbonPanel *rpanel = new wxRibbonPanel(m_ribbonHome, wxID_ANY, wxT("Send"), wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_NO_AUTO_MINIMISE))
        {
            wxRibbonToolBar *toolbar = new wxRibbonToolBar(rpanel, wxID_ANY);
            toolbar->AddTool(wxID_SENDSYSEX, wxPng(wxT("MD_PNG")));
        }

        if(wxRibbonPanel *rpanel = new wxRibbonPanel(m_ribbonHome, wxID_ANY, wxT("About"), wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_NO_AUTO_MINIMISE))
        {
            wxRibbonToolBar *toolbar = new wxRibbonToolBar(rpanel, wxID_ANY);
            toolbar->AddTool(wxID_ABOUT, wxPng(wxT("INFO_PNG")));
        }

        ////////////
        wxRibbonArtProvider* artProvider = new wxRibbonAUIArtProvider();
        wxColor a, b, c;
        artProvider->GetColourScheme(&a, &b, &c);
        artProvider->SetColourScheme(a, *wxLIGHT_GREY, *wxBLACK);
        m_ribbon->SetArtProvider(artProvider);
        //////////////

        m_ribbon->Realize();

        wxSizer *s = new wxBoxSizer(wxVERTICAL);
        s->Add(m_ribbon, 0, wxEXPAND);
        s->Add(m_wavePanel, 1, wxEXPAND);
        this->SetSizer(s);

        //Statusbar
        this->CreateStatusBar();

        m_pGauge = new wxGauge( this->GetStatusBar(), wxID_ANY, 100,
                                wxDefaultPosition, wxDefaultSize,
                                wxNO_BORDER|wxHORIZONTAL|wxGA_SMOOTH);

        wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->AddStretchSpacer(1);
        sizer->Add(m_pGauge, 1, wxALL|wxEXPAND|wxALIGN_RIGHT, 5);
        this->GetStatusBar()->SetSizer(sizer);

        m_pGauge->Hide();

        ////////////////////////////////////////////////////////////////////////

        const int n = 512;

        int last_n = 0;

        for(double f = 2; f < n; f*=1.5f)
        {
            last_n = f;
            for(double i = 0, j = 0, k = 1; i < f; i++)
            {

                short v = sin((M_PI/f)*j*2) * 0x7FFF;
                m_wavePanel->m_wave.push_back(v);
                j+=k;
            }
        }

        m_wavePanel->m_start = m_wavePanel->m_wave.size()-last_n;
        m_wavePanel->m_end = m_wavePanel->m_wave.size();

        class CheckForNewVersionThread: public wxThread
        {
        public:
            CheckForNewVersionThread() : wxThread(wxTHREAD_DETACHED)
            {
                if(wxTHREAD_NO_ERROR == Create())
                {
                    Run();
                }
            }
        protected:
            virtual ExitCode Entry()
            {
                std::string CheckForUpdate(const char* httpUrl, const char* currentVersion);

                wxThreadEvent* te = new wxThreadEvent(wxEVT_THREAD, wxEVT_NEWVERSION);
                te->SetPayload(CheckForUpdate(PROJECT_URL, CURRENT_VERSION));
                wxQueueEvent(MainFrame::m_pInstance, te);

                return static_cast<ExitCode>(NULL);
            }
        };

        new CheckForNewVersionThread();
    }

    virtual ~MainFrame()
    {
    }

private:

    virtual void OnInternalIdle()
    {
        this->GetStatusBar()->Layout();

        if(m_wavePanel->IsPlaying())
            m_wavePanel->Refresh();

        if(GetThread() && GetThread()->IsRunning())
        {
            m_ribbon->Enable(false);
        }
        else
        {
            m_ribbon->Enable(true);
            SetStatusText( wxString::Format(wxT("Sample Length: %d, Loop Start: %d Loop End: %d, Pos: %d"),
                                            m_wavePanel->m_wave.size(), m_wavePanel->m_start, m_wavePanel->m_end, m_wavePanel->m_pos) );
        }


        wxFrame::OnInternalIdle();
    }

    void OnOpenFile(wxCommandEvent& WXUNUSED(event))
    {
        wxString wildcards = wxT("Wave files (*.wav)|*.wav;*.WAV|MIDI Sample Dumps (*.syx)|*.syx;*.SYX");
        wxFileDialog dlgOpenFile( this, wxT("Open.."), wxEmptyString, wxEmptyString, wildcards);

        if( dlgOpenFile.ShowModal() == wxID_OK )
        {
            this->UpdateWindowUI();
            this->LoadWave( dlgOpenFile.GetPath() );
        }
    }

    void OnSaveFile(wxCommandEvent& WXUNUSED(event))
    {
        wxString wildcards = wxT("MIDI Sample Dumps (*.syx)|*.syx;*.SYX");
        wxFileDialog dlgSaveFile( this, wxT("Export.."), wxEmptyString, wxEmptyString, wildcards, wxFD_SAVE );

        if( dlgSaveFile.ShowModal() == wxID_OK)
        {
            this->SaveSDS(dlgSaveFile.GetPath());
        }
    }

    void OnFullscreen(wxCommandEvent& event )
    {
        this->ShowFullScreen( event.IsChecked() );
    }

    void OnProgress(float p)
    {
        if(p == 1.f)
        {
            m_pGauge->Hide();
        }
        else
        {
            m_pGauge->SetValue( (int)(p*100.f));
            m_pGauge->Show();

            SetStatusText( wxString::Format(wxT("Sending Sample ... %d%%"), (int)(p*100.f) ) );
        }
    }

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_THREAD(wxEVT_UPDATEGAUGE, MainFrame::OnThreadUpdate)
EVT_THREAD(wxEVT_NEWVERSION, MainFrame::OnNewVersion)
EVT_CLOSE(MainFrame::OnClose)
wxEND_EVENT_TABLE()

MainFrame* MainFrame::m_pInstance = NULL;

class MyApp : public wxApp
{
public:
    virtual bool OnInit()
    {
        wxImage::AddHandler(new wxPNGHandler);

        MainFrame* pFrame = new MainFrame();
        pFrame->Show(true);

        return true;
    }
};

IMPLEMENT_APP(MyApp)
//IMPLEMENT_APP_CONSOLE(MyApp);

