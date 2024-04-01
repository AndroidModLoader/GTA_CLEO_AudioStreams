#include <set>
#include "SimpleGTA.h"
#include "ibass.h"

class CSoundSystem;
class CAudioStream;
class C3DAudioStream;


enum eStreamType
{
    None = 0,
    SoundEffect,
    Music,
};
enum eStreamState
{
    Stopped = -1,
    PlayingInactive,
    Playing,
    Paused,
};

extern CSoundSystem* soundsys;
class CSoundSystem
{
    friend class CAudioStream;
    friend class C3DAudioStream;
    std::set<CAudioStream *> streams;
    BASS_INFO SoundDevice;
    int forceDevice;
    bool initialized;
    bool paused;
    bool bUseFPAudio;
    static float masterSpeed;
    static float masterVolumeSfx;
    static float masterVolumeMusic;
public:
    bool Init();
    inline bool Initialized() { return initialized; }
    CSoundSystem() : initialized(false), forceDevice(-1), paused(false), bUseFPAudio(false) { }
    ~CSoundSystem()
    {
        UnloadAllStreams();
        initialized = false;
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
    uint32_t streamInternal;
    eStreamState state;
    bool OK;
    eStreamType type;
    float rate = 44100.0f; // file's sampling rate
    double speed = 1.0f;
    double volume = 1.0f;
    // transitions
    double volumeTarget = 1.0f;
    double volumeTransitionStep = 1.0f;
    double speedTarget = 1.0f;
    double speedTransitionStep = 1.0f;

    CAudioStream();

public:
    CAudioStream(const char *src);
    virtual ~CAudioStream();
    // actions on streams
    void Play();
    void Pause(bool change_state = true);
    void Stop();
    void Resume();
    float GetLength();
    int GetState();
    void SetLooping(bool enable);
    bool GetLooping();
	uint32_t GetInternal();
    void UpdateSpeed();
    float GetSpeed();
    void SetSpeed(float value, float transitionTime = 0.0f);
    void UpdateVolume();
    float GetVolume();
    void SetVolume(float value, float transitionTime = 0.0f);
    void SetProgress(float value);
    float GetProgress();
    void SetType(int newtype);
    eStreamType GetType();

public:
    // overloadable actions
    virtual bool Is3DSource();
    virtual void Set3DPosition(const CVector& pos);
    virtual void Set3DPosition(float x, float y, float z);
    virtual void Set3DRadius(float radius);
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
    void UpdatePosition();
    virtual ~C3DAudioStream();
    // overloaded actions
    virtual bool Is3DSource();
    virtual void Set3DPosition(const CVector& pos);
    virtual void Set3DPosition(float x, float y, float z);
    virtual void Set3DRadius(float radius);
    virtual void Link(CPlaceable* placeable = NULL);
    virtual void Process();
};

extern IBASS* BASS;