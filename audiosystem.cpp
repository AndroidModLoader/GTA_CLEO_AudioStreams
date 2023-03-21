#include <algorithm>
#include <string>

#include "audiosystem.h"
#include "GTASA_STRUCTS.h"
#include "mod/amlmod.h"
#include "mod/logger.h"

BASS_3DVECTOR pos(0, 0, 0), vel(0, 0, 0), front(0, -1.0, 0), top(0, 0, 1.0);
BASS_3DVECTOR bass_tmp(0.0f, 0.0f, 0.0f), bass_tmp2(0.0f, 0.0f, 0.0f), bass_tmp3(0.0f, 0.0f, 0.0f);
extern CCamera *camera;
extern bool* userPaused;
extern bool* codePaused;
extern CPlayerPed* (*FindPlayerPed)(int);

std::string sGameRoot;

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
        if (stream->state == CAudioStream::playing) stream->Resume();
    });
}

void CSoundSystem::PauseStreams()
{
    paused = true;
    std::for_each(streams.begin(), streams.end(), [](CAudioStream *stream) {
        if (stream->state == CAudioStream::playing) stream->Pause(false);
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

        // TODO: Get it to work with CAMERA (Camera shows me it`s position as 15.9 XYZ...)
        //CMatrix* pMatrix = NULL;/
        //CVector* pVec = &FindPlayerPed(-1)->GetPosition();
        
        // Will these work?!
        /*CMatrix* pMatrix = &camera->m_matCameraMatrix;
        CVector* pVec = &camera->m_vecGameCamPos;
        
        bass_tmp.x = pVec->y;
        bass_tmp.y = pVec->z;
        bass_tmp.z = pVec->x;

        if(pMatrix)
        {
            bass_tmp2.x = pMatrix->at.y;
            bass_tmp2.y = pMatrix->at.z;
            bass_tmp2.z = pMatrix->at.x;

            bass_tmp3.x = pMatrix->up.y;
            bass_tmp3.y = pMatrix->up.z;
            bass_tmp3.z = pMatrix->up.x;

            BASS->Set3DPosition(&bass_tmp, NULL, &bass_tmp2, &bass_tmp3);
        }
        else
        {
            BASS->Set3DPosition(&bass_tmp, NULL, NULL, NULL);
        }*/
        
        CCam& cam = camera->m_apCams[camera->m_nCurrentActiveCam];
        
        // Pos
        bass_tmp.x = cam.Source.y;
        bass_tmp.y = cam.Source.z;
        bass_tmp.z = cam.Source.x;
        
        // At (Front)
        bass_tmp2.x = cam.Front.y;
        bass_tmp2.y = cam.Front.z;
        bass_tmp2.z = cam.Front.x;
        
        // Up
        bass_tmp3.x = cam.Up.y;
        bass_tmp3.y = cam.Up.z;
        bass_tmp3.z = cam.Up.x;
        
        BASS->Set3DPosition(&bass_tmp, NULL, &bass_tmp2, &bass_tmp3);

        // process all streams
        std::for_each(streams.begin(), streams.end(), [](CAudioStream *stream) { stream->Process(); });
        // apply above changes
        BASS->Apply3D();
    }
}

CAudioStream::CAudioStream() : streamInternal(0), state(no), OK(false) {}
CAudioStream::CAudioStream(const char *src) : state(no), OK(false)
{
    unsigned flags = BASS_SAMPLE_SOFTWARE;
    if (soundsys->bUseFPAudio) flags |= BASS_SAMPLE_FLOAT;
    if (!(streamInternal = BASS->StreamCreateURL(src, 0, flags, NULL)) && !(streamInternal = BASS->StreamCreateFile(false, src, 0, 0, flags)) &&
        !(streamInternal = BASS->StreamCreateFile(false, (sGameRoot + src).c_str(), 0, 0, flags)))
    {
        logger->Error("Loading audiostream failed. Error code: %d\nSource: \"%s\"", BASS->ErrorGetCode(), src);
    }
    else OK = true;
}

CAudioStream::~CAudioStream()
{
    if (streamInternal) BASS->StreamFree(streamInternal);
}

void CAudioStream::Play()
{
    BASS->ChannelPlay(streamInternal, true);
    state = playing;
}

void CAudioStream::Pause(bool change_state)
{
    BASS->ChannelPause(streamInternal);
    if (change_state) state = paused;
}

void CAudioStream::Stop()
{
    BASS->ChannelPause(streamInternal);
    BASS->ChannelSetPosition(streamInternal, 0, BASS_POS_BYTE);
    state = paused;
}

void CAudioStream::Resume()
{
    BASS->ChannelPlay(streamInternal, false);
    state = playing;
}

uint64_t CAudioStream::GetLength()
{
    return (uint64_t)BASS->ChannelBytes2Seconds(streamInternal, BASS->ChannelGetLength(streamInternal, BASS_POS_BYTE));
}

int CAudioStream::GetState()
{
    if (state == stopped) return -1;
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

float CAudioStream::GetVolume()
{
    float result;
    if (!BASS->ChannelGetAttribute(streamInternal, BASS_ATTRIB_VOL, &result)) return -1.0f;
    return result;
}

void CAudioStream::SetVolume(float val)
{
    BASS->ChannelSetAttribute(streamInternal, BASS_ATTRIB_VOL, val);
}

void CAudioStream::Loop(bool enable)
{
    BASS->ChannelFlags(streamInternal, enable ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);
}
uint64_t CAudioStream::GetInternal() { return streamInternal; }

void CAudioStream::Process()
{
    switch (BASS->ChannelIsActive(streamInternal))
    {
        case BASS_ACTIVE_PAUSED:
            state = paused;
            break;
            
        case BASS_ACTIVE_PLAYING:
        case BASS_ACTIVE_STALLED:
            state = playing;
            break;
            
        case BASS_ACTIVE_STOPPED:
            state = stopped;
            break;
    }
}

void CAudioStream::Set3DPosition(const CVector&)
{
    logger->Error("Unimplemented CAudioStream::Set3DPosition(const CVector&)");
}

void CAudioStream::Set3DPosition(float, float, float)
{
    logger->Error("Unimplemented CAudioStream::Set3DPosition(float,float,float)");
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
        !(streamInternal = BASS->StreamCreateFile(false, (sGameRoot + src).c_str(), 0, 0, flags)))
    {
        logger->Error("Loading 3D audiostream failed. Error code: %d\nSource: \"%s\"", BASS->ErrorGetCode(), src);
    }
    else
    {
        BASS->ChannelSet3DAttributes(streamInternal, 0, -1.0, -1.0, -1, -1, -1.0);
        OK = true;
    }
}

C3DAudioStream::~C3DAudioStream()
{
    if (streamInternal) BASS->StreamFree(streamInternal);
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
            state = paused;
            break;
            
        case BASS_ACTIVE_PLAYING:
        case BASS_ACTIVE_STALLED:
            state = playing;
            break;
            
        case BASS_ACTIVE_STOPPED:
            state = stopped;
            break;
    }
    
    if (state == playing)
    {
        if (link)
        {
            CVector* pVec = &link->GetPosition();
            position.x = pVec->y;
            position.y = pVec->z;
            position.z = pVec->x;
        }
        BASS->ChannelSet3DPosition(streamInternal, &position, NULL, NULL);
    }
}
