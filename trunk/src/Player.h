
#include <vector>

class Player
{
public:
    Player();
    ~Player();
    void Play(const std::vector<short>& wave, const unsigned int& loopstart, const unsigned int& loopend);
    void Stop();

    bool IsPlaying();

    unsigned int GetPlayPos();
};
