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

#include "WavePanel.h"

enum wxID
{
    wxID_FULLSCREEN = wxID_HIGHEST +100,
    wxID_LOOPSTART,
    wxID_LOOPEND,
    wxID_LOOPENTIRE,
    wxID_LOOPREMOVE,
};

unsigned int WavePanel::ScreenXToWorld(double x, double screenDX)
{
    return (m_offset/m_zoom) + ((x/screenDX) * (m_wave.size()/m_zoom));    //Todo: Optimize
}

int WavePanel::WorldXToScreen(double wx, double screenDX)
{
    return ((wx-(m_offset/m_zoom)) / (m_wave.size()/m_zoom)) * screenDX;    //Todo: Optimize
}

void WavePanel::DrawWaveValueAt(wxDC& dc, double x, double dx, double dy)
{
    unsigned int pos = ScreenXToWorld(x, dx);
    unsigned int pos2 = ScreenXToWorld(x+1, dx);

    short min = m_wave[pos];
    short max = m_wave[pos];

    for(; pos <= pos2; pos++)
    {
        min = std::min(min, m_wave[pos]);
        max = std::max(max, m_wave[pos]);
    }

    double fmin = (((((double)min) / 0xFFFF) * 0.9)*dy) + (dy/2);
    double fmax = (((((double)max) / 0xFFFF) * 0.9)*dy) + (dy/2);

    if(fmin>fmax)
        std::swap(fmin, fmax);

    dc.DrawLine(x, fmin, x+1, fmax);
    dc.DrawLine(x, fmax, x+1, fmin);
}

void WavePanel::DrawWave(wxDC& dc, double dx, double dy)
{
    dc.SetPen(*wxGREEN_PEN);
    dc.SetBrush(wxBrush(*wxGREEN));

    for(double i = 0; i < dx-1; i+=1)
    {
        DrawWaveValueAt(dc, i, dx, dy);
    }
}

WavePanel::WavePanel(wxWindow* parent):wxPanel(parent)
{
    m_offset = 0;
    m_lastPos = m_pos = 0;
    m_start = 0;
    m_end = 0;
    m_lastZoom = m_zoom = 1;

    Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( WavePanel::OnEraseBG ));
    Connect(wxEVT_SIZE, wxSizeEventHandler( WavePanel::OnSize ));
    Connect(wxEVT_PAINT, wxPaintEventHandler( WavePanel::OnPaint ));

    Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(WavePanel::OnMouseEvent));
    Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(WavePanel::OnMouseEvent));
    Connect(wxEVT_LEFT_UP, wxMouseEventHandler(WavePanel::OnMouseEvent));
    Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(WavePanel::OnMouseEvent));
    Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(WavePanel::OnMouseEvent));
    Connect(wxEVT_MOTION, wxMouseEventHandler(WavePanel::OnMouseEvent));
    Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(WavePanel::OnMouseEvent));
    Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(WavePanel::OnKeyEvent));
}

void WavePanel::OnPaint(wxPaintEvent& event)
{
    wxMemoryDC dc;
    dc.SelectObject(m_backBuffer);

    dc.SetBackground(*wxBLACK_BRUSH);

    dc.Clear();

    if(m_wave.size() > 0)
    {
        double dx = GetClientSize().x;
        double dy = GetClientSize().y;

        dc.SetPen(*wxGREY_PEN);

        dc.SetBrush(wxBrush(wxColor(60,60,60), wxBRUSHSTYLE_SOLID));
        dc.DrawRectangle( WorldXToScreen(m_start,dx), 0, WorldXToScreen(m_end, dx)-WorldXToScreen(m_start,dx), dy);

        dc.SetPen(*wxWHITE_PEN);
        dc.DrawLine(0,(dy/2), dx,(dy/2));

        DrawWave(dc, dx, dy);

        dc.SetPen(*wxRED_PEN);
        int pos = WorldXToScreen(m_pos, dx);
        dc.DrawLine(pos,0, pos, dy);

        dc.SetPen(*wxYELLOW_PEN);
        pos = WorldXToScreen(m_start, dx);
        dc.DrawLine(pos,0, pos, dy);

        dc.SetPen(*wxYELLOW_PEN);
        pos = WorldXToScreen(m_end, dx);
        dc.DrawLine(pos,0, pos, dy);

        dc.SetPen(*wxWHITE_PEN);
        pos = WorldXToScreen(_player.GetPlayPos(), dx);
        dc.DrawLine(pos,0, pos, dy);
    }

    dc.SelectObject(wxNullBitmap);
    wxPaintDC cdc(this);
    cdc.DrawBitmap(m_backBuffer, 0, 0);

    event.Skip();
}

void WavePanel::OnEraseBG(wxEraseEvent& event)
{
}

void WavePanel::OnSize(wxSizeEvent& event)
{
    m_backBuffer.Create(GetClientSize().x, GetClientSize().y);
    Refresh();
}

void WavePanel::OnMouseEvent(wxMouseEvent& event)
{
    this->SetFocus();
    this->SetCursor(wxCURSOR_ARROW);

    if(event.LeftDClick())
    {
        m_zoom = 1;
        return;
    }

    if (event.GetWheelRotation())
    {
        float zoom = (event.GetWheelRotation()<0) ? 0.9 : 1.1;


        float offScreen = (( (m_wave.size() * m_zoom * zoom) - m_wave.size())-
                           ((m_wave.size() * m_zoom) - m_wave.size()));

        m_offset += (((float)m_pos) / m_wave.size()) * offScreen;

        m_zoom *= zoom;
        m_zoom = std::max(m_zoom, 1.f);
    }

    double d = m_wave.size() /200 / m_zoom;
    bool startHitted = (m_lastPos < m_start + d) && (m_lastPos > m_start - d) && m_start != (unsigned int)-1;
    bool endHitted = (m_lastPos < m_end + d) && (m_lastPos > m_end - d) && m_end != (unsigned int)-1;

    if(startHitted || endHitted)
        this->SetCursor(wxCURSOR_SIZEWE);

    if (event.LeftIsDown() && !startHitted && !endHitted)
    {
        this->SetCursor(wxCURSOR_SIZING);
        m_offset += ((((float)m_lastX-event.GetX()) / GetSize().x) * m_wave.size());
    }
    else if (event.RightIsDown())
    {
        OnContext(event);
    }

    m_offset = std::max(0.f, m_offset);

    float maxOffset = (m_wave.size()*m_zoom) - m_wave.size();

    m_offset = std::min(maxOffset,m_offset);

    m_pos = m_offset + ((((float)event.GetX()) / GetClientSize().x) * m_wave.size());
    m_pos /= m_zoom;

    m_pos = std::max(0u, m_pos);
    m_pos = std::min(m_wave.size()-1,(size_t)m_pos);

    if(event.LeftIsDown())
    {
        if(startHitted)
            SetLoop(m_pos, true);
        else if(endHitted)
            SetLoop(m_pos, false);
    }

    m_lastX = event.GetX();
    m_lastZoom = m_zoom;
    m_lastPos = m_pos;

    Refresh();
}

void WavePanel::SetLoop(unsigned int pos, bool start)
{
    pos = std::max(pos, 0u);
    pos = std::min((size_t)pos, m_wave.size());

    if(start)
    {
        if(pos > m_end)
            m_end = m_pos;
        else
            m_start = pos;
    }
    else
    {
        if(pos < m_start)
            m_start = pos;
        else
            m_end = pos;
    }
}

void WavePanel::OnPopupClick(wxCommandEvent &evt)
{
    //void *data=static_cast<wxMenu *>(evt.GetEventObject())->GetClientData();
    switch(evt.GetId())
    {
    case wxID_LOOPSTART:
        SetLoop(m_pos, true);
        break;
    case wxID_LOOPEND:
        SetLoop(m_pos, false);
        break;
    case wxID_LOOPENTIRE:
        SetLoop(0, true);
        SetLoop(m_wave.size(), false);
        break;
    case wxID_LOOPREMOVE:
        m_start = m_end = m_wave.size();
        break;
    }
}

void WavePanel::OnContext(wxMouseEvent& event)
{
    wxMenu menu;
    wxPoint point = event.GetPosition();

    menu.Append(wxID_LOOPSTART, wxT("Set Loop Start at current Position"), wxT(""));
    menu.Append(wxID_LOOPEND, wxT("Set Loop End at current Position"), wxT(""));
    menu.Append(wxID_LOOPENTIRE, wxT("Loop entire Sample"), wxT(""));
    menu.Append(wxID_LOOPREMOVE, wxT("Remove Loop Points"), wxT(""));

    menu.Connect(wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&WavePanel::OnPopupClick, NULL, this);

    PopupMenu(&menu, point.x, point.y);
}

void WavePanel::OnKeyEvent(wxKeyEvent& event)
{
    if(event.GetKeyCode() == WXK_SPACE )
    {
        this->_player.Play(m_wave, m_start, m_end);
        this->Refresh();
    }
}
