#ifndef BASS_INTERFACE_H
#define BASS_INTERFACE_H

#include <stdint.h>

typedef struct BASS_3DVECTOR
{
#ifdef __cplusplus
	BASS_3DVECTOR() {};
	BASS_3DVECTOR(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {};
#endif
	float x;	// + = right, - = left
	float y;	// + = up, - = down
	float z;	// + = front, - = behind
} BASS_3DVECTOR;

typedef struct {
	uint32_t freq;		// default playback rate
	float volume;	// default volume (0-1)
	float pan;		// default pan (-1=left, 0=middle, 1=right)
	uint32_t flags;	// BASS_SAMPLE_xxx flags
	uint32_t length;	// length (in bytes)
	uint32_t max;		// maximum simultaneous playbacks
	uint32_t origres;	// original resolution
	uint32_t chans;	// number of channels
	uint32_t mingap;	// minimum gap (ms) between creating channels
	uint32_t mode3d;	// BASS_3DMODE_xxx mode
	float mindist;	// minimum distance
	float maxdist;	// maximum distance
	uint32_t iangle;	// angle of inside projection cone
	uint32_t oangle;	// angle of outside projection cone
	float outvol;	// delta-volume outside the projection cone
	uint32_t vam;		// unused
	uint32_t priority;	// unused
} BASS_SAMPLE;

typedef struct {
	uint32_t freq;		// default playback rate
	uint32_t chans;	// channels
	uint32_t flags;
	uint32_t ctype;	// type of channel
	uint32_t origres;	// original resolution
	uint32_t plugin;
	uint32_t sample;
	const char *filename;
} BASS_CHANNELINFO;

typedef struct {
	const char *name;	// description
	const char *driver;	// driver
	uint32_t flags;
} BASS_DEVICEINFO;

typedef struct {
	uint32_t flags;	// device capabilities (DSCCAPS_xxx flags)
	uint32_t formats;	// supported standard formats (WAVE_FORMAT_xxx flags)
	uint32_t inputs;	// number of inputs
	bool singlein;	// TRUE = only 1 input can be set at a time
	uint32_t freq;		// current input rate
} BASS_RECORDINFO;

typedef struct {
	uint32_t flags;	// device capabilities (DSCAPS_xxx flags)
	uint32_t hwsize;	// unused
	uint32_t hwfree;	// unused
	uint32_t freesam;	// unused
	uint32_t free3d;	// unused
	uint32_t minrate;	// unused
	uint32_t maxrate;	// unused
	bool eax;		// unused
	uint32_t minbuf;	// recommended minimum buffer length in ms
	uint32_t dsver;	// DirectSound version
	uint32_t latency;	// average delay (in ms) before start of playback
	uint32_t initflags; // BASS_Init "flags" parameter
	uint32_t speakers; // number of speakers available
	uint32_t freq;		// current output rate
} BASS_INFO;

typedef uint32_t (STREAMPROC)(uint32_t handle, void *buffer, uint32_t length, void *user);
typedef void (FILECLOSEPROC)(void *user);
typedef uint64_t (FILELENPROC)(void *user);
typedef uint32_t (FILEREADPROC)(void *buffer, uint32_t length, void *user);
typedef bool (FILESEEKPROC)(uint64_t offset, void *user);
typedef void (DOWNLOADPROC)(const void *buffer, uint32_t length, void *user);
typedef void (SYNCPROC)(uint32_t handle, uint32_t channel, uint32_t data, void *user);
typedef void (DSPPROC)(uint32_t handle, uint32_t channel, void *buffer, uint32_t length, void *user);
typedef bool (RECORDPROC)(uint32_t handle, const void *buffer, uint32_t length, void *user);
typedef void (IOSNOTIFYPROC)(uint32_t status); // That's for iOS but who cares, lol



class IBASS
{
public:
	virtual void Apply3D(void) = 0;
	virtual double ChannelBytes2Seconds(uint32_t handle, uint64_t pos) = 0;
	virtual uint32_t ChannelFlags(uint32_t handle, uint32_t flags, uint32_t mask) = 0;
	virtual bool ChannelFree(uint32_t handle) = 0;
	virtual bool ChannelGet3DAttributes(uint32_t handle, uint32_t* mode, float* min, float* max, uint32_t* iangle, uint32_t* oangle, float* outvol) = 0;
	virtual bool ChannelGet3DPosition(uint32_t handle, BASS_3DVECTOR* pos, BASS_3DVECTOR* orient, BASS_3DVECTOR* vel) = 0;
	virtual bool ChannelGetAttribute(uint32_t handle, uint32_t attrib, float* value) = 0;
	virtual uint32_t ChannelGetAttributeEx(uint32_t handle, uint32_t attrib, void* value, uint32_t size) = 0;
	virtual uint32_t ChannelGetData(uint32_t handle, void* buffer, uint32_t length) = 0;
	virtual uint32_t ChannelGetDevice(uint32_t handle) = 0;
	virtual bool ChannelGetInfo(uint32_t handle, BASS_CHANNELINFO* info) = 0;
	virtual uint64_t ChannelGetLength(uint32_t handle, uint32_t mode) = 0;
	virtual uint32_t ChannelGetLevel(uint32_t handle) = 0;
	virtual bool ChannelGetLevelEx(uint32_t handle, float* levels, float length, uint32_t flags) = 0;
	virtual uint64_t ChannelGetPosition(uint32_t handle, uint32_t mode) = 0;
	virtual const char* ChannelGetTags(uint32_t handle, uint32_t tags) = 0;
	virtual uint32_t ChannelIsActive(uint32_t handle) = 0;
	virtual bool ChannelIsSliding(uint32_t handle, uint32_t attrib) = 0;
	virtual bool ChannelLock(uint32_t handle, bool lock) = 0;
	virtual bool ChannelPause(uint32_t handle) = 0;
	virtual bool ChannelPlay(uint32_t handle, bool restart) = 0;
	virtual bool ChannelRemoveDSP(uint32_t handle, uint32_t dsp) = 0;
	virtual bool ChannelRemoveFX(uint32_t handle, uint32_t fx) = 0;
	virtual bool ChannelRemoveLink(uint32_t handle, uint32_t chan) = 0;
	virtual bool ChannelRemoveSync(uint32_t handle, uint32_t sync) = 0;
	virtual uint64_t ChannelSeconds2Bytes(uint32_t handle, double pos) = 0;
	virtual bool ChannelSet3DAttributes(uint32_t handle, int mode, float min, float max, int iangle, int oangle, float outvol) = 0;
	virtual bool ChannelSet3DPosition(uint32_t handle, const BASS_3DVECTOR* pos, const BASS_3DVECTOR* orient, const BASS_3DVECTOR* vel) = 0;
	virtual bool ChannelSetAttribute(uint32_t handle, uint32_t attrib, float value) = 0;
	virtual bool ChannelSetAttributeEx(uint32_t handle, uint32_t attrib, void* value, uint32_t size) = 0;
	virtual uint32_t ChannelSetDSP(uint32_t handle, DSPPROC* proc, void* user, int priority) = 0;
	virtual bool ChannelSetDevice(uint32_t handle, uint32_t device) = 0;
	virtual uint32_t ChannelSetFX(uint32_t handle, uint32_t type, int priority) = 0;
	virtual bool ChannelSetLink(uint32_t handle, uint32_t chan) = 0;
	virtual bool ChannelSetPosition(uint32_t handle, uint64_t pos, uint32_t mode) = 0;
	virtual uint32_t ChannelSetSync(uint32_t handle, uint32_t type, uint64_t param, SYNCPROC* proc, void* user) = 0;
	virtual bool ChannelSlideAttribute(uint32_t handle, uint32_t attrib, float value, uint32_t time) = 0;
	virtual bool ChannelStop(uint32_t handle) = 0;
	virtual bool ChannelUpdate(uint32_t handle, uint32_t length) = 0;
	virtual int ErrorGetCode(void) = 0;
	virtual bool FXGetParameters(uint32_t handle, void *params) = 0;
	virtual bool FXReset(uint32_t handle) = 0;
	virtual bool FXSetParameters(uint32_t handle, const void *params) = 0;
	virtual bool FXSetPriority(uint32_t handle, int priority) = 0;
	virtual bool Free(void) = 0;
	virtual bool Get3DFactors(float *distf, float *rollf, float *doppf) = 0;
	virtual bool Get3DPosition(BASS_3DVECTOR *pos, BASS_3DVECTOR *vel, BASS_3DVECTOR *front, BASS_3DVECTOR *top) = 0;
	virtual float GetCPU(void) = 0;
	virtual uint32_t GetConfig(uint32_t option) = 0;
	virtual const void* GetConfigPtr(uint32_t option) = 0;
	virtual uint32_t GetDevice(void) = 0;
	virtual bool GetDeviceInfo(uint32_t device, BASS_DEVICEINFO *info) = 0;
	virtual bool GetInfo(BASS_INFO *info) = 0;
	virtual uint32_t GetVersion(void) = 0;
	virtual float GetVolume(void) = 0;
	virtual bool Init(int device, uint32_t freq, uint32_t flags) = 0;
	virtual uint32_t IsStarted(void) = 0;
	virtual bool MusicFree(uint32_t handle) = 0;
	virtual uint32_t MusicLoad(bool mem, const void* file, uint64_t offset, uint32_t length, uint32_t flags, uint32_t freq) = 0;
	virtual bool Pause(void) = 0;
	virtual uint32_t SampleCreate(uint32_t length, uint32_t freq, uint32_t chans, uint32_t max, uint32_t flags) = 0;
	virtual bool SampleFree(uint32_t handle) = 0;
	virtual uint32_t SampleGetChannel(uint32_t handle, uint32_t flags) = 0;
	virtual uint32_t SampleGetChannels(uint32_t handle, uint32_t *channels) = 0;
	virtual bool SampleGetData(uint32_t handle, void *buffer) = 0;
	virtual bool SampleGetInfo(uint32_t handle, BASS_SAMPLE *info) = 0;
	virtual uint32_t SampleLoad(bool mem, const void *file, uint64_t offset, uint32_t length, uint32_t max, uint32_t flags) = 0;
	virtual bool SampleSetData(uint32_t handle, const void *buffer) = 0;
	virtual bool SampleSetInfo(uint32_t handle, const BASS_SAMPLE *info) = 0;
	virtual bool SampleStop(uint32_t handle) = 0;
	virtual bool Set3DFactors(float distf, float rollf, float doppf) = 0;
	virtual bool Set3DPosition(const BASS_3DVECTOR *pos, const BASS_3DVECTOR *vel, const BASS_3DVECTOR *front, const BASS_3DVECTOR *top) = 0;
	virtual bool SetConfig(uint32_t option, uint32_t value) = 0;
	virtual bool SetConfigPtr(uint32_t option, const void* value) = 0;
	virtual bool SetDevice(uint32_t device) = 0;
	virtual bool SetVolume(float volume) = 0;
	virtual bool Start(void) = 0;
	virtual bool Stop(void) = 0;
	virtual uint32_t StreamCreate(uint32_t freq, uint32_t chans, uint32_t flags, STREAMPROC *proc, void *user) = 0;
	virtual uint32_t StreamCreateFile(bool mem, const void *file, uint64_t offset, uint64_t length, uint32_t flags) = 0;
	virtual uint32_t StreamCreateURL(const char* url, uint32_t offset, uint32_t flags, DOWNLOADPROC* proc) = 0;
	virtual bool StreamFree(uint32_t handle) = 0;
	virtual uint64_t StreamGetFilePosition(uint32_t handle, uint32_t mode) = 0;
	virtual uint32_t StreamPutData(uint32_t handle, const void *buffer, uint32_t length) = 0;
	virtual uint32_t StreamPutFileData(uint32_t handle, const void *buffer, uint32_t length) = 0;
	virtual bool Update(uint32_t length) = 0;
};



// Error codes returned by BASS_ErrorGetCode
#define BASS_OK				0	// all is OK
#define BASS_ERROR_MEM		1	// memory error
#define BASS_ERROR_FILEOPEN	2	// can't open the file
#define BASS_ERROR_DRIVER	3	// can't find a free/valid driver
#define BASS_ERROR_BUFLOST	4	// the sample buffer was lost
#define BASS_ERROR_HANDLE	5	// invalid handle
#define BASS_ERROR_FORMAT	6	// unsupported sample format
#define BASS_ERROR_POSITION	7	// invalid position
#define BASS_ERROR_INIT		8	// BASS_Init has not been successfully called
#define BASS_ERROR_START	9	// BASS_Start has not been successfully called
#define BASS_ERROR_SSL		10	// SSL/HTTPS support isn't available
#define BASS_ERROR_REINIT	11	// device needs to be reinitialized
#define BASS_ERROR_ALREADY	14	// already initialized/paused/whatever
#define BASS_ERROR_NOTAUDIO	17	// file does not contain audio
#define BASS_ERROR_NOCHAN	18	// can't get a free channel
#define BASS_ERROR_ILLTYPE	19	// an illegal type was specified
#define BASS_ERROR_ILLPARAM	20	// an illegal parameter was specified
#define BASS_ERROR_NO3D		21	// no 3D support
#define BASS_ERROR_NOEAX	22	// no EAX support
#define BASS_ERROR_DEVICE	23	// illegal device number
#define BASS_ERROR_NOPLAY	24	// not playing
#define BASS_ERROR_FREQ		25	// illegal sample rate
#define BASS_ERROR_NOTFILE	27	// the stream is not a file stream
#define BASS_ERROR_NOHW		29	// no hardware voices available
#define BASS_ERROR_EMPTY	31	// the MOD music has no sequence data
#define BASS_ERROR_NONET	32	// no internet connection could be opened
#define BASS_ERROR_CREATE	33	// couldn't create the file
#define BASS_ERROR_NOFX		34	// effects are not available
#define BASS_ERROR_NOTAVAIL	37	// requested data/action is not available
#define BASS_ERROR_DECODE	38	// the channel is/isn't a "decoding channel"
#define BASS_ERROR_DX		39	// a sufficient DirectX version is not installed
#define BASS_ERROR_TIMEOUT	40	// connection timedout
#define BASS_ERROR_FILEFORM	41	// unsupported file format
#define BASS_ERROR_SPEAKER	42	// unavailable speaker
#define BASS_ERROR_VERSION	43	// invalid BASS version (used by add-ons)
#define BASS_ERROR_CODEC	44	// codec is not available/supported
#define BASS_ERROR_ENDED	45	// the channel/file has ended
#define BASS_ERROR_BUSY		46	// the device is busy
#define BASS_ERROR_UNSTREAMABLE	47	// unstreamable file
#define BASS_ERROR_PROTOCOL	48	// unsupported protocol
#define BASS_ERROR_UNKNOWN	-1	// some other mystery problem

// BASS_SetConfig options
#define BASS_CONFIG_BUFFER			0
#define BASS_CONFIG_UPDATEPERIOD	1
#define BASS_CONFIG_GVOL_SAMPLE		4
#define BASS_CONFIG_GVOL_STREAM		5
#define BASS_CONFIG_GVOL_MUSIC		6
#define BASS_CONFIG_CURVE_VOL		7
#define BASS_CONFIG_CURVE_PAN		8
#define BASS_CONFIG_FLOATDSP		9
#define BASS_CONFIG_3DALGORITHM		10
#define BASS_CONFIG_NET_TIMEOUT		11
#define BASS_CONFIG_NET_BUFFER		12
#define BASS_CONFIG_PAUSE_NOPLAY	13
#define BASS_CONFIG_NET_PREBUF		15
#define BASS_CONFIG_NET_PASSIVE		18
#define BASS_CONFIG_REC_BUFFER		19
#define BASS_CONFIG_NET_PLAYLIST	21
#define BASS_CONFIG_MUSIC_VIRTUAL	22
#define BASS_CONFIG_VERIFY			23
#define BASS_CONFIG_UPDATETHREADS	24
#define BASS_CONFIG_DEV_BUFFER		27
#define BASS_CONFIG_REC_LOOPBACK	28
#define BASS_CONFIG_VISTA_TRUEPOS	30
#define BASS_CONFIG_IOS_SESSION		34
#define BASS_CONFIG_IOS_MIXAUDIO	34
#define BASS_CONFIG_DEV_DEFAULT		36
#define BASS_CONFIG_NET_READTIMEOUT	37
#define BASS_CONFIG_VISTA_SPEAKERS	38
#define BASS_CONFIG_IOS_SPEAKER		39
#define BASS_CONFIG_MF_DISABLE		40
#define BASS_CONFIG_HANDLES			41
#define BASS_CONFIG_UNICODE			42
#define BASS_CONFIG_SRC				43
#define BASS_CONFIG_SRC_SAMPLE		44
#define BASS_CONFIG_ASYNCFILE_BUFFER 45
#define BASS_CONFIG_OGG_PRESCAN		47
#define BASS_CONFIG_MF_VIDEO		48
#define BASS_CONFIG_AIRPLAY			49
#define BASS_CONFIG_DEV_NONSTOP		50
#define BASS_CONFIG_IOS_NOCATEGORY	51
#define BASS_CONFIG_VERIFY_NET		52
#define BASS_CONFIG_DEV_PERIOD		53
#define BASS_CONFIG_FLOAT			54
#define BASS_CONFIG_NET_SEEK		56
#define BASS_CONFIG_AM_DISABLE		58
#define BASS_CONFIG_NET_PLAYLIST_DEPTH	59
#define BASS_CONFIG_NET_PREBUF_WAIT	60
#define BASS_CONFIG_ANDROID_SESSIONID	62
#define BASS_CONFIG_WASAPI_PERSIST	65
#define BASS_CONFIG_REC_WASAPI		66
#define BASS_CONFIG_ANDROID_AAUDIO	67
#define BASS_CONFIG_SAMPLE_ONEHANDLE	69
#define BASS_CONFIG_DEV_TIMEOUT		70
#define BASS_CONFIG_NET_META		71
#define BASS_CONFIG_NET_RESTRATE	72

// BASS_SetConfigPtr options
#define BASS_CONFIG_NET_AGENT		16
#define BASS_CONFIG_NET_PROXY		17
#define BASS_CONFIG_IOS_NOTIFY		46
#define BASS_CONFIG_LIBSSL			64

#define BASS_CONFIG_THREAD			0x40000000 // flag: thread-specific setting

// BASS_CONFIG_IOS_SESSION flags
#define BASS_IOS_SESSION_MIX		1
#define BASS_IOS_SESSION_DUCK		2
#define BASS_IOS_SESSION_AMBIENT	4
#define BASS_IOS_SESSION_SPEAKER	8
#define BASS_IOS_SESSION_DISABLE	16

// BASS_Init flags
#define BASS_DEVICE_8BITS		1		// unused
#define BASS_DEVICE_MONO		2		// mono
#define BASS_DEVICE_3D			4		// unused
#define BASS_DEVICE_16BITS		8		// limit output to 16-bit
#define BASS_DEVICE_REINIT		128		// reinitialize
#define BASS_DEVICE_LATENCY		0x100	// unused
#define BASS_DEVICE_CPSPEAKERS	0x400	// unused
#define BASS_DEVICE_SPEAKERS	0x800	// force enabling of speaker assignment
#define BASS_DEVICE_NOSPEAKER	0x1000	// ignore speaker arrangement
#define BASS_DEVICE_DMIX		0x2000	// use ALSA "dmix" plugin
#define BASS_DEVICE_FREQ		0x4000	// set device sample rate
#define BASS_DEVICE_STEREO		0x8000	// limit output to stereo
#define BASS_DEVICE_HOG			0x10000	// hog/exclusive mode
#define BASS_DEVICE_AUDIOTRACK	0x20000	// use AudioTrack output
#define BASS_DEVICE_DSOUND		0x40000	// use DirectSound output
#define BASS_DEVICE_SOFTWARE	0x80000	// disable hardware/fastpath output

// DirectSound interfaces (for use with BASS_GetDSoundObject)
#define BASS_OBJECT_DS		1	// IDirectSound
#define BASS_OBJECT_DS3DL	2	// IDirectSound3DListener

// BASS_DEVICEINFO flags
#define BASS_DEVICE_ENABLED		1
#define BASS_DEVICE_DEFAULT		2
#define BASS_DEVICE_INIT		4
#define BASS_DEVICE_LOOPBACK	8
#define BASS_DEVICE_DEFAULTCOM	128

#define BASS_DEVICE_TYPE_MASK			0xff000000
#define BASS_DEVICE_TYPE_NETWORK		0x01000000
#define BASS_DEVICE_TYPE_SPEAKERS		0x02000000
#define BASS_DEVICE_TYPE_LINE			0x03000000
#define BASS_DEVICE_TYPE_HEADPHONES		0x04000000
#define BASS_DEVICE_TYPE_MICROPHONE		0x05000000
#define BASS_DEVICE_TYPE_HEADSET		0x06000000
#define BASS_DEVICE_TYPE_HANDSET		0x07000000
#define BASS_DEVICE_TYPE_DIGITAL		0x08000000
#define BASS_DEVICE_TYPE_SPDIF			0x09000000
#define BASS_DEVICE_TYPE_HDMI			0x0a000000
#define BASS_DEVICE_TYPE_DISPLAYPORT	0x40000000

// BASS_GetDeviceInfo flags
#define BASS_DEVICES_AIRPLAY	0x1000000

#define BASS_SAMPLE_8BITS		1	// 8 bit
#define BASS_SAMPLE_FLOAT		256	// 32 bit floating-point
#define BASS_SAMPLE_MONO		2	// mono
#define BASS_SAMPLE_LOOP		4	// looped
#define BASS_SAMPLE_3D			8	// 3D functionality
#define BASS_SAMPLE_SOFTWARE	16	// unused
#define BASS_SAMPLE_MUTEMAX		32	// mute at max distance (3D only)
#define BASS_SAMPLE_VAM			64	// unused
#define BASS_SAMPLE_FX			128	// unused
#define BASS_SAMPLE_OVER_VOL	0x10000	// override lowest volume
#define BASS_SAMPLE_OVER_POS	0x20000	// override longest playing
#define BASS_SAMPLE_OVER_DIST	0x30000 // override furthest from listener (3D only)

#define BASS_STREAM_PRESCAN		0x20000 // scan file for accurate seeking and length
#define BASS_STREAM_AUTOFREE	0x40000	// automatically free the stream when it stops/ends
#define BASS_STREAM_RESTRATE	0x80000	// restrict the download rate of internet file stream
#define BASS_STREAM_BLOCK		0x100000 // download internet file stream in small blocks
#define BASS_STREAM_DECODE		0x200000 // don't play the stream, only decode
#define BASS_STREAM_STATUS		0x800000 // give server status info (HTTP/ICY tags) in DOWNLOADPROC

#define BASS_MP3_IGNOREDELAY	0x200 // ignore LAME/Xing/VBRI/iTunes delay & padding info
#define BASS_MP3_SETPOS			BASS_STREAM_PRESCAN

#define BASS_MUSIC_FLOAT		BASS_SAMPLE_FLOAT
#define BASS_MUSIC_MONO			BASS_SAMPLE_MONO
#define BASS_MUSIC_LOOP			BASS_SAMPLE_LOOP
#define BASS_MUSIC_3D			BASS_SAMPLE_3D
#define BASS_MUSIC_FX			BASS_SAMPLE_FX
#define BASS_MUSIC_AUTOFREE		BASS_STREAM_AUTOFREE
#define BASS_MUSIC_DECODE		BASS_STREAM_DECODE
#define BASS_MUSIC_PRESCAN		BASS_STREAM_PRESCAN	// calculate playback length
#define BASS_MUSIC_CALCLEN		BASS_MUSIC_PRESCAN
#define BASS_MUSIC_RAMP			0x200	// normal ramping
#define BASS_MUSIC_RAMPS		0x400	// sensitive ramping
#define BASS_MUSIC_SURROUND		0x800	// surround sound
#define BASS_MUSIC_SURROUND2	0x1000	// surround sound (mode 2)
#define BASS_MUSIC_FT2PAN		0x2000	// apply FastTracker 2 panning to XM files
#define BASS_MUSIC_FT2MOD		0x2000	// play .MOD as FastTracker 2 does
#define BASS_MUSIC_PT1MOD		0x4000	// play .MOD as ProTracker 1 does
#define BASS_MUSIC_NONINTER		0x10000	// non-interpolated sample mixing
#define BASS_MUSIC_SINCINTER	0x800000 // sinc interpolated sample mixing
#define BASS_MUSIC_POSRESET		0x8000	// stop all notes when moving position
#define BASS_MUSIC_POSRESETEX	0x400000 // stop all notes and reset bmp/etc when moving position
#define BASS_MUSIC_STOPBACK		0x80000	// stop the music on a backwards jump effect
#define BASS_MUSIC_NOSAMPLE		0x100000 // don't load the samples

// Speaker assignment flags
#define BASS_SPEAKER_FRONT	0x1000000	// front speakers
#define BASS_SPEAKER_REAR	0x2000000	// rear/side speakers
#define BASS_SPEAKER_CENLFE	0x3000000	// center & LFE speakers (5.1)
#define BASS_SPEAKER_REAR2	0x4000000	// rear center speakers (7.1)
#define BASS_SPEAKER_N(n)	((n)<<24)	// n'th pair of speakers (max 15)
#define BASS_SPEAKER_LEFT	0x10000000	// modifier: left
#define BASS_SPEAKER_RIGHT	0x20000000	// modifier: right
#define BASS_SPEAKER_FRONTLEFT	BASS_SPEAKER_FRONT|BASS_SPEAKER_LEFT
#define BASS_SPEAKER_FRONTRIGHT	BASS_SPEAKER_FRONT|BASS_SPEAKER_RIGHT
#define BASS_SPEAKER_REARLEFT	BASS_SPEAKER_REAR|BASS_SPEAKER_LEFT
#define BASS_SPEAKER_REARRIGHT	BASS_SPEAKER_REAR|BASS_SPEAKER_RIGHT
#define BASS_SPEAKER_CENTER		BASS_SPEAKER_CENLFE|BASS_SPEAKER_LEFT
#define BASS_SPEAKER_LFE		BASS_SPEAKER_CENLFE|BASS_SPEAKER_RIGHT
#define BASS_SPEAKER_REAR2LEFT	BASS_SPEAKER_REAR2|BASS_SPEAKER_LEFT
#define BASS_SPEAKER_REAR2RIGHT	BASS_SPEAKER_REAR2|BASS_SPEAKER_RIGHT

#define BASS_ASYNCFILE			0x40000000	// read file asynchronously
#define BASS_UNICODE			0x80000000	// UTF-16

#define BASS_RECORD_PAUSE		0x8000	// start recording paused
#define BASS_RECORD_ECHOCANCEL	0x2000
#define BASS_RECORD_AGC			0x4000

// BASS_CHANNELINFO types
#define BASS_CTYPE_SAMPLE		1
#define BASS_CTYPE_RECORD		2
#define BASS_CTYPE_STREAM		0x10000
#define BASS_CTYPE_STREAM_VORBIS	0x10002
#define BASS_CTYPE_STREAM_OGG	0x10002
#define BASS_CTYPE_STREAM_MP1	0x10003
#define BASS_CTYPE_STREAM_MP2	0x10004
#define BASS_CTYPE_STREAM_MP3	0x10005
#define BASS_CTYPE_STREAM_AIFF	0x10006
#define BASS_CTYPE_STREAM_CA	0x10007
#define BASS_CTYPE_STREAM_MF	0x10008
#define BASS_CTYPE_STREAM_AM	0x10009
#define BASS_CTYPE_STREAM_SAMPLE	0x1000a
#define BASS_CTYPE_STREAM_DUMMY		0x18000
#define BASS_CTYPE_STREAM_DEVICE	0x18001
#define BASS_CTYPE_STREAM_WAV	0x40000 // WAVE flag (LOWORD=codec)
#define BASS_CTYPE_STREAM_WAV_PCM	0x50001
#define BASS_CTYPE_STREAM_WAV_FLOAT	0x50003
#define BASS_CTYPE_MUSIC_MOD	0x20000
#define BASS_CTYPE_MUSIC_MTM	0x20001
#define BASS_CTYPE_MUSIC_S3M	0x20002
#define BASS_CTYPE_MUSIC_XM		0x20003
#define BASS_CTYPE_MUSIC_IT		0x20004
#define BASS_CTYPE_MUSIC_MO3	0x00100 // MO3 flag

// BASS_ChannelSetSync types
#define BASS_SYNC_POS			0
#define BASS_SYNC_END			2
#define BASS_SYNC_META			4
#define BASS_SYNC_SLIDE			5
#define BASS_SYNC_STALL			6
#define BASS_SYNC_DOWNLOAD		7
#define BASS_SYNC_FREE			8
#define BASS_SYNC_SETPOS		11
#define BASS_SYNC_MUSICPOS		10
#define BASS_SYNC_MUSICINST		1
#define BASS_SYNC_MUSICFX		3
#define BASS_SYNC_OGG_CHANGE	12
#define BASS_SYNC_DEV_FAIL		14
#define BASS_SYNC_DEV_FORMAT	15
#define BASS_SYNC_THREAD		0x20000000	// flag: call sync in other thread
#define BASS_SYNC_MIXTIME		0x40000000	// flag: sync at mixtime, else at playtime
#define BASS_SYNC_ONETIME		0x80000000	// flag: sync only once, else continuously

// BASS_ChannelIsActive return values
#define BASS_ACTIVE_STOPPED			0
#define BASS_ACTIVE_PLAYING			1
#define BASS_ACTIVE_STALLED			2
#define BASS_ACTIVE_PAUSED			3
#define BASS_ACTIVE_PAUSED_DEVICE	4

// Channel attributes
#define BASS_ATTRIB_FREQ			1
#define BASS_ATTRIB_VOL				2
#define BASS_ATTRIB_PAN				3
#define BASS_ATTRIB_EAXMIX			4
#define BASS_ATTRIB_NOBUFFER		5
#define BASS_ATTRIB_VBR				6
#define BASS_ATTRIB_CPU				7
#define BASS_ATTRIB_SRC				8
#define BASS_ATTRIB_NET_RESUME		9
#define BASS_ATTRIB_SCANINFO		10
#define BASS_ATTRIB_NORAMP			11
#define BASS_ATTRIB_BITRATE			12
#define BASS_ATTRIB_BUFFER			13
#define BASS_ATTRIB_GRANULE			14
#define BASS_ATTRIB_USER			15
#define BASS_ATTRIB_TAIL			16
#define BASS_ATTRIB_PUSH_LIMIT		17
#define BASS_ATTRIB_MUSIC_AMPLIFY	0x100
#define BASS_ATTRIB_MUSIC_PANSEP	0x101
#define BASS_ATTRIB_MUSIC_PSCALER	0x102
#define BASS_ATTRIB_MUSIC_BPM		0x103
#define BASS_ATTRIB_MUSIC_SPEED		0x104
#define BASS_ATTRIB_MUSIC_VOL_GLOBAL 0x105
#define BASS_ATTRIB_MUSIC_ACTIVE	0x106
#define BASS_ATTRIB_MUSIC_VOL_CHAN	0x200 // + channel #
#define BASS_ATTRIB_MUSIC_VOL_INST	0x300 // + instrument #

// BASS_ChannelSlideAttribute flags
#define BASS_SLIDE_LOG				0x1000000

// BASS_ChannelGetData flags
#define BASS_DATA_AVAILABLE	0			// query how much data is buffered
#define BASS_DATA_NOREMOVE	0x10000000	// flag: don't remove data from recording buffer
#define BASS_DATA_FIXED		0x20000000	// flag: return 8.24 fixed-point data
#define BASS_DATA_FLOAT		0x40000000	// flag: return floating-point sample data
#define BASS_DATA_FFT256	0x80000000	// 256 sample FFT
#define BASS_DATA_FFT512	0x80000001	// 512 FFT
#define BASS_DATA_FFT1024	0x80000002	// 1024 FFT
#define BASS_DATA_FFT2048	0x80000003	// 2048 FFT
#define BASS_DATA_FFT4096	0x80000004	// 4096 FFT
#define BASS_DATA_FFT8192	0x80000005	// 8192 FFT
#define BASS_DATA_FFT16384	0x80000006	// 16384 FFT
#define BASS_DATA_FFT32768	0x80000007	// 32768 FFT
#define BASS_DATA_FFT_INDIVIDUAL 0x10	// FFT flag: FFT for each channel, else all combined
#define BASS_DATA_FFT_NOWINDOW	0x20	// FFT flag: no Hanning window
#define BASS_DATA_FFT_REMOVEDC	0x40	// FFT flag: pre-remove DC bias
#define BASS_DATA_FFT_COMPLEX	0x80	// FFT flag: return complex data
#define BASS_DATA_FFT_NYQUIST	0x100	// FFT flag: return extra Nyquist value

// BASS_ChannelGetLevelEx flags
#define BASS_LEVEL_MONO		1	// get mono level
#define BASS_LEVEL_STEREO	2	// get stereo level
#define BASS_LEVEL_RMS		4	// get RMS levels
#define BASS_LEVEL_VOLPAN	8	// apply VOL/PAN attributes to the levels
#define BASS_LEVEL_NOREMOVE	16	// don't remove data from recording buffer

// BASS_ChannelGetTags types : what's returned
#define BASS_TAG_ID3		0	// ID3v1 tags : TAG_ID3 structure
#define BASS_TAG_ID3V2		1	// ID3v2 tags : variable length block
#define BASS_TAG_OGG		2	// OGG comments : series of null-terminated UTF-8 strings
#define BASS_TAG_HTTP		3	// HTTP headers : series of null-terminated ASCII strings
#define BASS_TAG_ICY		4	// ICY headers : series of null-terminated ANSI strings
#define BASS_TAG_META		5	// ICY metadata : ANSI string
#define BASS_TAG_APE		6	// APE tags : series of null-terminated UTF-8 strings
#define BASS_TAG_MP4 		7	// MP4/iTunes metadata : series of null-terminated UTF-8 strings
#define BASS_TAG_WMA		8	// WMA tags : series of null-terminated UTF-8 strings
#define BASS_TAG_VENDOR		9	// OGG encoder : UTF-8 string
#define BASS_TAG_LYRICS3	10	// Lyric3v2 tag : ASCII string
#define BASS_TAG_CA_CODEC	11	// CoreAudio codec info : TAG_CA_CODEC structure
#define BASS_TAG_MF			13	// Media Foundation tags : series of null-terminated UTF-8 strings
#define BASS_TAG_WAVEFORMAT	14	// WAVE format : WAVEFORMATEEX structure
#define BASS_TAG_AM_NAME	16	// Android Media codec name : ASCII string
#define BASS_TAG_ID3V2_2	17	// ID3v2 tags (2nd block) : variable length block
#define BASS_TAG_AM_MIME	18	// Android Media MIME type : ASCII string
#define BASS_TAG_LOCATION	19	// redirected URL : ASCII string
#define BASS_TAG_RIFF_INFO	0x100 // RIFF "INFO" tags : series of null-terminated ANSI strings
#define BASS_TAG_RIFF_BEXT	0x101 // RIFF/BWF "bext" tags : TAG_BEXT structure
#define BASS_TAG_RIFF_CART	0x102 // RIFF/BWF "cart" tags : TAG_CART structure
#define BASS_TAG_RIFF_DISP	0x103 // RIFF "DISP" text tag : ANSI string
#define BASS_TAG_RIFF_CUE	0x104 // RIFF "cue " chunk : TAG_CUE structure
#define BASS_TAG_RIFF_SMPL	0x105 // RIFF "smpl" chunk : TAG_SMPL structure
#define BASS_TAG_APE_BINARY	0x1000	// + index #, binary APE tag : TAG_APE_BINARY structure
#define BASS_TAG_MUSIC_NAME		0x10000	// MOD music name : ANSI string
#define BASS_TAG_MUSIC_MESSAGE	0x10001	// MOD message : ANSI string
#define BASS_TAG_MUSIC_ORDERS	0x10002	// MOD order list : BYTE array of pattern numbers
#define BASS_TAG_MUSIC_AUTH		0x10003	// MOD author : UTF-8 string
#define BASS_TAG_MUSIC_INST		0x10100	// + instrument #, MOD instrument name : ANSI string
#define BASS_TAG_MUSIC_CHAN		0x10200	// + channel #, MOD channel name : ANSI string
#define BASS_TAG_MUSIC_SAMPLE	0x10300	// + sample #, MOD sample name : ANSI string

// BASS_ChannelGetLength/GetPosition/SetPosition modes
#define BASS_POS_BYTE			0		// byte position
#define BASS_POS_MUSIC_ORDER	1		// order.row position, MAKELONG(order,row)
#define BASS_POS_OGG			3		// OGG bitstream number
#define BASS_POS_END			0x10	// trimmed end position
#define BASS_POS_LOOP			0x11	// loop start positiom
#define BASS_POS_FLUSH			0x1000000 // flag: flush decoder/FX buffers
#define BASS_POS_RESET			0x2000000 // flag: reset user file buffers
#define BASS_POS_RELATIVE		0x4000000 // flag: seek relative to the current position
#define BASS_POS_INEXACT		0x8000000 // flag: allow seeking to inexact position
#define BASS_POS_DECODE			0x10000000 // flag: get the decoding (not playing) position
#define BASS_POS_DECODETO		0x20000000 // flag: decode to the position instead of seeking
#define BASS_POS_SCAN			0x40000000 // flag: scan to the position

// BASS_ChannelSetDevice/GetDevice option
#define BASS_NODEVICE		0x20000

// BASS_RecordSetInput flags
#define BASS_INPUT_OFF		0x10000
#define BASS_INPUT_ON		0x20000

#define BASS_INPUT_TYPE_MASK		0xff000000
#define BASS_INPUT_TYPE_UNDEF		0x00000000
#define BASS_INPUT_TYPE_DIGITAL		0x01000000
#define BASS_INPUT_TYPE_LINE		0x02000000
#define BASS_INPUT_TYPE_MIC			0x03000000
#define BASS_INPUT_TYPE_SYNTH		0x04000000
#define BASS_INPUT_TYPE_CD			0x05000000
#define BASS_INPUT_TYPE_PHONE		0x06000000
#define BASS_INPUT_TYPE_SPEAKER		0x07000000
#define BASS_INPUT_TYPE_WAVE		0x08000000
#define BASS_INPUT_TYPE_AUX			0x09000000
#define BASS_INPUT_TYPE_ANALOG		0x0a000000

#endif // BASS_INTERFACE_H