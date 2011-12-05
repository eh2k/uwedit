
#include <vector>
#include"wx/wx.h"
#include <BandedWG.h>

using namespace stk;

void Synthesize(std::vector<short>& wave)
{
    try
    {
        stk::Stk::setRawwavePath( "rawwaves\\" );

        BandedWG bell;

       // bell.setPreset(3);
        bell.noteOn(100,20);

        for(int i = 0; i < 3000; i++)
        {
            if(i==1000)
                bell.noteOff(0);
            wave.push_back( (short)(bell.tick()*0x7FFF));
        }
    }
    catch(StkError err)
    {
        wxMessageBox(err.getMessageCString());
    }

}
