const cfg = { 

  cmds : {
    '9801' : 'Unsupported Get object properties supported',
    '9802' : 'Unsupported Get object property description',
    '9803' : 'Unsupported Get object property value',
    '9805' : 'Unsupported Get object property list',
    '100a' : 'Unsupported Get thumbnail',
    '100b' : 'Unsupported Delete object',
    '100f' : 'Unsupported Format storage',

  },

  return_codes : {
    '2000' : 'Undefined',
    '2001' : 'OK',
    '2002' : 'GeneralError',
    '2003' : 'SessionNotOpen',
    '2004' : 'InvalidTransactionID',
    '2005' : 'OperationNotSupported',
    '2006' : 'ParameterNotSupported',
    '2007' : 'IncompleteTransfer',
    '2008' : 'InvalidStorageId',
    '2009' : 'InvalidObjectHandle',
    '200a' : 'DevicePropNotSupported',
    '200b' : 'InvalidObjectFormatCode',
    '2010' : 'NoThumbnailPresent',
    '2015' : 'NoValidObjectInfo',
    '2018' : 'CaptureAlreadyTerminated',
    '2019' : 'DeviceBusy',
    '201b' : 'InvalidDevicePropFormat',
    '201c' : 'InvalidDevicePropValue',
    '201d' : 'InvalidParameter',
    '201e' : 'SessionAlreadyOpened',
  },

  // Props
  props : { },
  DPC_ImageSize         : '5003',
  DPC_WhiteBalance      : '5005',
  DPC_FNumber           : '5007',
  DPC_FocusMode         : '500a',   // Enumeration [1,32769,32770] 
  DPC_ExposureTime      : '500d',
  DPC_ExposureProgramMode : '500e',  // '0100', '0300', '0400', '0600'
  DPC_PreCaptureDelay   : '5012',   // Off/2s/10s
  DPC_Quality           : 'd018',   // DPV_Quality
  DPC_ShotCount         : 'd154',   // Returns an int
  DPC_CommandDialMode   : 'd028',   // Not supported
  DPC_PriorityMode      : 'd207',   // Usb vs Wifi
  DPC_CaptureControl    : 'd208',   // 
  DPC_AutoFocusStatus   : 'd209',
  DPC_CurrentState      : 'd212',   // Num Events/Changes???
  DPC_LensZoomPosCaps   : 'd38c',   // Ranged
  DPC_FreeSDRAMImages   : 'd20e',   // Reduced with each take

  //DPC_BatteryLevel      : '5001',  // Not supported

  DPC_MediaStatus       : 'd211',   // Works? Num frames than can be captured?
  DPC_DeviceError       : 'd21b',   // Works?
  DPC_AppVersion        : 'df24',   // Works?

  //DPC_RemainingCaptures : 'd229',   // Not supported in XT-2

  DPV_Quality : {
    '0100' : 'Raw',
    '0200' : 'Fine',
    '0300' : 'Normal',
    '0400' : 'Fine+Raw',
    '0500' : 'Normal+Raw',
  },
  DPV_PreCaptureDelay : {
    'd007' : '10 Secs',
    '1027' : '2 Secs',
    '0000' : 'Off',
  },
  DPV_FocusMode : {
    '0100' : 'Manual',
    '0180' : 'Single Auto',
    '0280' : 'Continuous Auto',
  },
  DPV_CaptureControl : {
    '0401' : 'Unknown',
    '0002' : 'AutoFocus',
    '0400' : 'Cancel Autofocus',
    '0403' : 'Shoot',
    '0005' : 'Bulb On',
    '0c00' : 'Bulb Off',
    '00a0' : 'Unknown',
    '0600' : 'Unknown',
    '0090' : 'Unknown',
    '0200' : 'Unknown',
    '0091' : 'Unknown',
    '0100' : 'Unknown',
    '0093' : 'Unknown',
    '0500' : 'Unknown',
  },
  DPV_PriorityModel : {
    '0100' : 'Camera',
    '0200' : 'USB',
  },
  DPV_CommandDialMode : {
    '0000' : "Both",
    '0100' : "Aperture",
    '0200' : "ShutterSpeed",
    '0300' : "None",
  },

  // -----------------------------------------
  data_types : {
    1 : "INT8",
    2 : "UINT8",
    3 : "INT16",
    4 : "UINT16",
    5 : "INT32",
    6 : "UINT32",
    7 : "INT64",
    8 : "UINT64",
    "65535" : "STR",
  },

  formats : {
    '3800' : 'UNKNOWN',
    '3801' : 'JPEG',
    '3802' : 'TIFF',
    '3804' : 'PNG',
    'b103' : 'RAF',
  },

  events : {
    '4002' : "ObjectAdded",
    '4003' : "ObjectRemoved",
    '4004' : "StoreAdded",
    '4005' : "StoreRemoved",
    '400d' : "CaptureComplete",
    'c006' : "Unknown Event",
  },

  storage : {

    types : {
      1 : 'Fixed ROM',
      2 : 'Removable ROM',
      3 : 'Fixed RAM',
      4 : 'Removable RAM',
    },

    file_systems : {
      1 : 'Generic Flat',
      2 : 'Hierarchical',
      3 : 'DOS FAT',
      4 : 'NTFS',
      5 : 'UDF',
    },

    access : {
      0 : 'Read/Write',
      1 : 'Read-Only Perm',
      2 : 'Read-Only Protected',
    },

  }

}

if( !cfg.props.length ) {
  Object.keys( cfg )
    .filter( s => s.startsWith( 'DPC_' ))
    .forEach( code => {
      const val = cfg[ code ]
      const name = code.substr( 4 )
      const human_label = name.replace( /([A-Z])/g, " $1")
                              .trim()
                              .replace( 'S D R A M', 'SDRAM' )
                              .replace( 'A E Lock', 'AELock' )
      cfg.props[ val ] = human_label
    })
    //console.log( cfg.props )
}

module.exports = cfg