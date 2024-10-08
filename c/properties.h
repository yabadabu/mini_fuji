#pragma once

#include "connection.h"

// This file is autogenerated

const char* prop_get_value_str( prop_t* p );
prop_t* prop_by_id( uint16_t prop_id );
const prop_t** prop_get_all( );
bool prop_get_nth_value( prop_t* prop, uint32_t idx, void* out_value );

extern prop_t prop_quality;
#define PDV_Quality  0xd018
#define PDV_Quality_Raw                   0x0001
#define PDV_Quality_Fine                  0x0002
#define PDV_Quality_Normal                0x0003
#define PDV_Quality_Fine_Raw              0x0004
#define PDV_Quality_Normal_Raw            0x0005

extern prop_t prop_priority_mode;
#define PDV_Priority_Mode  0xd207
#define PDV_Priority_Mode_Camera          0x0001
#define PDV_Priority_Mode_USB             0x0002

extern prop_t prop_capture_control;
#define PDV_Capture_Control  0xd208
#define PDV_Capture_Control_AutoFocus     0x0200
#define PDV_Capture_Control_Shoot         0x0304

extern prop_t prop_pending_events;

extern prop_t prop_pre_capture_delay;
#define PDV_Pre_Capture_Delay  0x5012
#define PDV_Pre_Capture_Delay_Off         0x0000
#define PDV_Pre_Capture_Delay_2_Secs      0x07d0
#define PDV_Pre_Capture_Delay_10_Secs     0x2710

extern prop_t prop_focus_mode;
#define PDV_Focus_Mode  0x500a
#define PDV_Focus_Mode_Manual             0x0001
#define PDV_Focus_Mode_Single_Auto        0x8001
#define PDV_Focus_Mode_Continuous_Auto    0x8002

extern prop_t prop_batery_info;

extern prop_t prop_batery_info_str;

extern prop_t prop_lens_name_and_serial;

extern prop_t prop_exposure_index;
#define PDV_Exposure_Index  0x500f
#define PDV_Exposure_Index_ISO_100        0x00000064
#define PDV_Exposure_Index_ISO_200        0x000000c8
#define PDV_Exposure_Index_ISO_800        0x00000320
#define PDV_Exposure_Index_ISO_1000       0x000003e8
#define PDV_Exposure_Index_ISO_1250       0x000004e2
#define PDV_Exposure_Index_ISO_1600       0x00000640
#define PDV_Exposure_Index_ISO_2000       0x000007D0
#define PDV_Exposure_Index_ISO_3200       0x00000c80
#define PDV_Exposure_Index_ISO_6400       0x00001900
#define PDV_Exposure_Index_ISO_12800      0x00003200
#define PDV_Exposure_Index_ISO_25600      0x00006400
#define PDV_Exposure_Index_ISO_Auto       0xffffffff

extern prop_t prop_exposure_time;
#define PDV_Exposure_Time  0x500d
#define PDV_Exposure_Time_1_8000_sec      0x0000007a
#define PDV_Exposure_Time_1_6400_sec      0x00000099
#define PDV_Exposure_Time_1_5000_sec      0x000000c1
#define PDV_Exposure_Time_1_4000_sec      0x000000f4
#define PDV_Exposure_Time_1_3200_sec      0x00000133
#define PDV_Exposure_Time_1_2500_sec      0x00000183
#define PDV_Exposure_Time_1_1600_sec      0x00000267
#define PDV_Exposure_Time_1_1250_sec      0x00000307
#define PDV_Exposure_Time_1_1000_sec      0x000003d0
#define PDV_Exposure_Time_1_800_sec       0x000004ce
#define PDV_Exposure_Time_1_640_sec       0x0000060e
#define PDV_Exposure_Time_1_500_sec       0x000007a1
#define PDV_Exposure_Time_1_400_sec       0x0000099c
#define PDV_Exposure_Time_1_320_sec       0x00000c1c
#define PDV_Exposure_Time_1_250_sec       0x00000f42
#define PDV_Exposure_Time_1_200_sec       0x00001339
#define PDV_Exposure_Time_1_160_sec       0x00001838
#define PDV_Exposure_Time_1_125_sec       0x00001e84
#define PDV_Exposure_Time_1_100_sec       0x00002673
#define PDV_Exposure_Time_1_80_sec        0x00003071
#define PDV_Exposure_Time_1_60_sec        0x00003d09
#define PDV_Exposure_Time_1_50_sec        0x00004ce6
#define PDV_Exposure_Time_1_40_sec        0x000060e3
#define PDV_Exposure_Time_1_30_sec        0x00007a12
#define PDV_Exposure_Time_1_25_sec        0x000099cc
#define PDV_Exposure_Time_1_20_sec        0x0000c1c6
#define PDV_Exposure_Time_1_15_sec        0x0000f424
#define PDV_Exposure_Time_1_13_sec        0x00013399
#define PDV_Exposure_Time_1_10_sec        0x0001838c
#define PDV_Exposure_Time_1_8_sec         0x0001e848
#define PDV_Exposure_Time_1_6_sec         0x00026732
#define PDV_Exposure_Time_1_5_sec         0x00030719
#define PDV_Exposure_Time_1_4_sec         0x0003d090
#define PDV_Exposure_Time_1_3_sec         0x0004ce64
#define PDV_Exposure_Time_1_2_sec         0x0007a120
#define PDV_Exposure_Time_1_sec           0x000f4240
#define PDV_Exposure_Time_1_3_secs        0x00133991
#define PDV_Exposure_Time_1_5_secs        0x001836c9
#define PDV_Exposure_Time_2_secs          0x001e8480
#define PDV_Exposure_Time_2_5_secs        0x00267522
#define PDV_Exposure_Time_3_secs          0x00307192
#define PDV_Exposure_Time_4_secs          0x003d0900
#define PDV_Exposure_Time_5_secs          0x004ce644
#define PDV_Exposure_Time_6_5_secs        0x0060e324
#define PDV_Exposure_Time_8_secs          0x007a1200
#define PDV_Exposure_Time_10_secs         0x0099cc88
#define PDV_Exposure_Time_13_secs         0x00c1c648
#define PDV_Exposure_Time_15_secs         0x00f42400
#define PDV_Exposure_Time_20_secs         0x01339910
#define PDV_Exposure_Time_25_secs         0x01838c90
#define PDV_Exposure_Time_30_secs         0x01e84800
#define PDV_Exposure_Time_40_secs         0x02673221
#define PDV_Exposure_Time_50_secs         0x03071921
#define PDV_Exposure_Time_60_secs         0x03d09000
#define PDV_Exposure_Time_2_mins          0x03d0901e
#define PDV_Exposure_Time_4_mins          0x03d0903c
#define PDV_Exposure_Time_8_mins          0x03d0905a
#define PDV_Exposure_Time_15_mins         0x03d09078

