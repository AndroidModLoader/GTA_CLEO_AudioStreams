#include <mod/amlmod.h>
#include <mod/logger.h>
#include "audiosystem.h"

#include "cleo.h"
#include "cleoaddon.h"
cleo_ifs_t* cleo = NULL;
cleo_addon_ifs_t* cleoaddon = NULL;
IBASS* BASS = NULL;

static CSoundSystem soundsysLocal;
CSoundSystem* soundsys = &soundsysLocal;

CCamera *camera;
bool* userPaused;
bool* codePaused;
uint32_t* m_snTimeInMillisecondsNonClipped;
uint32_t* m_snPreviousTimeInMillisecondsNonClipped;
float* ms_fTimeScale;

int nGameLoaded = -1;

CObject*    (*GetObjectFromRef)(int) = NULL;
CPed*       (*GetPedFromRef)(int) = NULL;
CVehicle*   (*GetVehicleFromRef)(int) = NULL;
bool        (*Get_Just_Switched_Status)(CCamera*) = NULL;

MYMOD(net.alexblade.rusjj.audiostreams, CLEO AudioStreams, 1.4, Alexander Blade & RusJJ)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.cleolib, 2.0.1.6)
    ADD_DEPENDENCY(net.rusjj.basslib)
END_DEPLIST()

// Funcies

CLEO_Fn(LOAD_AUDIO_STREAM)
{
    char buf[256];
    cleoaddon->ReadString(handle, buf, sizeof(buf));
    int i = 0;
    while(buf[i] != 0) // A little hack
    {
        if(buf[i] == '\\') buf[i] = '/';
        ++i;
    }

    auto stream = soundsys->LoadStream(buf, false);
    cleo->GetPointerToScriptVar(handle)->u = (uint32_t)stream;
    cleoaddon->UpdateCompareFlag(handle, stream != NULL);
}
CLEO_Fn(SET_AUDIO_STREAM_STATE)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    if(!stream)
    {
        logger->Error("[%04X] Trying to do an action on NULL audiostream!", opcode);
        return;
    }
    int action = cleo->ReadParam(handle)->i;
    switch(action)
    {
        case 0: stream->Stop();   break;
        case 1: stream->Play();   break;
        case 2: stream->Pause();  break;
        case 3: stream->Resume(); break;
        default:
            logger->Error("[%04X] Unknown Audiostream's action: %d", opcode, action); break;
    }
}
CLEO_Fn(REMOVE_AUDIO_STREAM)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    if(!stream)
    {
        logger->Error("[%04X] Trying to remove zero audiostream", opcode);
        return;
    }
    soundsys->UnloadStream(stream);
}
CLEO_Fn(GET_AUDIO_STREAM_LENGTH)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleo->GetPointerToScriptVar(handle)->i = stream ? stream->GetLength() : -1;
}
CLEO_Fn(GET_AUDIO_STREAM_STATE)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleo->GetPointerToScriptVar(handle)->i = stream ? stream->GetState() : -1;
}
CLEO_Fn(GET_AUDIO_STREAM_VOLUME)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleo->GetPointerToScriptVar(handle)->f = stream ? stream->GetVolume() : 0.0f;
}
CLEO_Fn(SET_AUDIO_STREAM_VOLUME)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    if(stream) stream->SetVolume(cleo->ReadParam(handle)->f);
}
CLEO_Fn(SET_AUDIO_STREAM_LOOPED)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    if(stream) stream->SetLooping(cleo->ReadParam(handle)->i != 0);
}
CLEO_Fn(LOAD_3D_AUDIO_STREAM)
{
    char buf[256];
    cleoaddon->ReadString(handle, buf, sizeof(buf));
    int i = 0;
    while(buf[i] != 0) // A little hack
    {
        if(buf[i] == '\\') buf[i] = '/';
        ++i;
    }
    auto stream = soundsys->LoadStream(buf, true);
    cleo->GetPointerToScriptVar(handle)->u = (uint32_t)stream;
    cleoaddon->UpdateCompareFlag(handle, stream != NULL);
}
CLEO_Fn(SET_PLAY_3D_AUDIO_STREAM_AT_COORDS)
{
    C3DAudioStream* stream = (C3DAudioStream*)cleo->ReadParam(handle)->u;
    if(stream)
    {
        stream->Set3DPosition(cleo->ReadParam(handle)->f, cleo->ReadParam(handle)->f, cleo->ReadParam(handle)->f);
    }
}
CLEO_Fn(SET_PLAY_3D_AUDIO_STREAM_AT_OBJECT)
{
    C3DAudioStream* stream = (C3DAudioStream*)cleo->ReadParam(handle)->u;
    if(stream)
    {
        stream->Link(GetObjectFromRef(cleo->ReadParam(handle)->u));
    }
}
CLEO_Fn(SET_PLAY_3D_AUDIO_STREAM_AT_CHAR)
{
    C3DAudioStream* stream = (C3DAudioStream*)cleo->ReadParam(handle)->u;
    if(stream)
    {
        stream->Link(GetPedFromRef(cleo->ReadParam(handle)->u));
    }
}
CLEO_Fn(SET_PLAY_3D_AUDIO_STREAM_AT_CAR)
{
    C3DAudioStream* stream = (C3DAudioStream*)cleo->ReadParam(handle)->u;
    if(stream)
    {
        stream->Link(GetVehicleFromRef(cleo->ReadParam(handle)->u));
    }
}

// CLEO 5

CLEO_Fn(IS_AUDIO_STREAM_PLAYING)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    int state = (stream) ? stream->GetState() : -1;
    cleoaddon->UpdateCompareFlag(handle, state == 1);
}
CLEO_Fn(GET_AUDIO_STREAM_DURATION)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;

    float length = 0.0f;
    if (stream)
    {
        float length = stream->GetLength();
        float speed = stream->GetSpeed();
        if (speed <= 0.0f) length = FLT_MAX; // it would take forever to play paused
        else length /= speed; // speed corrected
    }
    cleo->GetPointerToScriptVar(handle)->f = length;
}
CLEO_Fn(GET_AUDIO_STREAM_SPEED)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleo->GetPointerToScriptVar(handle)->f = stream ? stream->GetSpeed() : 0.0f;
}
CLEO_Fn(SET_AUDIO_STREAM_SPEED)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    float speed = cleo->ReadParam(handle)->f;
    if(stream) stream->SetSpeed(speed);
}
CLEO_Fn(SET_AUDIO_STREAM_VOLUME_WITH_TRANSITION)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    float volume = cleo->ReadParam(handle)->f;
    int time = cleo->ReadParam(handle)->i;
    if(stream) stream->SetVolume(volume, 0.001f * time);
}
CLEO_Fn(SET_AUDIO_STREAM_SPEED_WITH_TRANSITION)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    float speed = cleo->ReadParam(handle)->f;
    int time = cleo->ReadParam(handle)->i;
    if(stream) stream->SetSpeed(speed, 0.001f * time);
}
CLEO_Fn(SET_AUDIO_STREAM_SOURCE_SIZE)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    float radius = cleo->ReadParam(handle)->f;
    if(stream)
    {
        if(radius == 0)
        {
            stream->SetMin3DRadius(3.0f);
            stream->SetMax3DRadius(1E+12f);
        }
        else if(radius > 0)
        {
            // ugly original logic...
            stream->SetMin3DRadius(radius);
        }
        else if(radius < 0)
        {
            // mobile has MAX radius (mhm)
            stream->SetMax3DRadius(-radius);
        }
    }
}
CLEO_Fn(GET_AUDIO_STREAM_PROGRESS)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleo->GetPointerToScriptVar(handle)->f = stream ? stream->GetProgress() : 0.0f;
}
CLEO_Fn(SET_AUDIO_STREAM_PROGRESS)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    float progress = cleo->ReadParam(handle)->f;
    if(stream) stream->SetProgress(progress);
}
CLEO_Fn(GET_AUDIO_STREAM_TYPE)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleo->GetPointerToScriptVar(handle)->i = stream ? stream->GetType() : eStreamType::None;
}
CLEO_Fn(SET_AUDIO_STREAM_TYPE)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    int newtype = cleo->ReadParam(handle)->i;
    if(stream) stream->SetType(newtype);
}
CLEO_Fn(GET_STREAM_TAKING_GAME_SPEED)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleoaddon->UpdateCompareFlag(handle, stream && stream->IsTakingGameSpeedIntoAccount());
}
CLEO_Fn(SET_STREAM_TAKING_GAME_SPEED)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    bool takeIt = cleo->ReadParam(handle)->i;
    if(stream) stream->SetTakeGameSpeedIntoAccount(takeIt);
}
CLEO_Fn(IS_AUDIO_STREAM_IN_3D)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleoaddon->UpdateCompareFlag(handle, stream && stream->Is3DSource());
}
CLEO_Fn(GET_AUDIO_STREAM_POSITION)
{
    C3DAudioStream* stream = (C3DAudioStream*)cleo->ReadParam(handle)->u;
    if(stream && stream->Is3DSource())
    {
        CVector pos = stream->GetPosition();
        cleo->GetPointerToScriptVar(handle)->f = pos.x;
        cleo->GetPointerToScriptVar(handle)->f = pos.y;
        cleo->GetPointerToScriptVar(handle)->f = pos.z;
    }
    else
    {
        cleo->GetPointerToScriptVar(handle)->f = 0.0f;
        cleo->GetPointerToScriptVar(handle)->f = 0.0f;
        cleo->GetPointerToScriptVar(handle)->f = 0.0f;
    }
}
CLEO_Fn(IS_AUDIO_STREAM_LINKED)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleoaddon->UpdateCompareFlag(handle, stream && stream->Is3DSource() && stream->IsLinked());
}
CLEO_Fn(IS_AUDIO_STREAM_VALID)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleoaddon->UpdateCompareFlag(handle, stream && soundsys->IsStreamInList(stream));
}
CLEO_Fn(GET_STREAM_DOPPLER_EFFECT)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleoaddon->UpdateCompareFlag(handle, stream && stream->GetDopplerEffect());
}
CLEO_Fn(SET_STREAM_DOPPLER_EFFECT)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    bool takeIt = cleo->ReadParam(handle)->i;
    if(stream) stream->SetDopplerEffect(takeIt);
}

// Hookies

DECL_HOOKv(PauseOpenAL, void* self, int doPause)
{
    PauseOpenAL(self, doPause);
    doPause ? soundsys->PauseStreams() : soundsys->ResumeStreams();
}
DECL_HOOKv(GameShutdown)
{
    GameShutdown();
    soundsys->UnloadAllStreams();
}
DECL_HOOKv(GameShutdownEngine)
{
    GameShutdownEngine();
    soundsys->UnloadAllStreams();
}
DECL_HOOKv(GameRestart)
{
    GameRestart();
    soundsys->UnloadAllStreams();
}
DECL_HOOK(void*, UpdateGameLogic, uintptr_t a1)
{
    soundsys->Update();
    return UpdateGameLogic(a1);
}
DECL_HOOKv(StartUserPause) // VC
{
    StartUserPause();
    soundsys->PauseStreams();
}
DECL_HOOKv(UpdateTimer) // VC
{
    UpdateTimer();
    soundsys->ResumeStreams();
}

void* AEAudioHardware;
float (*GetEffectsMasterScalingFactor)(void*);
float (*GetMusicMasterScalingFactor)(void*);
char* SampleManager_VC;
float GetEffectsVolume()
{
    if(nGameLoaded == 0) // GTA:SA
    {
        return GetEffectsMasterScalingFactor(AEAudioHardware);
    }
    else // GTA:VC ?
    {
        float val = (float)( *(SampleManager_VC + 0x8) ) / 127.0f;
        
        if(val > 1) return 1.0f;
        else if(val < 0) return 0.0f;
        else return val;
    }
}
float GetMusicVolume()
{
    if(nGameLoaded == 0) // GTA:SA
    {
        return GetMusicMasterScalingFactor(AEAudioHardware);
    }
    else // GTA:VC ?
    {
        float val = (float)( *(SampleManager_VC + 0x9) ) / 127.0f;
        
        if(val > 1) return 1.0f;
        else if(val < 0) return 0.0f;
        else return val;
    }
}

extern "C" void OnModLoad()
{
    logger->SetTag("[CLEO] AudioStreams");
    if(!(cleo = (cleo_ifs_t*)GetInterface("CLEO")))
    {
        logger->Error("Cannot start: CLEO interface is missing!");
        return;
    }
    if(!(cleoaddon = (cleo_addon_ifs_t*)GetInterface("CLEOAddon")))
    {
        logger->Error("Cannot start: CLEO's Addon interface is missing!");
        return;
    }
    if(!(BASS = (IBASS*)GetInterface("BASS")))
    {
        logger->Error("Cannot start: BASS interface is missing!");
        return;
    }

    logger->Info("Starting AudioStreams...");
    uintptr_t gameAddr = (uintptr_t)cleo->GetMainLibraryLoadAddress();
    SET_TO(camera, cleo->GetMainLibrarySymbol("TheCamera"));
    SET_TO(userPaused, cleo->GetMainLibrarySymbol("_ZN6CTimer11m_UserPauseE"));
    SET_TO(codePaused, cleo->GetMainLibrarySymbol("_ZN6CTimer11m_CodePauseE"));
    SET_TO(m_snTimeInMillisecondsNonClipped, cleo->GetMainLibrarySymbol("_ZN6CTimer32m_snTimeInMillisecondsNonClippedE"));
    SET_TO(m_snPreviousTimeInMillisecondsNonClipped, cleo->GetMainLibrarySymbol("_ZN6CTimer40m_snPreviousTimeInMillisecondsNonClippedE"));
    SET_TO(ms_fTimeScale, cleo->GetMainLibrarySymbol("_ZN6CTimer13ms_fTimeScaleE"));

         if((uintptr_t)camera == (uintptr_t)(gameAddr + 0x951FA8)) nGameLoaded = 0; // SA 2.00
    else if((uintptr_t)camera == (uintptr_t)(gameAddr + 0x595420)) nGameLoaded = 1; // VC 1.09
    else
    {
        logger->Info("The loaded game is not GTA:SA v2.00 or GTA:VC v1.09. Aborting...");
        return;
    }

    if(nGameLoaded == 0) // GTA:SA
    {
        HOOKPLT(UpdateGameLogic,    gameAddr + 0x66FE58);
        HOOKPLT(PauseOpenAL,        gameAddr + 0x674BE0);
        HOOKPLT(GameShutdown,       gameAddr + 0x672864);
        HOOKPLT(GameShutdownEngine, gameAddr + 0x6756F0);
        HOOKPLT(GameRestart,        gameAddr + 0x6731A0);

        SET_TO(AEAudioHardware, cleo->GetMainLibrarySymbol("AEAudioHardware"));
        SET_TO(GetEffectsMasterScalingFactor, cleo->GetMainLibrarySymbol("_ZN16CAEAudioHardware29GetEffectsMasterScalingFactorEv"));
        SET_TO(GetMusicMasterScalingFactor, cleo->GetMainLibrarySymbol("_ZN16CAEAudioHardware27GetMusicMasterScalingFactorEv"));
    }
    else // GTA:VC ?
    {
        HOOK(UpdateGameLogic,       gameAddr + 0x14C990 + 0x1);
        HOOKBL(StartUserPause,      gameAddr + 0x21E4C0 + 0x1);
        HOOKBL(UpdateTimer,         gameAddr + 0x21E3AE + 0x1);
        HOOK(GameShutdown,          gameAddr + 0x14C75C + 0x1);
        HOOK(GameShutdownEngine,    gameAddr + 0x14F078 + 0x1);
        HOOK(GameRestart,           gameAddr + 0x14CBB8 + 0x1);

        SET_TO(SampleManager_VC, cleo->GetMainLibrarySymbol("SampleManager"));
    }
    
    SET_TO(GetObjectFromRef, cleo->GetMainLibrarySymbol("_ZN6CPools9GetObjectEi"));
    SET_TO(GetPedFromRef, cleo->GetMainLibrarySymbol("_ZN6CPools6GetPedEi"));
    SET_TO(GetVehicleFromRef, cleo->GetMainLibrarySymbol("_ZN6CPools10GetVehicleEi"));
    SET_TO(Get_Just_Switched_Status, cleo->GetMainLibrarySymbol("_ZN7CCamera24Get_Just_Switched_StatusEv"));

    CLEO_RegisterOpcode(0x0AAC, LOAD_AUDIO_STREAM);
    CLEO_RegisterOpcode(0x0AAD, SET_AUDIO_STREAM_STATE);
    CLEO_RegisterOpcode(0x0AAE, REMOVE_AUDIO_STREAM);
    CLEO_RegisterOpcode(0x0AAF, GET_AUDIO_STREAM_LENGTH);
    CLEO_RegisterOpcode(0x0AB9, GET_AUDIO_STREAM_STATE);
    CLEO_RegisterOpcode(0x0ABB, GET_AUDIO_STREAM_VOLUME);
    CLEO_RegisterOpcode(0x0ABC, SET_AUDIO_STREAM_VOLUME);
    CLEO_RegisterOpcode(0x0AC0, SET_AUDIO_STREAM_LOOPED);
    CLEO_RegisterOpcode(0x0AC1, LOAD_3D_AUDIO_STREAM);
    CLEO_RegisterOpcode(0x0AC2, SET_PLAY_3D_AUDIO_STREAM_AT_COORDS);
    CLEO_RegisterOpcode(0x0AC3, SET_PLAY_3D_AUDIO_STREAM_AT_OBJECT);
    CLEO_RegisterOpcode(0x0AC4, SET_PLAY_3D_AUDIO_STREAM_AT_CHAR);
    CLEO_RegisterOpcode(0x0AC5, SET_PLAY_3D_AUDIO_STREAM_AT_CAR);

    // CLEO 5
    CLEO_RegisterOpcode(0x2500, IS_AUDIO_STREAM_PLAYING);
    CLEO_RegisterOpcode(0x2501, GET_AUDIO_STREAM_DURATION);
    CLEO_RegisterOpcode(0x2502, GET_AUDIO_STREAM_SPEED);
    CLEO_RegisterOpcode(0x2503, SET_AUDIO_STREAM_SPEED);
    CLEO_RegisterOpcode(0x2504, SET_AUDIO_STREAM_VOLUME_WITH_TRANSITION);
    CLEO_RegisterOpcode(0x2505, SET_AUDIO_STREAM_SPEED_WITH_TRANSITION);
    CLEO_RegisterOpcode(0x2506, SET_AUDIO_STREAM_SOURCE_SIZE);
    CLEO_RegisterOpcode(0x2507, GET_AUDIO_STREAM_PROGRESS);
    CLEO_RegisterOpcode(0x2508, SET_AUDIO_STREAM_PROGRESS);
    CLEO_RegisterOpcode(0x2509, GET_AUDIO_STREAM_TYPE);
    CLEO_RegisterOpcode(0x250A, SET_AUDIO_STREAM_TYPE);

    // Author's stuff (CLEO5 maintainer is a dumbo, im sorry...)
    CLEO_RegisterOpcode(0x2540, GET_STREAM_TAKING_GAME_SPEED); // 2540=1,does_game_speed_affect_stream %1d%
    CLEO_RegisterOpcode(0x2541, SET_STREAM_TAKING_GAME_SPEED); // 2541=2,set_stream %1d% being_affected_by_game_speed %2d%
    CLEO_RegisterOpcode(0x2542, IS_AUDIO_STREAM_IN_3D);        // 2542=1,is_audio_stream_3d %1d%
    CLEO_RegisterOpcode(0x2543, GET_AUDIO_STREAM_POSITION);    // 2543=4,get_audio_stream %1d% position %2d% %3d% %4d%
    CLEO_RegisterOpcode(0x2544, IS_AUDIO_STREAM_LINKED);       // 2544=1,is_audio_stream_linked %1d%
    CLEO_RegisterOpcode(0x2545, IS_AUDIO_STREAM_VALID);        // 2545=1,is_audio_stream_valid %1d%
    CLEO_RegisterOpcode(0x2546, GET_STREAM_DOPPLER_EFFECT);    // 2546=1,has_audio_stream_doppler_effect %1d%
    CLEO_RegisterOpcode(0x2547, SET_STREAM_DOPPLER_EFFECT);    // 2547=2,set_stream %1d% doppler_effect %2d%

    soundsys->Init();

    logger->Info("AudioStreams has been loaded!");
}
