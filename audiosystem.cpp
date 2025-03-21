#include <algorithm>
#include <string>

#include "audiosystem.h"
#include "mod/amlmod.h"
#include "mod/logger.h"

#include "cleo.h"
extern cleo_ifs_t* cleo;



BASS_3DVECTOR pos(0, 0, 0), vel(0, 0, 0), front(0, -1.0f, 0), top(0, 0, 1.0f);
BASS_3DVECTOR bass_frontVec(0.0f, 0.0f, 0.0f), bass_topVec(0.0f, 0.0f, 0.0f), bass_emptyVec(0.0f, 0.0f, 0.0f);

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
extern int (*GetScreenOrientation)();
float GetEffectsVolume();
float GetMusicVolume();



inline bool IsURLPath(const char* path)
{
    return strncmp("http:", path, 5) == 0 ||  strncmp("https:", path, 6) == 0;
}

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
int updateFrames = 0, displayOrientation = 0;
void CSoundSystem::Update()
{
    if (*userPaused || *codePaused)	// covers menu pausing, no disc in drive pausing (KILL MAN: disc on a phone), etc.
    {
        if (!paused) PauseStreams();
    }
    else
    {
        updateFrames = (++updateFrames) % 8;
        if(updateFrames == 0)
        {
            displayOrientation = GetScreenOrientation();
        }
            
        if (paused) ResumeStreams();

        masterSpeed = *ms_fTimeScale;
        masterVolumeSfx = 0.5f * GetEffectsVolume();
        masterVolumeMusic = 0.5f * GetMusicVolume();

        CMatrix* pMatrix = nGameLoaded == 1 ? camera->GetCamMatVC() : camera->GetMatSA();
        CVector* pVec = &pMatrix->pos;

        bass_frontVec = { pMatrix->at.y, pMatrix->at.z, pMatrix->at.x };
        bass_topVec   = { pMatrix->up.y, pMatrix->up.z, pMatrix->up.x };

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

        if(displayOrientation > 1)
        {
            // is this correct?
            bass_frontVec.z = -bass_frontVec.z;
        }

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
bool CSoundSystem::IsStreamInList(CAudioStream *stream)
{
    return ( streams.find(stream) != streams.end() );
}

CAudioStream::CAudioStream() : streamInternal(0), state(eStreamState::Paused), OK(false),
                               type(eStreamType::SoundEffect), takeGameSpeedIntoAccount(false) {}

CAudioStream::CAudioStream(const char *src) : state(eStreamState::Paused), OK(false),
                                              type(eStreamType::SoundEffect), takeGameSpeedIntoAccount(false)
{
    unsigned flags = BASS_SAMPLE_SOFTWARE | BASS_STREAM_PRESCAN;
    if (soundsys->bUseFPAudio) flags |= BASS_SAMPLE_FLOAT;
    if (!(IsURLPath(src) && (streamInternal = BASS->StreamCreateURL(src, 0, flags, NULL))) &&
        !(streamInternal = BASS->StreamCreateFile(false, src, 0, 0, flags)) &&
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
        if (remaining < 0.0f) // overshoot
        {
            speed = speedTarget; // done
            if (speed <= 0.0f) Pause();
        }
    }

    float masterSpeed = takeGameSpeedIntoAccount ? CSoundSystem::masterSpeed : 1.0f;
    /*switch(type)
    {
        case eStreamType::SoundEffect:
        case eStreamType::Music: // and muted
            masterSpeed = CSoundSystem::masterSpeed;
            break;

        default:
            masterSpeed = 1.0f;
            break;
    }*/

    float freq = rate * (float)speed * masterSpeed;
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
        if (remaining < 0.0f) // overshoot
        {
            volume = volumeTarget;
            if (volume <= 0.0f) Pause();
        }
    }

    float masterVolume;
    switch(type)
    {
        case SoundEffect: masterVolume = CSoundSystem::masterVolumeSfx; break;
        case Music: masterVolume = (CSoundSystem::masterSpeed == 1.0f) ? CSoundSystem::masterVolumeMusic : 0.0f; break;
        case UserInterface: masterVolume = CSoundSystem::masterVolumeSfx; break;
        default: masterVolume = 1.0f;
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

    if (transitionTime <= 0.0f)
        volume = value; // instant
    else
        volumeTransitionStep = (volumeTarget - volume) / (1000.0f * transitionTime);
}

void CAudioStream::SetProgress(float value)
{
    if (GetState() == Stopped)
    {
        state = Paused; // resume from set progress
    }
    value = std::clamp(value, 0.0f, 1.0f);
    auto bytePos = BASS->ChannelSeconds2Bytes(streamInternal, GetLength() * value);
    BASS->ChannelSetPosition(streamInternal, bytePos, BASS_POS_BYTE);
}

float CAudioStream::GetProgress()
{
    auto bytePos = BASS->ChannelGetPosition(streamInternal, BASS_POS_BYTE);
    if (bytePos == -1) bytePos = 0; // error or not available yet
    auto pos = BASS->ChannelBytes2Seconds(streamInternal, bytePos);

    auto byteTotal = BASS->ChannelGetLength(streamInternal, BASS_POS_BYTE);
    auto total = BASS->ChannelBytes2Seconds(streamInternal, byteTotal);
    return (float)(pos / total);
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

void CAudioStream::SetTakeGameSpeedIntoAccount(bool enable)
{
    takeGameSpeedIntoAccount = enable;
}

bool CAudioStream::IsTakingGameSpeedIntoAccount()
{
    return takeGameSpeedIntoAccount;
}

void CAudioStream::Process()
{
    if (state == eStreamState::PlayingInactive)
    {
        BASS->ChannelPlay(streamInternal, false);
        state = eStreamState::Playing;
    }
    else
    {
        if (state == Playing && BASS->ChannelIsActive(streamInternal) == BASS_ACTIVE_STOPPED) // end reached
        {
            state = eStreamState::Stopped;
        }
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

void CAudioStream::SetMin3DRadius(float radius)
{
    logger->Error("Unimplemented CAudioStream::SetMin3DRadius(float)");
}

void CAudioStream::SetMax3DRadius(float radius)
{
    logger->Error("Unimplemented CAudioStream::SetMax3DRadius(float)");
}

float CAudioStream::GetMin3DRadius()
{
    logger->Error("Unimplemented CAudioStream::GetMin3DRadius()");
    return 0.0f;
}

float CAudioStream::GetMax3DRadius()
{
    logger->Error("Unimplemented CAudioStream::GetMax3DRadius()");
    return 0.0f;
}

void CAudioStream::Link(CPlaceable*)
{
    logger->Error("Unimplemented CAudioStream::Link(CPlaceable*)");
}

bool CAudioStream::IsLinked()
{
    logger->Error("Unimplemented CAudioStream::IsLinked()");
    return false;
}

CVector CAudioStream::GetPosition()
{
    logger->Error("Unimplemented CAudioStream::GetPosition()");
    return CVector {0, 0, 0};
}

void CAudioStream::SetDopplerEffect(bool enable)
{
    logger->Error("Unimplemented CAudioStream::SetDopplerEffect(bool)");
}

bool CAudioStream::GetDopplerEffect()
{
    logger->Error("Unimplemented CAudioStream::GetDopplerEffect()");
    return false;
}

////////////////// 3D Audiostream //////////////////

C3DAudioStream::C3DAudioStream(const char *src) : CAudioStream(), link(NULL), dopplerEffect(true)
{
    unsigned flags = BASS_SAMPLE_3D | BASS_SAMPLE_MONO | BASS_SAMPLE_SOFTWARE;
    if (soundsys->bUseFPAudio) flags |= BASS_SAMPLE_FLOAT;
    if (!(IsURLPath(src) && (streamInternal = BASS->StreamCreateURL(src, 0, flags, NULL))) &&
        !(streamInternal = BASS->StreamCreateFile(false, src, 0, 0, flags)) &&
        !(streamInternal = BASS->StreamCreateFile(false, (sGameRoot + src).c_str(), 0, 0, flags)) &&
        !(streamInternal = BASS->StreamCreateFile(false, (std::string(cleo->GetCleoStorageDir()) + "/" + src).c_str(), 0, 0, flags)))
    {
        logger->Error("Loading 3D audiostream failed. Error code: %d\nSource: \"%s\"", BASS->ErrorGetCode(), src);
        return;
    }
    OK = true;
    BASS->ChannelGetAttribute(streamInternal, BASS_ATTRIB_FREQ, &rate);
    UpdateRadius();
    //BASS->ChannelSet3DAttributes(streamInternal, BASS_3DMODE_NORMAL, 3.0f, 1E+12f, -1, -1, -1.0f); // ^ this one is in a function above it
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
    BASS->ChannelSet3DPosition(streamInternal, &position, NULL, &bass_emptyVec);
}

void C3DAudioStream::Set3DPosition(float x, float y, float z)
{
    position.x = y;
    position.y = z;
    position.z = x;
    link = NULL;
    BASS->ChannelSet3DPosition(streamInternal, &position, NULL, &bass_emptyVec);
}

void C3DAudioStream::SetMin3DRadius(float radius)
{
    minRadius = radius;
    UpdateRadius();
}

void C3DAudioStream::SetMax3DRadius(float radius)
{
    maxRadius = radius;
    UpdateRadius();
}

float C3DAudioStream::GetMin3DRadius()
{
    return minRadius;
}

float C3DAudioStream::GetMax3DRadius()
{
    return maxRadius;
}

void C3DAudioStream::Link(CPlaceable *placeable)
{
    link = placeable;
}

bool C3DAudioStream::IsLinked()
{
    return (link != NULL);
}

void C3DAudioStream::Process()
{
    CAudioStream::Process();

    if (state != eStreamState::Playing) return; // done

    UpdatePosition();
}

void C3DAudioStream::UpdatePosition()
{
    BASS_3DVECTOR avel = bass_emptyVec;
    if (link) // attached to entity
    {
        auto prevPos = position;
        CVector* pVec = nGameLoaded==1 ? link->GetPosVC() : link->GetPosSA();
        position = BASS_3DVECTOR(pVec->y, pVec->z, pVec->x);

        if(dopplerEffect)
        {
            // calculate velocity
            avel = position;
            avel.x -= prevPos.x;
            avel.y -= prevPos.y;
            avel.z -= prevPos.z;
            auto timeDelta = 0.001f * (*m_snTimeInMillisecondsNonClipped - *m_snPreviousTimeInMillisecondsNonClipped);
            avel.x *= timeDelta;
            avel.y *= timeDelta;
            avel.z *= timeDelta;
        }
    }
    BASS->ChannelSet3DPosition(streamInternal, &position, NULL, &avel);
}

void C3DAudioStream::UpdateRadius()
{
    BASS->ChannelSet3DAttributes(streamInternal, BASS_3DMODE_NORMAL, minRadius, maxRadius, -1, -1, -1.0f);
}

CVector C3DAudioStream::GetPosition()
{
    return CVector {position.z, position.x, position.y};
}

void C3DAudioStream::SetDopplerEffect(bool enable)
{
    dopplerEffect = enable;
}

bool C3DAudioStream::GetDopplerEffect()
{
    return dopplerEffect;
}
