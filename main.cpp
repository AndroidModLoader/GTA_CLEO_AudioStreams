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

int nGameLoaded = -1;

CObject*    (*GetObjectFromRef)(int) = NULL;
CPed*       (*GetPedFromRef)(int) = NULL;
CVehicle*   (*GetVehicleFromRef)(int) = NULL;

MYMOD(net.alexblade.rusjj.audiostreams, CLEO AudioStreams, 1.3, Alexander Blade & RusJJ)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.cleolib, 2.0.1.6)
    ADD_DEPENDENCY(net.rusjj.basslib)
END_DEPLIST()

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
    cleo->GetPointerToScriptVar(handle)->u = (uint32_t)soundsys->LoadStream(buf);
}

CLEO_Fn(SET_AUDIO_STREAM_STATE)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    if(!stream)
    {
        logger->Error("[%04X] Trying to do an action on zero audiostream", opcode);
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
    if(stream) stream->Loop(cleo->ReadParam(handle)->i != 0);
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

extern "C" void OnModLoad()
{
    logger->SetTag("[CLEO] AudioStreams");
    if(!(cleo = (cleo_ifs_t*)GetInterface("CLEO")))
    {
        logger->Error("Cannot load: CLEO interface is not found!");
        return;
    }
    if(!(cleoaddon = (cleo_addon_ifs_t*)GetInterface("CLEOAddon")))
    {
        logger->Error("Cannot load a mod: CLEO's Addon interface is unknown!");
        return;
    }
    if(!(BASS = (IBASS*)GetInterface("BASS")))
    {
        logger->Error("Cannot load: BASS interface is not found!");
        return;
    }

    logger->Info("Starting AudioStreams...");
    uintptr_t gameAddr = (uintptr_t)cleo->GetMainLibraryLoadAddress();
    SET_TO(camera, cleo->GetMainLibrarySymbol("TheCamera"));
    SET_TO(userPaused, cleo->GetMainLibrarySymbol("_ZN6CTimer11m_UserPauseE"));
    SET_TO(codePaused, cleo->GetMainLibrarySymbol("_ZN6CTimer11m_CodePauseE"));

    if((uintptr_t)camera == gameAddr + 0x951FA8) nGameLoaded = 0; // SA 2.00
    else if((uintptr_t)camera == gameAddr + 0x595420) nGameLoaded = 1; // VC 1.09
    else
    {
        logger->Info("The loaded game is not GTA:SA v2.00 or GTA:VC v1.09. Aborting...");
        return;
    }

    if(nGameLoaded == 0) // GTA:SA
    {
        HOOKPLT(UpdateGameLogic, gameAddr + 0x66FE58);
        HOOKPLT(PauseOpenAL, gameAddr + 0x674BE0);
        HOOKPLT(GameShutdown, gameAddr + 0x672864);
        HOOKPLT(GameShutdownEngine, gameAddr + 0x6756F0);
        HOOKPLT(GameRestart, gameAddr + 0x6731A0);
    }
    else // GTA:VC ?
    {
        HOOKBL(UpdateGameLogic, gameAddr + 0x14ECD2 + 0x1);
        HOOKBL(StartUserPause, gameAddr + 0x21E4C0 + 0x1);
        HOOKBL(UpdateTimer, gameAddr + 0x21E3AE + 0x1);
        HOOKBL(GameShutdown, gameAddr + 0x21E02E + 0x1); HOOKBL(GameShutdown, gameAddr + 0x21E9B4 + 0x1);
        HOOKBL(GameShutdownEngine, gameAddr + 0x14F078 + 0x1);
        HOOKBL(GameRestart, gameAddr + 0x14CCA6 + 0x1); HOOKBL(GameRestart, gameAddr + 0x21E8AA + 0x1);
    }
    

    SET_TO(GetObjectFromRef, cleo->GetMainLibrarySymbol("_ZN6CPools9GetObjectEi"));
    SET_TO(GetPedFromRef, cleo->GetMainLibrarySymbol("_ZN6CPools6GetPedEi"));
    SET_TO(GetVehicleFromRef, cleo->GetMainLibrarySymbol("_ZN6CPools10GetVehicleEi"));

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

    soundsys->Init();

    logger->Info("AudioStreams has been loaded!");
}