#ifndef _SFX_ASM_H_
#define _SFX_ASM_H_

#define SFX_STEREOMODE 0x0005
#define SFX_PLAYER 0x8400
#define SFX_MODUL 0x87C0
#define SFX_SFX_CODE 0xAA50
#define SFX_TRACKS 0x0004
#define SFX_FEAT_SFX 0x0001
#define SFX_FEAT_GLOBALVOLUMEFADE 0x0001
#define SFX_FEAT_NOSTARTINGSONGLINE 0x0000
#define SFX_FEAT_INSTRSPEED 0x0001
#define SFX_FEAT_CONSTANTSPEED 0x0000
#define SFX_FEAT_COMMAND1 0x0001
#define SFX_FEAT_COMMAND2 0x0001
#define SFX_FEAT_COMMAND3 0x0000
#define SFX_FEAT_COMMAND4 0x0000
#define SFX_FEAT_COMMAND5 0x0000
#define SFX_FEAT_COMMAND6 0x0000
#define SFX_FEAT_COMMAND7SETNOTE 0x0000
#define SFX_FEAT_COMMAND7VOLUMEONLY 0x0000
#define SFX_FEAT_PORTAMENTO 0x0000
#define SFX_FEAT_FILTER 0x0000
#define SFX_FEAT_FILTERG0L 0x0000
#define SFX_FEAT_FILTERG1L 0x0000
#define SFX_FEAT_FILTERG0R 0x0000
#define SFX_FEAT_FILTERG1R 0x0000
#define SFX_FEAT_BASS16 0x0000
#define SFX_FEAT_BASS16G1L 0x0000
#define SFX_FEAT_BASS16G3L 0x0000
#define SFX_FEAT_BASS16G1R 0x0000
#define SFX_FEAT_BASS16G3R 0x0000
#define SFX_FEAT_VOLUMEONLYG0L 0x0000
#define SFX_FEAT_VOLUMEONLYG2L 0x0000
#define SFX_FEAT_VOLUMEONLYG3L 0x0000
#define SFX_FEAT_VOLUMEONLYG0R 0x0000
#define SFX_FEAT_VOLUMEONLYG2R 0x0000
#define SFX_FEAT_VOLUMEONLYG3R 0x0000
#define SFX_FEAT_TABLETYPE 0x0000
#define SFX_FEAT_TABLEMODE 0x0000
#define SFX_FEAT_TABLEGO 0x0001
#define SFX_FEAT_AUDCTLMANUALSET 0x0000
#define SFX_FEAT_VOLUMEMIN 0x0001
#define SFX_FEAT_EFFECTVIBRATO 0x0001
#define SFX_FEAT_EFFECTFSHIFT 0x0001
#define SFX_P_TIS 0x00CB
#define SFX_P_INSTRSTABLE 0x00CB
#define SFX_P_TRACKSLBSTABLE 0x00CD
#define SFX_P_TRACKSHBSTABLE 0x00CF
#define SFX_P_SONG 0x00D1
#define SFX_NS 0x00D3
#define SFX_NR 0x00D5
#define SFX_NT 0x00D7
#define SFX_REG1 0x00D9
#define SFX_REG2 0x00DA
#define SFX_REG3 0x00DB
#define SFX_TMP 0x00DC
#define SFX_FRQADDCMD2 0x00DD
#define SFX_TRACK_VARIABLES 0x80E0
#define SFX_TRACKN_DB 0x80E0
#define SFX_TRACKN_HB 0x80E4
#define SFX_TRACKN_IDX 0x80E8
#define SFX_TRACKN_PAUSE 0x80EC
#define SFX_TRACKN_NOTE 0x80F0
#define SFX_TRACKN_VOLUME 0x80F4
#define SFX_TRACKN_DISTOR 0x80F8
#define SFX_TRACKN_SHIFTFRQ 0x80FC
#define SFX_TRACKN_INSTRX2 0x8100
#define SFX_TRACKN_INSTRDB 0x8104
#define SFX_TRACKN_INSTRHB 0x8108
#define SFX_TRACKN_INSTRIDX 0x810C
#define SFX_TRACKN_INSTRLEN 0x8110
#define SFX_TRACKN_INSTRLOP 0x8114
#define SFX_TRACKN_INSTRREACHEND 0x8118
#define SFX_TRACKN_VOLUMESLIDEDEPTH 0x811C
#define SFX_TRACKN_VOLUMESLIDEVALUE 0x8120
#define SFX_TRACKN_VOLUMEMIN 0x8124
#define SFX_FEAT_EFFECTS 0x0001
#define SFX_TRACKN_EFFDELAY 0x8128
#define SFX_TRACKN_EFFVIBRATOA 0x812C
#define SFX_TRACKN_EFFSHIFT 0x8130
#define SFX_TRACKN_TABLETYPESPEED 0x8134
#define SFX_TRACKN_TABLENOTE 0x8138
#define SFX_TRACKN_TABLEA 0x813C
#define SFX_TRACKN_TABLEEND 0x8140
#define SFX_TRACKN_TABLELOP 0x8144
#define SFX_TRACKN_TABLESPEEDA 0x8148
#define SFX_TRACKN_AUDF 0x814C
#define SFX_TRACKN_AUDC 0x8150
#define SFX_V_ASPEED 0x8154
#define SFX_TRACK_ENDVARIABLES 0x8155
#define SFX_INSTRPAR 0x000C
#define SFX_TABBEGANDDISTOR 0x8182
#define SFX_VIBTABBEG 0x8192
#define SFX_VIB0 0x8196
#define SFX_VIB1 0x8197
#define SFX_VIB2 0x819B
#define SFX_VIB3 0x81A1
#define SFX_VIBTABNEXT 0x81AB
#define SFX_FRQTAB 0x8200
#define SFX_FRQTABBASS1 0x8200
#define SFX_FRQTABBASS2 0x8240
#define SFX_FRQTABPURE 0x8280
#define SFX_VOLUMETAB 0x8300
#define SFX_RASTERMUSICTRACKER 0x8400
#define SFX_RMT_INIT 0x8412
#define SFX_RI0 0x841B
#define SFX_RI1 0x8430
#define SFX_RMT_SILENCE 0x8452
#define SFX_SI1 0x8464
#define SFX_GETSONGLINETRACKLINEINITOFNEWSETINSTRUMENTSONLYRMTP3 0x8470
#define SFX_GETSONGLINE 0x8470
#define SFX_NN0 0x8475
#define SFX_NN1 0x8475
#define SFX_NN1A 0x8485
#define SFX_NN1A2 0x848F
#define SFX_XTRACKS01 0x8498
#define SFX_XTRACKS02 0x849F
#define SFX_NN1B 0x84A7
#define SFX_NN2 0x84AA
#define SFX_NN2A 0x84AC
#define SFX_NN3 0x84B0
#define SFX_GETTRACKLINE 0x84C0
#define SFX_OO0 0x84C0
#define SFX_OO0A 0x84C0
#define SFX_V_SPEED 0x84C1
#define SFX_OO1 0x84C7
#define SFX_OO1B 0x84CD
#define SFX_OO1I 0x84D7
#define SFX_OO1A 0x84F5
#define SFX_RMTGLOBALVOLUMEFADE 0x850C
#define SFX_VOIG 0x8511
#define SFX_OO1X 0x8516
#define SFX_XTRACKS03SUB1 0x8516
#define SFX_V_BSPEED 0x851B
#define SFX_OO2 0x8525
#define SFX_OO62_B 0x8538
#define SFX_OO63 0x8544
#define SFX_OO63_1X 0x8554
#define SFX_OO63_11 0x8561
#define SFX_P2XRMTP3 0x8564
#define SFX_P2X0 0x8567
#define SFX_INITOFNEWSETINSTRUMENTSONLY 0x856A
#define SFX_P2X1 0x856A
#define SFX_RMT_SFX 0x8575
#define SFX_RMTSFXVOLUME 0x8579
#define SFX_SETUPINSTRUMENTY2 0x857D
#define SFX_XATA_RTSHERE 0x85F1
#define SFX_RMT_PLAY 0x85F2
#define SFX_RMT_P0 0x85F2
#define SFX_RMT_P1 0x85F5
#define SFX_RMT_P2 0x85F5
#define SFX_V_ABEAT 0x85FE
#define SFX_V_MAXTRACKLEN 0x8600
#define SFX_P2O3 0x8606
#define SFX_GO_PPNEXT 0x8609
#define SFX_RMT_P3 0x860C
#define SFX_XTRACKS05SUB1 0x8610
#define SFX_PP1 0x8612
#define SFX_PP1B 0x863D
#define SFX_PP2 0x8640
#define SFX_INSTRUMENTSEFFECTS 0x8662
#define SFX_EI1 0x8685
#define SFX_EI2 0x8688
#define SFX_EI2C 0x8694
#define SFX_EI2C2 0x86A2
#define SFX_EI2A 0x86A5
#define SFX_EI2F 0x86BA
#define SFX_EI3 0x86C0
#define SFX_EI4 0x86E4
#define SFX_JMX 0x86F1
#define SFX_CMD1 0x86FE
#define SFX_CMD2 0x8703
#define SFX_CMD3 0x870D
#define SFX_CMD4 0x870D
#define SFX_CMD5 0x870D
#define SFX_CMD6 0x870D
#define SFX_CMD7 0x870D
#define SFX_CMD0 0x870D
#define SFX_CMD0A 0x8713
#define SFX_CMD0A1 0x8722
#define SFX_CMD0C 0x872C
#define SFX_PP9 0x872F
#define SFX_PPNEXT 0x872F
#define SFX_RMT_P4 0x8735
#define SFX_QQ1 0x8737
#define SFX_QQ5 0x873A
#define SFX_RMT_P5 0x873D
#define SFX_SETPOKEY 0x8740
#define SFX_DTRACKN_AUDF0 0x8740
#define SFX_DTRACKN_AUDF1 0x8745
#define SFX_DTRACKN_AUDF2 0x874A
#define SFX_DTRACKN_AUDF3 0x874F
#define SFX_DTRACKN_AUDC0 0x8754
#define SFX_DTRACKN_AUDC1 0x8759
#define SFX_DTRACKN_AUDC2 0x875E
#define SFX_DTRACKN_AUDC3 0x8763
#define SFX_VP1 0x8768
#define SFX_V_AUDCTL 0x876E
#define SFX_RMTPLAYEREND 0x87BE
#define SFX_NEW_INIT 0xAA50
#define SFX_LOOP 0xAA51
#define SFX_FINISH_INIT 0xAA6A
#define SFX_ENABLE_VBI 0xAA76
#define SFX_DISABLE_VBI 0xAA82
#define SFX_NTSC_VBI 0xAA8E
#define SFX_VBI 0xAA9B
#define SFX_SKIP_COUNTER 0xAAA3
#define SFX_SKIP_SILENCE 0xAAB0
#define SFX_MUSIC_PART 0xAAC8
#define SFX_NOT_SETTING_TRACK 0xAADE
#define SFX_END_VBI 0xAAE1
#define SFX_OLD_VBI 0xAAE4
#define SFX_SFX_EFFECT 0xAAE6
#define SFX_TRACK_TO_PLAY 0xAAE7
#define SFX_MUSIC_PLAYS 0xAAE8
#define SFX_SOUND_ACTIVE 0xAAE9
#define SFX_CALL_SILENCE 0xAAEA
#define SFX_NTSC_COUNTER 0xAAEB
#define SFX_VBI_ADDRESS 0xAAEC
#define SFX_VBI_COUNTER 0xAAEE

#endif
