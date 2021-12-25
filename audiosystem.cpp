#include <algorithm>
#include <string>

#include "ibass.h"
#include "audiosystem.h"
#include "mod/logger.h"

BASS_3DVECTOR pos(0, 0, 0), vel(0, 0, 0), front(0, -1.0, 0), top(0, 0, 1.0);
BASS_3DVECTOR bass_tmp(0.0f, 0.0f, 0.0f), bass_tmp2(0.0f, 0.0f, 0.0f), bass_tmp3(0.0f, 0.0f, 0.0f);
extern CPlaceable *camera;
extern bool* userPaused;
extern bool* codePaused;

bool CSoundSystem::Init()
{
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
    logger->Error("Could not initialize BASS sound system");
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
    return nullptr;
}

void CSoundSystem::UnloadStream(CAudioStream *stream)
{
    if (streams.erase(stream))
        delete stream;
    else
        logger->Error("Unloading of stream that is not in list of loaded streams");
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

        // not in menu
        // process camera movements

        CMatrixLink * pMatrix = nullptr;
        CVector * pVec = nullptr;
        if (camera->m_matrix)
        {
            pMatrix = camera->m_matrix;
            pVec = &pMatrix->pos;
        }
        else pVec = &camera->m_placement.m_vPosn;

        bass_tmp.x = pVec->y;
        bass_tmp.y = pVec->z;
        bass_tmp.z = pVec->x;

        bass_tmp2.x = pMatrix->at.y;
        bass_tmp2.y = pMatrix->at.z;
        bass_tmp2.z = pMatrix->at.x;

        bass_tmp3.x = pMatrix->up.y;
        bass_tmp3.y = pMatrix->up.z;
        bass_tmp3.z = pMatrix->up.x;

        BASS->Set3DPosition(
            &bass_tmp,
            nullptr,
            pMatrix ? &bass_tmp2 : nullptr,
            pMatrix ? &bass_tmp3 : nullptr
        );

        // process all streams
        std::for_each(streams.begin(), streams.end(), [](CAudioStream *stream) {
            stream->Process();
        });
        // apply above changes
        BASS->Apply3D();
    }
}

CAudioStream::CAudioStream()
    : streamInternal(0), state(no), OK(false)
{
}

CAudioStream::CAudioStream(const char *src) : state(no), OK(false)
{
    unsigned flags = BASS_SAMPLE_SOFTWARE;
    if (soundsys->bUseFPAudio)
        flags |= BASS_SAMPLE_FLOAT;
    std::string sabc1 = "/storage/emulated/0/Android/data/com.rockstargames.gtasa/files/"; sabc1 += src; // Yeah, completely hardcoded. For yet.
    if (!(streamInternal = BASS->StreamCreateURL(src, 0, flags, nullptr)) && !(streamInternal = BASS->StreamCreateFile(false, src, 0, 0, flags)) &&
        !(streamInternal = BASS->StreamCreateFile(false, sabc1.c_str(), 0, 0, flags)))
    {
        logger->Error("Loading audiostream failed. Error code: %d\nSource: \"%s\"", BASS->ErrorGetCode(), src);
    }
    else OK = true;
}

CAudioStream::~CAudioStream()
{
    if (streamInternal) BASS->StreamFree(streamInternal);
}

C3DAudioStream::C3DAudioStream(const char *src) : CAudioStream(), link(nullptr)
{
    unsigned flags = BASS_SAMPLE_3D | BASS_SAMPLE_MONO | BASS_SAMPLE_SOFTWARE;
    if (soundsys->bUseFPAudio)
        flags |= BASS_SAMPLE_FLOAT;
    std::string sabc1 = "/storage/emulated/0/Android/data/com.rockstargames.gtasa/files/"; sabc1 += src; // Yeah, completely hardcoded. For yet.
    if (!(streamInternal = BASS->StreamCreateURL(src, 0, flags, nullptr)) && !(streamInternal = BASS->StreamCreateFile(false, src, 0, 0, flags)) &&
        !(streamInternal = BASS->StreamCreateFile(false, sabc1.c_str(), 0, 0, flags)))
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
    return (unsigned)BASS->ChannelBytes2Seconds(streamInternal,
        BASS->ChannelGetLength(streamInternal, BASS_POS_BYTE));
}

int CAudioStream::GetState()
{
    if (state == stopped) return -1;		// dont do this in case we changed state by pausing
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
    if (!BASS->ChannelGetAttribute(streamInternal, BASS_ATTRIB_VOL, &result))
        return -1.0f;
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
uint64_t CAudioStream::GetInternal(){ return streamInternal; }

void CAudioStream::Process()
{
    // no actions required			// liez!

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

void CAudioStream::Set3dPosition(const CVector& pos)
{
    logger->Error("Unimplemented CAudioStream::Set3dPosition()");
}

void CAudioStream::Set3dPosition(float x, float y, float z)
{
    logger->Error("Unimplemented CAudioStream::Set3dPosition()");
}

void CAudioStream::Link(CPlaceable *placable)
{
    logger->Error("Unimplemented CAudioStream::Link()");
}

void C3DAudioStream::Set3dPosition(const CVector& pos)
{
    position.x = pos.y;
    position.y = pos.z;
    position.z = pos.x;
    link = nullptr;
    BASS->ChannelSet3DPosition(streamInternal, &position, nullptr, nullptr);
}

void C3DAudioStream::Set3dPosition(float x, float y, float z)
{
    position.x = y;
    position.y = z;
    position.z = x;
    link = nullptr;
    BASS->ChannelSet3DPosition(streamInternal, &position, nullptr, nullptr);
}

void C3DAudioStream::Link(CPlaceable *placable)
{
    link = placable;
    //Set3dPosition(placable->GetPos());
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
            // CPlaceable::m_matrix = this+20
            //CVector * pVec = link->m_matrix ? &link->m_matrix->pos : &link->m_placement.m_vPosn;

            //CMatrix* mtrx = *(CMatrix**)((uintptr_t)link+20);
            //CVector* pVec = mtrx ? &mtrx->pos : &link->m_placement.m_vPosn;

            CVector* pVec = &link->GetPosition();

            bass_tmp.x = pVec->y;
            bass_tmp.y = pVec->z;
            bass_tmp.z = pVec->x;
            BASS->ChannelSet3DPosition(streamInternal, &bass_tmp, nullptr, nullptr);
        }
        else
        {
            bass_tmp.x = position.y;
            bass_tmp.y = position.z;
            bass_tmp.z = position.x;
            BASS->ChannelSet3DPosition(streamInternal, &bass_tmp, nullptr, nullptr);
            //BASS->ChannelGet3DPosition(streamInternal, &position, nullptr, nullptr);
        }
    }
}