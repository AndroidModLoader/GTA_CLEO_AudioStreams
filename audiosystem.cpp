#include <algorithm>
#include <string>

#include "audiosystem.h"
#include "mod/amlmod.h"
#include "mod/logger.h"

#include "cleo.h"
extern cleo_ifs_t* cleo;

BASS_3DVECTOR pos(0, 0, 0), vel(0, 0, 0), front(0, -1.0, 0), top(0, 0, 1.0);
BASS_3DVECTOR bass_tmp(0.0f, 0.0f, 0.0f), bass_tmp2(0.0f, 0.0f, 0.0f), bass_tmp3(0.0f, 0.0f, 0.0f);
extern CCamera *camera;
extern bool* userPaused;
extern bool* codePaused;
extern int nGameLoaded;
extern uint32_t* m_snTimeInMillisecondsNonClipped;
extern uint32_t* m_snPreviousTimeInMillisecondsNonClipped;
extern float* ms_fTimeScale;

std::string sGameRoot;
float CSoundSystem::masterSpeed = 1.0f;
float CSoundSystem::masterVolumeSfx = 1.0f;
float CSoundSystem::masterVolumeMusic = 1.0f;

bool CSoundSystem::Init()
{
    sGameRoot = aml->GetAndroidDataPath();
    if (BASS->Set3DFactors(1.0f, 0.3f, 1.0f) && BASS->Set3DPosition(&pos, &vel, &front, &top))
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
        if (stream->state == eStreamState::Playing) stream->Pause(false);
    });
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

        CMatrix* pMatrix = nGameLoaded == 1 ? camera->GetCamMatVC() : camera->GetMatSA();
        CVector* pVec = &pMatrix->pos;

        bass_tmp = {pVec->y, pVec->z, pVec->x};
        bass_tmp2 = {pMatrix->at.y, pMatrix->at.z, pMatrix->at.x};
        bass_tmp3 = {pMatrix->up.y, pMatrix->up.z, pMatrix->up.x};

        // TODO: Doppler effect here

        BASS->Set3DPosition( &bass_tmp, nullptr, pMatrix ? &bass_tmp2 : nullptr, pMatrix ? &bass_tmp3 : nullptr);

        // process all streams
        std::for_each(streams.begin(), streams.end(), [](CAudioStream *stream) { stream->Process(); });
        // apply above changes
        BASS->Apply3D();
    }
}

CAudioStream::CAudioStream() : streamInternal(0), state(eStreamState::NoState), OK(false) {}
CAudioStream::CAudioStream(const char *src) : state(eStreamState::NoState), OK(false)
{
    unsigned flags = BASS_SAMPLE_SOFTWARE;
    if (soundsys->bUseFPAudio) flags |= BASS_SAMPLE_FLOAT;
    if (!(streamInternal = BASS->StreamCreateURL(src, 0, flags, NULL)) && !(streamInternal = BASS->StreamCreateFile(false, src, 0, 0, flags)) &&
        !(streamInternal = BASS->StreamCreateFile(false, (sGameRoot + src).c_str(), 0, 0, flags)) &&
        !(streamInternal = BASS->StreamCreateFile(false, (std::string(cleo->GetCleoStorageDir()) + "/" + src).c_str(), 0, 0, flags)))
    {
        logger->Error("Loading audiostream failed. Error code: %d\nSource: \"%s\"", BASS->ErrorGetCode(), src);
    }
    else OK = true;
    BASS->ChannelGetAttribute(streamInternal, BASS_ATTRIB_FREQ, &rate);
}

CAudioStream::~CAudioStream()
{
    if (streamInternal) BASS->StreamFree(streamInternal);
}

void CAudioStream::Play()
{
    BASS->ChannelPlay(streamInternal, true);
    state = eStreamState::Playing;
}

void CAudioStream::Pause(bool change_state)
{
    BASS->ChannelPause(streamInternal);
    if (change_state) state = eStreamState::Paused;
}

void CAudioStream::Stop()
{
    BASS->ChannelPause(streamInternal);
    BASS->ChannelSetPosition(streamInternal, 0, BASS_POS_BYTE);
    state = eStreamState::Paused;
}

void CAudioStream::Resume()
{
    BASS->ChannelPlay(streamInternal, false);
    state = eStreamState::Playing;
}

float CAudioStream::GetLength()
{
    return (float)BASS->ChannelBytes2Seconds(streamInternal, BASS->ChannelGetLength(streamInternal, BASS_POS_BYTE));
}

int CAudioStream::GetState()
{
    if (state == eStreamState::Stopped) return -1;
    switch (BASS->ChannelIsActive(streamInternal))
    {
        case BASS_ACTIVE_STOPPED:
        default:
            return -1;
            
        case BASS_ACTIVE_PLAYING:
        case BASS_ACTIVE_STALLED:
            return 1;
            
        case BASS_ACTIVE_PAUSED:
            return 2;
    };
}
 
void CAudioStream::Loop(bool enable)
{
    BASS->ChannelFlags(streamInternal, enable ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);
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

    float masterVolume = 1.0f;
    //switch(type)
    //{
    //    case SoundEffect: masterVolume = CSoundSystem::masterVolumeSfx; break;
    //    case Music: masterVolume = CSoundSystem::masterVolumeMusic; break;
    //}

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
    double val = std::clamp(value, 0.0f, 1.0f);
    uint64_t total = BASS->ChannelGetLength(streamInternal, BASS_POS_BYTE);
    uint64_t bytePos = (uint64_t)(total * val);
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
    switch (BASS->ChannelIsActive(streamInternal))
    {
        case BASS_ACTIVE_PAUSED:
            state = eStreamState::Paused;
            break;
            
        case BASS_ACTIVE_PLAYING:
        case BASS_ACTIVE_STALLED:
            state = eStreamState::Playing;
            break;
            
        case BASS_ACTIVE_STOPPED:
            state = eStreamState::Stopped;
            break;
    }

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
    }
    else
    {
        BASS->ChannelSet3DAttributes(streamInternal, 0, -1.0, -1.0, -1, -1, -1.0);
        OK = true;
    }
    BASS->ChannelGetAttribute(streamInternal, BASS_ATTRIB_FREQ, &rate);
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
    BASS->ChannelSet3DPosition(streamInternal, &position, NULL, NULL);
}

void C3DAudioStream::Set3DPosition(float x, float y, float z)
{
    position.x = y;
    position.y = z;
    position.z = x;
    link = NULL;
    BASS->ChannelSet3DPosition(streamInternal, &position, NULL, NULL);
}

void C3DAudioStream::Set3DRadius(float radius)
{
    BASS_SAMPLE sampleData;
    BASS->SampleGetInfo(streamInternal, &sampleData);
    sampleData.maxdist = radius;
    BASS->SampleSetInfo(streamInternal, &sampleData);
}

void C3DAudioStream::Link(CPlaceable *placeable)
{
    link = placeable;
}

void C3DAudioStream::Process()
{
    // update playing position of the linked object
    switch (BASS->ChannelIsActive(streamInternal))
    {
        case BASS_ACTIVE_PAUSED:
            state = eStreamState::Paused;
            break;
            
        case BASS_ACTIVE_PLAYING:
        case BASS_ACTIVE_STALLED:
            state = eStreamState::Playing;
            break;
            
        case BASS_ACTIVE_STOPPED:
            state = eStreamState::Stopped;
            break;
    }
    
    if (state == eStreamState::Playing)
    {
        if (link)
        {
            CVector* pVec = nGameLoaded==1 ? link->GetPosVC() : link->GetPosSA();
            position.x = pVec->y;
            position.y = pVec->z;
            position.z = pVec->x;
        }
        BASS->ChannelSet3DPosition(streamInternal, &position, NULL, NULL);
    }

    UpdateSpeed();
    UpdateVolume();
}
