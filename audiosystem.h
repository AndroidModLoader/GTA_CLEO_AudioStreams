#include <set>
#include "SimpleGTA.h"
#include "ibass.h"

class CSoundSystem;
class CAudioStream;
class C3DAudioStream;

extern CSoundSystem* soundsys;
class CSoundSystem
{
    friend class CAudioStream;
    friend class C3DAudioStream;
    std::set<CAudioStream *> streams;
    BASS_INFO SoundDevice;
    bool initialized;
    int forceDevice;
    bool paused;
    bool bUseFPAudio;
public:
    bool Init();
    inline bool Initialized() { return initialized; }
    CSoundSystem() : initialized(false), forceDevice(-1), paused(false), bUseFPAudio(false)
    {
        // TODO: give to user an ability to force a sound device to use (ini-file or cmd-line?)
        // ANDROID: we dont need that ^
    }
    ~CSoundSystem()
    {
        //TRACE("Closing SoundSystem...");
        UnloadAllStreams();
        if (initialized)
        {
            //TRACE("Freeing BASS library");
            //BASS->Free();
            initialized = false;
        }
        //TRACE("SoundSystem closed!");
    }
    CAudioStream * LoadStream(const char *filename, bool in3d = false);
    void PauseStreams();
    void ResumeStreams();
    void UnloadStream(CAudioStream *stream);
    void UnloadAllStreams();
    void Update();
};
class CAudioStream
{
    friend class CSoundSystem;
    CAudioStream(const CAudioStream&);
protected:
    uint64_t streamInternal;
    enum eStreamState
    {
        no,
        playing,
        paused,
        stopped,
    } state;
    bool OK;
    CAudioStream();
public:
    CAudioStream(const char *src);
    virtual ~CAudioStream();
    // actions on streams
    void Play();
    void Pause(bool change_state = true);
    void Stop();
    void Resume();
    uint64_t GetLength();
    int GetState();
    float GetVolume();
    void SetVolume(float val);
    void Loop(bool enable);
	uint64_t GetInternal();
    // overloadable actions
    virtual void Set3DPosition(const CVector& pos);
    virtual void Set3DPosition(float x, float y, float z);
    virtual void Link(CPlaceable* placeable = NULL);
    virtual void Process();
};
class C3DAudioStream : public CAudioStream
{
    friend class CSoundSystem;
    C3DAudioStream(const C3DAudioStream&);
protected:
    CPlaceable*     link;
    BASS_3DVECTOR   position;
public:
    C3DAudioStream(const char *src);
    virtual ~C3DAudioStream();
    // overloaded actions
    virtual void Set3DPosition(const CVector& pos);
    virtual void Set3DPosition(float x, float y, float z);
    virtual void Link(CPlaceable* placeable = NULL);
    virtual void Process();
};

extern IBASS* BASS;
