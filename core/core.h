int LoadWaveFile(const char *filename, short *data, size_t size);
void SaveWaveFile(const char *filename, const short *data, size_t size);

void Player_Stop();
void Player_Play(int deviceId, const short *wave, size_t len, int *playpos, const int *loopstart, const int *loopend);
const char *GetAudioDevice(int i);

int SaveMidiSDS(const char *filename, const short *wave, size_t len, unsigned int loopStart, unsigned int loopEnd, const char *sampleName, int samplePos);
size_t LoadMidiSDS(const char *filename, short *dst, unsigned int *loopStart, unsigned int *loopEnd, char *sampleName, int *samplePos);

const char *GetMidiOutDevice(int i);
void SendMidiSDS(int delay_ms, const short *wave, size_t len, unsigned int loopStart, unsigned int loopEnd, const char *sampleName, int samplePos);

void MidiOut_openPort(int port);
void MidiIn_openPort(int port);
void MidiOut_closePort();
void MidiIn_closePort();
void MidiOut_sendMessage(const unsigned char *rawBytes, size_t len);
void MidiIn_setCallback(/* goMidiInCallback */);