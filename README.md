### What is that?
It's a port of CLEO4's Audio plugin.

### Is this CLEO4 or CLEO5 version?
It was a CLEO4 version. Starting from v1.3, there is CLEO 5 opcodes. I tried to make it compatible with CLEO4 so you really shouldn`t see any difference or glitches, unless you enable them with the new opcodes!

### How to use opcodes on mobile?!
Compile the script as a PC script or simply add lines below into your SASCM.ini (or VCSCM.ini file):
```
; Since AudioStreams v1.0
0AAC=2,%2d% = load_audio_stream %1d%
0AAD=2,set_audio_stream %1d% state %2d%
0AAE=1,remove_audio_stream %1d%
0AAF=2,%2d% = get_audio_stream_length %1d%
0AB9=2,get_audio_stream %1d% state_to %2d%
0ABB=2,%2d% = audio_stream %1d% volume
0ABC=2,set_audio_stream %1d% volume %2d%
0AC0=2,set_audio_stream %1d% looped %2d%
0AC1=2,%2d% = load_audio_stream_with_3d_support %1d% ; IF and SET
0AC2=4,link_3d_audio_stream %1d% at_coords %2d% %3d% %4d%
0AC3=2,link_3d_audio_stream %1d% to_object %2d%
0AC4=2,link_3d_audio_stream %1d% to_actor %2d%
0AC5=2,link_3d_audio_stream %1d% to_car %2d%

; Since AudioStream v1.3
2500=1,is_audio_stream_playing %1d%
2501=2,%2d% = get_audiostream_duration %1d%
2502=2,get_audio_stream_speed %1d% store_to %2d%
2503=2,set_audio_stream_speed %1d% speed %2d%
2504=3,set_audio_stream_volume_with_transition %1d% volume %2d% time_ms %2d%
2505=3,set_audio_stream_speed_with_transition %1d% speed %2d% time_ms %2d%
2506=2,set_audio_stream_source_size %1d% radius %2d%
2507=2,get_audio_stream_progress %1d% store_to %2d%
2508=2,set_audio_stream_progress %1d% speed %2d%
2509=2,get_audio_stream_type %1d% store_to %2d%
250A=2,set_audio_stream_type %1d% type %2d%

; Since AudioStream v1.4
250B=2,get_audio_stream_progress_seconds %1d% store_to %2d%
250C=2,set_audio_stream_progress_seconds %1d% value %2d%

; Exclusive mobile opcodes (since v1.4)
2540=1,does_game_speed_affect_stream %1d%
2541=2,set_stream %1d% being_affected_by_game_speed %2d%
2542=1,is_audio_stream_3d %1d%
2543=4,get_audio_stream %1d% position %2d% %3d% %4d%
2544=1,is_audio_stream_linked %1d%
2545=1,is_audio_stream_valid %1d%
2546=1,has_audio_stream_doppler_effect %1d%
2547=2,set_stream %1d% doppler_effect %2d%
```