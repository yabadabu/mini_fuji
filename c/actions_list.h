#pragma once

#include "properties.h" 

const op_code_t actions_take[] = {
  { OP_DISCOVER_CAMERA   },
  { OP_CONNECT_TO_CAMERA },
  { OC_READ_STORAGE_IDS  },
  { OC_SET_PROP,         PDV_Priority_Mode,   PDV_Priority_Mode_USB },
//  { OC_SET_PROP,         &prop_exposure_time,   PDV_Exposure_Time_5_secs },
  { OC_SET_PROP_ARRAY,   },
  { OC_GET_PROP_ARRAY,   },

  // This is required... even when we don't want autofocus
  { OC_SET_PROP,         PDV_Capture_Control, PDV_Capture_Control_AutoFocus },
  { OC_INITIATE_CAPTURE, },
  { OC_SET_PROP,         PDV_Capture_Control, PDV_Capture_Control_Shoot },
  { OC_INITIATE_CAPTURE, },

  { OC_WAIT_SHOOT_ENDS,  },
  { OC_READ_OBJ_HANDLES  },
  //{ OC_SAVE_IMAGES       },
  { OC_DELETE_IMAGES     },
  { OC_TERMINATE_CAPTURE },
  { OC_SET_PROP,         PDV_Priority_Mode,   PDV_Priority_Mode_Camera },
  { OC_END_OF_PROGRAM    }
};

const op_code_t actions_set_config[] = {
  { OP_DISCOVER_CAMERA   },
  { OP_CONNECT_TO_CAMERA },
  { OC_SET_PROP,         PDV_Priority_Mode,   PDV_Priority_Mode_USB },
  { OC_SET_PROP_ARRAY    },
  { OC_SET_PROP,         PDV_Priority_Mode,   PDV_Priority_Mode_Camera },
  { OC_END_OF_PROGRAM    }
};
