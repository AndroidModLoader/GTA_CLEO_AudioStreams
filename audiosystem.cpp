#include <algorithm>
#include <string>

#include "audiosystem.h"
#include "mod/amlmod.h"
#include "mod/logger.h"

#include "cleo.h"
extern cleo_ifs_t* cleo;



BASS_3DVECTOR pos(0, 0, 0), vel(0, 0, 0), front(0, -1.0, 0), top(0, 0, 1.0);
BASS_3DVECTOR bass_frontVec(0.0f, 0.0f, 0.0f), bass_topVec(0.0f, 0.0f, 0.0f);

std::string sGameRoot;
float CSoundSystem::masterSpeed = 1.0f;
float CSoundSystem::masterVolumeSfx = 1.0f;
float CSoundSystem::masterVolumeMusic = 1.0f;



extern CCamera *camera;
extern bool* userPaused;
extern bool* codePaused;
extern int nGameLoaded;
extern uint32_t* m_snTimeInMillisecondsNonClipped;
extern uint32_t* m_snPreviousTimeInMillisecondsNonClipped;
extern float* ms_fTimeScale;

extern bool (*Get_Just_Switched_Status)(CCamera*);
float GetEffectsVolume();
float GetMusicVolume();



bool CSoundSystem::Init()
{
    sGameRoot = aml->GetAndroidDataPath();
    if (BASS->Set3DFactors(1.0f, 3.0f, 80.0f) && BASS->Set3DPosition(&pos, &vel, &front, &top))
    {
        logger->Info("Initializing SoundSystem...");

        // Can we use floating-point (HQ) audio streams?
        uint32_t floatable; // floating-point channel support? 0 = no, else yes
        if ((floatable = BASS->StreamCreate(44100, 1, BASS_SAMPLE_FLOAT, NULL, NULL)))
        {
            logger->Info("Floating-point audio is supported!");
            bUseFPAudio = true;
            BASS->StreamFree(floatable);
        }
        else
        {
            logger->Info("Floating-point audio is not supported!");
        }

        initialized = true;
        BASS->Apply3D();
        return true;
    }
    logger->Error("Could not initialize SoundSys");
    return false;
}

CAudioStream *CSoundSystem::LoadStream(const char *filename, bool in3d)
{
    CAudioStream *result = in3d ? new C3DAudioStream(filename) : new CAudioStream(filename);
    if (result->OK)
    {
        streams.insert(result);
        return result;
    }
    delete result;
    return NULL;
}

void CSoundSystem::UnloadStream(CAudioStream *stream)
{
    if (streams.erase(stream))
        delete stream;
    else
        logger->Error("Unloading of stream that is not in a list of loaded streams");
}

void CSoundSystem::UnloadAllStreams()
{
    std::for_each(streams.begin(), streams.end(), [](CAudioStream *stream)
    {
        delete stream;
    });
    streams.clear();
}

void CSoundSystem::ResumeStreams()
{
    paused = false;
    std::for_each(streams.begin(), streams.end(), [](CAudioStream *stream) {
        if (stream->state == eStreamState::Playing) stream->Resume();
    });
}

void CSoundSystem::PauseStreams()
{
    paused = true;
    std::for_each(streams.begin(), streams.end(), [](CAudioStream *stream) {
        stream->Pause(false);
    });
}

inline bool CameraJustRestored()
{
    if(nGameLoaded == 0) // GTA:SA
    {
        return *(bool*)((int)camera + 0x1C);
    }
    else // GTA:VC ?
    {
        return *(bool*)((int)camera + 0x4C);
    }
}
void CSoundSystem::Update()
{
    if (*userPaused || *codePaused)	// covers menu pausing, no disc in drive pausing (KILL MAN: disc on a phone), etc.
    {
        if (!paused) PauseStreams();
    }
    else
    {
        if (paused) ResumeStreams();

        masterSpeed = *ms_fTimeScale;
        masterVolumeSfx = 0.5f * GetEffectsVolume();
        masterVolumeMusic = 0.5f * GetMusicVolume();

        CMatrix* pMatrix = nGameLoaded == 1 ? camera->GetCamMatVC() : camera->GetMatSA();
        CVector* pVec = &pMatrix->pos;

        bass_frontVec = {pMatrix->at.y, pMatrix->at.z, pMatrix->at.x};
        bass_topVec = {pMatrix->up.y, pMatrix->up.z, pMatrix->up.x};

        BASS_3DVECTOR prevPos = pos;
        pos = BASS_3DVECTOR(pVec->y, pVec->z, pVec->x);

        vel = prevPos;
        vel.x -= pos.x;
        vel.y -= pos.y;
        vel.z -= pos.z;
        float timeDelta = 0.001f * (*m_snTimeInMillisecondsNonClipped - *m_snPreviousTimeInMillisecondsNonClipped);
        vel.x *= timeDelta;
        vel.y *= timeDelta;
        vel.z *= timeDelta;

        if(!Get_Just_Switched_Status(camera) && !CameraJustRestored())
        {
            BASS->Set3DPosition( &pos, &vel, pMatrix ? &bass_frontVec : NULL, pMatrix ? &bass_topVec : NULL);
        }

        // process all streams
        std::for_each(streams.begin(), streams.end(), [](CAudioStream *stream) { stream->Process(); });

        // apply above changes
        BASS->Apply3D();
    }
}

CAudioStream::CAudioStream() : streamInternal(0), state(eStreamState::Paused), OK(false), type(eStreamType::None) {}
CAudioStream::CAudioStream(const char *src) : state(eStreamState::Paused), OK(false), type(eStreamType::None)
{
    unsigned flags = BASS_SAMPLE_SOFTWARE;
    if (soundsys->bUseFPAudio) flags |= BASS_SAMPLE_FLOAT;
    if (!(streamInternal = BASS->StreamCreateURL(src, 0, flags, NULL)) && !(streamInternal = BASS->StreamCreateFile(false, src, 0, 0, flags)) &&
        !(streamInternal = BASS->StreamCreateFile(false, (sGameRoot + src).c_str(), 0, 0, flags)) &&
        !(streamInternal = BASS->StreamCreateFile(false, (std::string(cleo->GetCleoStorageDir()) + "/" + src).c_str(), 0, 0, flags)))
    {
        logger->Error("Loading audiostream failed. Error code: %d\nSource: \"%s\"", BASS->ErrorGetCode(), src);
        return;
    }
    OK = true;
    BASS->ChannelGetAttribute(streamInternal, BASS_ATTRIB_FREQ, &rate);
}

CAudioStream::~CAudioStream()
{
    if (streamInternal) BASS->StreamFree(streamInternal);
}

void CAudioStream::Play()
{
    if (state == eStreamState::Stopped) BASS->ChannelSetPosition(streamInternal, 0, BASS_POS_BYTE); // rewind
    state = eStreamState::PlayingInactive;
}

void CAudioStream::Pause(bool change_state)
{
    if (GetState() == eStreamState::Playing)
    {
        BASS->ChannelPause(streamInternal);
        state = (change_state) ? eStreamState::Paused : eStreamState::PlayingInactive;
    }
}

void CAudioStream::Stop()
{
    BASS->ChannelPause(streamInternal);
    state = eStreamState::Stopped;

    speed = speedTarget;
    volume = volumeTarget;
}

void CAudioStream::Resume()
{
    Play();
}

float CAudioStream::GetLength()
{
    return (float)BASS->ChannelBytes2Seconds(streamInternal, BASS->ChannelGetLength(streamInternal, BASS_POS_BYTE));
}

int CAudioStream::GetState()
{
    return (state == eStreamState::PlayingInactive) ? eStreamState::Playing : state;
}
 
void CAudioStream::SetLooping(bool enable)
{
    BASS->ChannelFlags(streamInternal, enable ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);
}

bool CAudioStream::GetLooping()
{
    return (BASS->ChannelFlags(streamInternal, 0, 0) & BASS_SAMPLE_LOOP) != 0;
}

uint32_t CAudioStream::GetInternal()
{
    return streamInternal;
}

void CAudioStream::UpdateSpeed()
{
    if (speed != speedTarget)
    {
        auto timeDelta = *m_snTimeInMillisecondsNonClipped - *m_snPreviousTimeInMillisecondsNonClipped;
        speed += speedTransitionStep * (double)timeDelta; // animate the transition

        // check progress
        auto remaining = speedTarget - speed;
        remaining *= (speedTransitionStep > 0.0) ? 1.0 : -1.0;
        if (remaining < 0.0) // overshoot
        {
            speed = speedTarget; // done
            if (speed <= 0.0f) Pause();
        }
    }

    float freq = rate * (float)speed * CSoundSystem::masterSpeed;
    freq = fmaxf(freq, 0.000001f); // 0 results in original speed
    BASS->ChannelSetAttribute(streamInternal, BASS_ATTRIB_FREQ, freq);
}

float CAudioStream::GetSpeed()
{
    return speed;
}

void CAudioStream::SetSpeed(float value, float transitionTime)
{
    if (transitionTime > 0.0f) Resume();

    value = fmaxf(value, 0.0f);
    speedTarget = value;

    if (transitionTime <= 0.0)
        speed = value; // instant
    else
        speedTransitionStep = (speedTarget - speed) / (1000.0 * transitionTime);
}

void CAudioStream::UpdateVolume()
{
    if (volume != volumeTarget)
    {
        auto timeDelta = *m_snTimeInMillisecondsNonClipped - *m_snPreviousTimeInMillisecondsNonClipped;
        volume += volumeTransitionStep * (double)timeDelta; // animate the transition

        // check progress
        auto remaining = volumeTarget - volume;
        remaining *= (volumeTransitionStep > 0.0) ? 1.0 : -1.0;
        if (remaining < 0.0) // overshoot
        {
            volume = volumeTarget;
            if (volume <= 0.0f) Pause();
        }
    }

    float masterVolume;
    switch(type)
    {
        case SoundEffect: masterVolume = CSoundSystem::masterVolumeSfx; break;
        case Music: masterVolume = CSoundSystem::masterVolumeMusic; break;
        default: masterVolume = 1.0f; break;
    }

    BASS->ChannelSetAttribute(streamInternal, BASS_ATTRIB_VOL, (float)volume * masterVolume);
}

float CAudioStream::GetVolume()
{
    return volume;
}

void CAudioStream::SetVolume(float value, float transitionTime)
{
    if (transitionTime > 0.0f) Resume();

    value = fmaxf(value, 0.0f);
    volumeTarget = value;

    if (transitionTime <= 0.0)
        volume = value; // instant
    else
        volumeTransitionStep = (volumeTarget - volume) / (1000.0 * transitionTime);
}

void CAudioStream::SetProgress(float value)
{
    if(value == 0)
    {
        BASS->ChannelSetPosition(streamInternal, bytePos, BASS_POS_BYTE);
        return;
    }
    double val = (value > 1.0) ? 1.0 : ((value < 0.0) ? 0.0 : (double)value);
    uint64_t total = BASS->ChannelGetLength(streamInternal, BASS_POS_BYTE);
    uint64_t bytePos = (uint64_t)(val * total);
    BASS->ChannelSetPosition(streamInternal, bytePos, BASS_POS_BYTE);
}

float CAudioStream::GetProgress()
{
    auto total = BASS->ChannelGetLength(streamInternal, BASS_POS_BYTE); // returns -1 on error
    auto bytePos = BASS->ChannelGetPosition(streamInternal, BASS_POS_BYTE); // returns -1 on error

    if (bytePos == -1) bytePos = 0; // error or not available yet

    float progress = (float)bytePos / total;
    progress = std::clamp(progress, 0.0f, 1.0f);
    return progress;
}

void CAudioStream::SetType(int newtype)
{
    if(newtype == eStreamType::SoundEffect || newtype == eStreamType::Music)
    {
        type = (eStreamType)newtype;
    }
    else
    {
        type = (eStreamType)eStreamType::None;
    }
}

eStreamType CAudioStream::GetType()
{
    return type;
}

void CAudioStream::Process()
{
    if (state == eStreamState::PlayingInactive)
    {
        BASS->ChannelPlay(streamInternal, false);
        state = eStreamState::Playing;
    }

    if (!GetLooping() && GetProgress() >= 1.0f) // end reached
    {
        state = eStreamState::Stopped;
    }

    if (state != eStreamState::Playing) return; // done

    UpdateSpeed();
    UpdateVolume();
}

bool CAudioStream::Is3DSource()
{
    return false;
}

void CAudioStream::Set3DPosition(const CVector&)
{
    logger->Error("Unimplemented CAudioStream::Set3DPosition(const CVector&)");
}

void CAudioStream::Set3DPosition(float, float, float)
{
    logger->Error("Unimplemented CAudioStream::Set3DPosition(float,float,float)");
}

void CAudioStream::Set3DRadius(float radius)
{
    logger->Error("Unimplemented CAudioStream::Set3DRadius(float radius)");
}

void CAudioStream::Link(CPlaceable*)
{
    logger->Error("Unimplemented CAudioStream::Link(CPlaceable*)");
}

////////////////// 3D Audiostream //////////////////

C3DAudioStream::C3DAudioStream(const char *src) : CAudioStream(), link(NULL)
{
    unsigned flags = BASS_SAMPLE_3D | BASS_SAMPLE_MONO | BASS_SAMPLE_SOFTWARE;
    if (soundsys->bUseFPAudio) flags |= BASS_SAMPLE_FLOAT;
    if (!(streamInternal = BASS->StreamCreateURL(src, 0, flags, NULL)) && !(streamInternal = BASS->StreamCreateFile(false, src, 0, 0, flags)) &&
        !(streamInternal = BASS->StreamCreateFile(false, (sGameRoot + src).c_str(), 0, 0, flags)) &&
        !(streamInternal = BASS->StreamCreateFile(false, (std::string(cleo->GetCleoStorageDir()) + "/" + src).c_str(), 0, 0, flags)))
    {
        logger->Error("Loading 3D audiostream failed. Error code: %d\nSource: \"%s\"", BASS->ErrorGetCode(), src);
        return;
    }
    OK = true;
    BASS->ChannelGetAttribute(streamInternal, BASS_ATTRIB_FREQ, &rate);
    BASS->ChannelSet3DAttributes(streamInternal, BASS_3DMODE_NORMAL, 3.0f, 1E+12f, -1, -1, -1.0f);
}

C3DAudioStream::~C3DAudioStream()
{
    if (streamInternal) BASS->StreamFree(streamInternal);
}

bool C3DAudioStream::Is3DSource()
{
    return true;
}

void C3DAudioStream::Set3DPosition(const CVector& pos)
{
    position.x = pos.y;
    position.y = pos.z;
    position.z = pos.x;
    link = NULL;
    BASS_3DVECTOR avel = { 0.0f, 0.0f, 0.0f };
    BASS->ChannelSet3DPosition(streamInternal, &position, NULL, &avel);
}

void C3DAudioStream::Set3DPosition(float x, float y, float z)
{
    position.x = y;
    position.y = z;
    position.z = x;
    link = NULL;
    BASS_3DVECTOR avel = { 0.0f, 0.0f, 0.0f };
    BASS->ChannelSet3DPosition(streamInternal, &position, NULL, &avel);
}

void C3DAudioStream::Set3DRadius(float radius)
{
    BASS->ChannelSet3DAttributes(streamInternal, BASS_3DMODE_NORMAL, radius, 1E+12f, -1, -1, -1.0f);
}

void C3DAudioStream::Link(CPlaceable *placeable)
{
    link = placeable;
}

void C3DAudioStream::Process()
{
    CAudioStream::Process();

    if (state != eStreamState::Playing) return; // done

    UpdatePosition();
}

void C3DAudioStream::UpdatePosition()
{
    if (link) // attached to entity
    {
        auto prevPos = position;
        CVector* pVec = nGameLoaded==1 ? link->GetPosVC() : link->GetPosSA();
        position = BASS_3DVECTOR(pVec->y, pVec->z, pVec->x);

        // calculate velocity
        BASS_3DVECTOR avel = position;
        avel.x -= prevPos.x;
        avel.y -= prevPos.y;
        avel.z -= prevPos.z;
        auto timeDelta = 0.001f * (*m_snTimeInMillisecondsNonClipped - *m_snPreviousTimeInMillisecondsNonClipped);
        avel.x *= timeDelta;
        avel.y *= timeDelta;
        avel.z *= timeDelta;

        BASS->ChannelSet3DPosition(streamInternal, &position, NULL, &avel);
    }
}
