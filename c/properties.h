#pragma once

#include "connection.h"

// This file is autogenerated

const char* prop_get_value_str( prop_t* p );
const prop_t* prop_by_id( uint16_t prop_id );

extern const prop_t prop_quality;
#define PDV_Quality_Raw                   0x0001
#define PDV_Quality_Fine                  0x0002
#define PDV_Quality_Normal                0x0003
#define PDV_Quality_FineRaw               0x0004
#define PDV_Quality_NormalRaw             0x0005

extern const prop_t prop_priority_mode;
#define PDV_Priority_Mode_Camera          0x0001
#define PDV_Priority_Mode_USB             0x0002

extern const prop_t prop_capture_control;
#define PDV_Capture_Control_AutoFocus     0x0200
#define PDV_Capture_Control_Shoot         0x0304

extern const prop_t prop_pending_events;

extern const prop_t prop_pre_capture_delay;
#define PDV_Pre_Capture_Delay_2Secs       0x2710
#define PDV_Pre_Capture_Delay_10Secs      0x07d0
#define PDV_Pre_Capture_Delay_Off         0x0000

extern const prop_t prop_focus_mode;
#define PDV_Focus_Mode_SingleAuto         0x8001
#define PDV_Focus_Mode_ContinuousAuto     0x8002
#define PDV_Focus_Mode_Manual             0x0001

extern const prop_t prop_exposure_time;
#define PDV_Exposure_Time_5secs           0x004ce644
#define PDV_Exposure_Time_6_5secs         0x0060e324
#define PDV_Exposure_Time_8secs           0x007a1200
#define PDV_Exposure_Time_10secs          0x0099cc88
#define PDV_Exposure_Time_13secs          0x00c1c648
#define PDV_Exposure_Time_15secs          0x00f42400
#define PDV_Exposure_Time_20secs          0x01339910
#define PDV_Exposure_Time_25secs          0x01838c90
#define PDV_Exposure_Time_30secs          0x01e84800
#define PDV_Exposure_Time_40secs          0x02673221
#define PDV_Exposure_Time_50secs          0x03071921
#define PDV_Exposure_Time_60secs          0x03d09000
#define PDV_Exposure_Time_2mins           0x03d0901e
#define PDV_Exposure_Time_4mins           0x03d0903c
#define PDV_Exposure_Time_8mins           0x03d0905a
#define PDV_Exposure_Time_15mins          0x03d09078

