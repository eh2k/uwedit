#include <wx/wx.h>
#include <vector>
#include "Player.h"

class WavePanel: public wxPanel
{
private:
    Player _player;

    wxBitmap m_backBuffer;
    float m_lastZoom;
    wxCoord m_lastX;
    unsigned int m_lastPos;
    float m_zoom;
    float m_offset;

    unsigned int ScreenXToWorld(double x, double screenDX);
    int WorldXToScreen(double wx, double screenDX);
    void DrawWaveValueAt(wxDC& dc, double x, double dx, double dy);
    void DrawWave(wxDC& dc, double dx, double dy);

    void OnPaint(wxPaintEvent& event);
    void OnEraseBG(wxEraseEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnMouseEvent(wxMouseEvent& event);
    void SetLoop(unsigned int pos, bool start);
    void OnPopupClick(wxCommandEvent &evt);
    void OnContext(wxMouseEvent& event);
    void OnKeyEvent(wxKeyEvent& event);
public:

    std::vector<short> m_wave;
    unsigned int m_start;
    unsigned int m_end;
    unsigned int m_pos;

    void Play()
    {
        _player.Play(m_wave, m_start, m_end);
        this->Refresh();
    }

    void PlayStop()
    {
        _player.Stop();
    }

    bool IsPlaying()
    {
        return _player.IsPlaying();
    }

    WavePanel(wxWindow* parent);
};
