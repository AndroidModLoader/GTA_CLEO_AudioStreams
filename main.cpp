#include <mod/amlmod.h>
#include <mod/logger.h>
#include "audiosystem.h"

#include "cleo.h"
cleo_ifs_t* cleo = NULL;
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
void        (*UpdateCompareFlag)(void*, uint8_t) = NULL;

MYMOD(net.alexblade.rusjj.audiostreams, CLEO AudioStreams, 1.2, Alexander Blade & RusJJ)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.cleolib, 2.0.1.4)
    ADD_DEPENDENCY(net.rusjj.basslib)
END_DEPLIST()

#define __decl_op(__name, __int)    const char* NAME_##__name = #__name; const uint16_t OP_##__name = __int;
#define __print_to_log(__str)       cleo->PrintToCleoLog(__str); logger->Info(__str)
#define __reg_opcode                cleo->RegisterOpcode
#define __reg_func                  cleo->RegisterOpcodeFunction
#define __handler_params            void *handle, uint32_t *ip, uint16_t opcode, const char *name
#define __op_name_match(x)          opcode == OP_##x || strcmp(name, NAME_##x) == 0
#define __reg_op_func(x, h)         __reg_opcode(OP_##x, h); __reg_func(NAME_##x, h);

__decl_op(LOAD_AUDIO_STREAM, 0x0AAC);                   // 0AAC=2,%2d% = load_audio_stream %1d%
__decl_op(SET_AUDIO_STREAM_STATE, 0x0AAD);              // 0AAD=2,set_audio_stream %1d% state %2d%
__decl_op(REMOVE_AUDIO_STREAM, 0x0AAE);                 // 0AAE=1,remove_audio_stream %1d%
__decl_op(GET_AUDIO_STREAM_LENGTH, 0x0AAF);             // 0AAF=2,%2d% = get_audio_stream_length %1d%
__decl_op(GET_AUDIO_STREAM_STATE, 0x0AB9);              // 0AB9=2,get_audio_stream %1d% state_to %2d%
__decl_op(GET_AUDIO_STREAM_VOLUME, 0x0ABB);             // 0ABB=2,%2d% = audio_stream %1d% volume
__decl_op(SET_AUDIO_STREAM_VOLUME, 0x0ABC);             // 0ABC=2,set_audio_stream %1d% volume %2d%
__decl_op(SET_AUDIO_STREAM_LOOPED, 0x0AC0);             // 0AC0=2,set_audio_stream %1d% looped %2d%
__decl_op(LOAD_3D_AUDIO_STREAM, 0x0AC1);                // 0AC1=2,%2d% = load_audio_stream_with_3d_support %1d% ; IF and SET
__decl_op(SET_PLAY_3D_AUDIO_STREAM_AT_COORDS, 0x0AC2);  // 0AC2=4,link_3d_audio_stream %1d% at_coords %2d% %3d% %4d%
__decl_op(SET_PLAY_3D_AUDIO_STREAM_AT_OBJECT, 0x0AC3);  // 0AC3=2,link_3d_audio_stream %1d% to_object %2d%
__decl_op(SET_PLAY_3D_AUDIO_STREAM_AT_CHAR, 0x0AC4);    // 0AC4=2,link_3d_audio_stream %1d% to_actor %2d%
__decl_op(SET_PLAY_3D_AUDIO_STREAM_AT_CAR, 0x0AC5);     // 0AC5=2,link_3d_audio_stream %1d% to_car %2d%

inline int GetPCOffset()
{
    switch(cleo->GetGameIdentifier())
    {
        case GTASA: return 20;
        case GTALCS: return 24;

        default: return 16;
    }
}
inline uint8_t*& GetPC(void* handle)
{
    return *(uint8_t**)((uintptr_t)handle + GetPCOffset());
}
inline uint8_t* GetPC_CLEO(void* handle) // weird-ass trash from CLEO for VC *facepalm*
{
    return (uint8_t*)cleo->GetRealCodePointer(*(uint32_t*)((uintptr_t)handle + GetPCOffset()));
}
inline char* CLEO_ReadStringEx(void* handle, char* buf, size_t size)
{
    uint8_t byte = *(cleo->GetGameIdentifier() == GTASA ? GetPC(handle) : GetPC_CLEO(handle));

    static char newBuf[128];
    if(!buf || size < 1) buf = (char*)newBuf;

    switch(byte)
    {
        default:
            return cleo->ReadStringLong(handle, buf, size) ? buf : NULL;
            
        case 0x09:
            GetPC(handle) += 1;
            return cleo->ReadString8byte(handle, buf, size) ? buf : NULL;

        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        {
            size = (size > 8) ? 8 : size;
            memcpy(buf, (char*)cleo->GetPointerToScriptVar(handle), size);
            buf[size-1] = 0;
            return buf;
        }

        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        {
            size = (size > 16) ? 16 : size;
            memcpy(buf, (char*)cleo->GetPointerToScriptVar(handle), size);
            buf[size-1] = 0;
            return buf;
        }
    }
    return buf;
}

void LOAD_AUDIO_STREAM(__handler_params)
{
    char param1[256];
    CLEO_ReadStringEx(handle, param1, sizeof(param1));
    param1[sizeof(param1)-1] = 0; // I can't trust game scripting engine...
    int i = 0;
    while(param1[i] != 0) // A little hack
    {
        if(param1[i] == '\\') param1[i] = '/';
        ++i;
    }
    cleo->GetPointerToScriptVar(handle)->u = (uint32_t)soundsys->LoadStream(param1);
}

void SET_AUDIO_STREAM_STATE(__handler_params)
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

void REMOVE_AUDIO_STREAM(__handler_params)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    if(!stream)
    {
        logger->Error("[%04X] Trying to remove zero audiostream", opcode);
        return;
    }
    soundsys->UnloadStream(stream);
}

void GET_AUDIO_STREAM_LENGTH(__handler_params)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleo->GetPointerToScriptVar(handle)->i = stream ? stream->GetLength() : -1;
}

void GET_AUDIO_STREAM_STATE(__handler_params)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleo->GetPointerToScriptVar(handle)->i = stream ? stream->GetState() : -1;
}

void GET_AUDIO_STREAM_VOLUME(__handler_params)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    cleo->GetPointerToScriptVar(handle)->f = stream ? stream->GetVolume() : 0.0f;
}

void SET_AUDIO_STREAM_VOLUME(__handler_params)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    if(stream) stream->SetVolume(cleo->ReadParam(handle)->f);
}

void SET_AUDIO_STREAM_LOOPED(__handler_params)
{
    CAudioStream* stream = (CAudioStream*)cleo->ReadParam(handle)->u;
    if(stream) stream->Loop(cleo->ReadParam(handle)->i != 0);
}

void LOAD_3D_AUDIO_STREAM(__handler_params)
{
    char param1[256];
    CLEO_ReadStringEx(handle, param1, sizeof(param1));
    param1[sizeof(param1)-1] = 0; // I can't trust game scripting engine...
    int i = 0;
    while(param1[i] != 0) // A little hack
    {
        if(param1[i] == '\\') param1[i] = '/';
        ++i;
    }
    auto stream = soundsys->LoadStream(param1, true);
    cleo->GetPointerToScriptVar(handle)->u = (uint32_t)stream;
    UpdateCompareFlag(handle, stream != NULL);
}

void SET_PLAY_3D_AUDIO_STREAM_AT_COORDS(__handler_params)
{
    C3DAudioStream* stream = (C3DAudioStream*)cleo->ReadParam(handle)->u;
    if(stream)
    {
        stream->Set3DPosition(cleo->ReadParam(handle)->f, cleo->ReadParam(handle)->f, cleo->ReadParam(handle)->f);
    }
}

void SET_PLAY_3D_AUDIO_STREAM_AT_OBJECT(__handler_params)
{
    C3DAudioStream* stream = (C3DAudioStream*)cleo->ReadParam(handle)->u;
    if(stream)
    {
        stream->Link(GetObjectFromRef(cleo->ReadParam(handle)->u));
    }
}

void SET_PLAY_3D_AUDIO_STREAM_AT_CHAR(__handler_params)
{
    C3DAudioStream* stream = (C3DAudioStream*)cleo->ReadParam(handle)->u;
    if(stream)
    {
        stream->Link(GetPedFromRef(cleo->ReadParam(handle)->u));
    }
}

void SET_PLAY_3D_AUDIO_STREAM_AT_CAR(__handler_params)
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
    if(!(BASS = (IBASS*)GetInterface("BASS")))
    {
        logger->Error("Cannot load: BASS interface is not found!");
        return;
    }

    __print_to_log("Starting AudioStreams...");
    uintptr_t gameAddr = (uintptr_t)cleo->GetMainLibraryLoadAddress();
    SET_TO(camera, cleo->GetMainLibrarySymbol("TheCamera"));
    SET_TO(userPaused, cleo->GetMainLibrarySymbol("_ZN6CTimer11m_UserPauseE"));
    SET_TO(codePaused, cleo->GetMainLibrarySymbol("_ZN6CTimer11m_CodePauseE"));

    if((uintptr_t)camera == gameAddr + 0x951FA8) nGameLoaded = 0; // SA 2.00
    else if((uintptr_t)camera == gameAddr + 0x595420) nGameLoaded = 1; // VC 1.09
    else
    {
        __print_to_log("The loaded game is not GTA:SA v2.00 or GTA:VC v1.09. Aborting...");
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
    SET_TO(UpdateCompareFlag, cleo->GetMainLibrarySymbol("_ZN14CRunningScript17UpdateCompareFlagEh"));

    __reg_op_func(LOAD_AUDIO_STREAM, LOAD_AUDIO_STREAM);
    __reg_op_func(SET_AUDIO_STREAM_STATE, SET_AUDIO_STREAM_STATE);
    __reg_op_func(REMOVE_AUDIO_STREAM, REMOVE_AUDIO_STREAM);
    __reg_op_func(GET_AUDIO_STREAM_LENGTH, GET_AUDIO_STREAM_LENGTH);
    __reg_op_func(GET_AUDIO_STREAM_STATE, GET_AUDIO_STREAM_STATE);
    __reg_op_func(GET_AUDIO_STREAM_VOLUME, GET_AUDIO_STREAM_VOLUME);
    __reg_op_func(SET_AUDIO_STREAM_VOLUME, SET_AUDIO_STREAM_VOLUME);
    __reg_op_func(SET_AUDIO_STREAM_LOOPED, SET_AUDIO_STREAM_LOOPED);
    __reg_op_func(LOAD_3D_AUDIO_STREAM, LOAD_3D_AUDIO_STREAM);
    __reg_op_func(SET_PLAY_3D_AUDIO_STREAM_AT_COORDS, SET_PLAY_3D_AUDIO_STREAM_AT_COORDS);
    __reg_op_func(SET_PLAY_3D_AUDIO_STREAM_AT_OBJECT, SET_PLAY_3D_AUDIO_STREAM_AT_OBJECT);
    __reg_op_func(SET_PLAY_3D_AUDIO_STREAM_AT_CHAR, SET_PLAY_3D_AUDIO_STREAM_AT_CHAR);
    __reg_op_func(SET_PLAY_3D_AUDIO_STREAM_AT_CAR, SET_PLAY_3D_AUDIO_STREAM_AT_CAR);

    soundsys->Init();

    __print_to_log("AudioStreams has been loaded!");
}
