const props = {
  Quality : {
    id : 'd018',
    dt : 'U16',
    values : [
      '0001' , 'Raw',
      '0002' , 'Fine',
      '0003' , 'Normal',
      '0004' , 'Fine Raw',
      '0005' , 'Normal Raw',
    ]
  },
  
  "Priority Mode" : {
    id : 'd207',
    dt : 'U16',
    values : [
      '0001' , 'Camera',
      '0002' , 'USB',
    ]
  },

  "Capture Control" : {
    id : 'd208',
    dt : 'U16',
    values : [
      '0200' , 'AutoFocus',
      '0304' , 'Shoot',
      //'0500' : 'Bulb On',
      //'000c' : 'Bulb Off',
    ]
  },

  "Pending Events" : {      // CurrentState in other sources
    id : 'd212',
    dt : 'ARRAY_PROP_VALUES',
    read_only : true,
  },

  "Pre Capture Delay" : {      // CurrentState in other sources
    id : '5012',
    dt : 'U16',
    values : [
      '0000' , 'Off',
      '07d0' , '2 Secs',     // 0x07d0 ->  2000 in decimal
      '2710' , '10 Secs',    // 0x2710 -> 10000 in decimal
    ]
  },

  "Focus Mode" : {      // CurrentState in other sources
    id : '500a',
    dt : 'U16',
    values : [
      '0001' , 'Manual',
      '8001' , 'Single Auto',
      '8002' , 'Continuous Auto',
    ]
  },

  "Batery Info" : {      // CurrentState in other sources
    id : 'D36A',
    dt : 'U32',
    ranged : true,   // 24 bits
    read_only : true,
  },

  "Batery Info Str" : {   
    id : 'D36B',
    dt : 'STRING',
    read_only : true,
  },

  "Lens Name And Serial" : {   
    id : 'D36D',
    dt : 'STRING',
    read_only : true,
  },

  "Exposure Index" : {
    id : '500f',
    dt : 'U32',
    values : [
      '00000064' , "ISO 100",
      '000000c8' , "ISO 200",
      '00000320' , "ISO 800",
      '000003e8' , "ISO 1000",
      '000004e2' , "ISO 1250",
      '00000640' , "ISO 1600",
      '000007D0' , "ISO 2000",
      '00000c80' , "ISO 3200",
      '00001900' , "ISO 6400",
      '00003200' , "ISO 12800",
      '00006400' , "ISO 25600",
      'ffffffff', 'ISO Auto',
//      250,320,400,500,640,2500,,4000,5000,,8000,10000,,,51200,4294967295,4294967294,4294967293
    ]
  },

  // Requires the camera to be in Time -mode
  "Exposure Time" : {
    id : '500d',
    dt : 'U32',
    values : [
      '0000007a' , "1/8000 sec",
      '00000099' , "1/6400 sec",
      '000000c1' , "1/5000 sec",
      '000000f4' , "1/4000 sec",
      '00000133' , "1/3200 sec",
      "00000183" , "1/2500 sec",
      //  1/2000 83010000', 'e8010000'
      '00000267' , "1/1600 sec" , 
      '00000307' , "1/1250 sec" ,
      '000003d0' , "1/1000 sec" ,
      '000004ce' , "1/800 sec" ,
      '0000060e' , "1/640 sec" ,
      '000007a1' , "1/500 sec" ,
      '0000099c' , "1/400 sec" ,
      '00000c1c' , "1/320 sec" ,
      '00000f42' , "1/250 sec" ,
      '00001339' , "1/200 sec" ,
      '00001838' , "1/160 sec" ,
      '00001e84' , "1/125 sec" ,
      '00002673' , "1/100 sec", /// OK... Net 73260000
      '00003071' , "1/80 sec",
      '00003d09' , "1/60 sec",
      '00004ce6' , "1/50 sec",
      '000060e3' , "1/40 sec",
      '00007a12' , "1/30 sec",
      '000099cc' , "1/25 sec",
      '0000c1c6' , "1/20 sec",
      '0000f424' , "1/15 sec",
      '00013399' , "1/13 sec",
      '0001838c' , "1/10 sec",
      '0001e848' , "1/8 sec",
      '00026732' , "1/6 sec",
      '00030719' , "1/5 sec",
      '0003d090' , "1/4 sec",
      '0004ce64' , "1/3 sec",
      '0007a120' , "1/2 sec",
      '000f4240' , "1 sec",
      '00133991' , "1.3 secs",
      '001836c9' , "1.5 secs",
      '001e8480' , "2 secs",
      '00267522' , "2.5 secs",
      '00307192' , "3 secs",
      '003d0900' , "4 secs",
      '004ce644' , "5 secs",    // <<< This is host .   network is 44 e6 4c 00
      '0060e324' , "6.5 secs",
      '007a1200' , "8 secs",
      '0099cc88' , "10 secs",
      '00c1c648' , "13 secs",
      '00f42400' , "15 secs",
      '01339910' , "20 secs",
      '01838c90' , "25 secs",
      '01e84800' , "30 secs",
      '02673221' , "40 secs",
      '03071921' , "50 secs",
      '03d09000' , "60 secs",
      '03d0901e' , "2 mins",
      '03d0903c' , "4 mins",
      '03d0905a' , "8 mins",
      '03d09078' , "15 mins",
      //'ffffffff'
    ]
  },

/*
Label: Focus Metering Mode
Type: RADIO
Current: Single-area AF
Choice: 0 Single-area AF
Choice: 1 Dynamic-area AF
Choice: 2 Group-dynamic AF


Label: Exposure Program
Type: RADIO
Current: A
Choice: 0 M
Choice: 1 A
Choice: 2 S
Choice: 3 Action



Label: WhiteBalance
Type: RADIO
Current: Automatic
Choice: 0 Automatic
Choice: 1 Daylight
Choice: 2 Tungsten
Choice: 3 Unknown value 0008
Choice: 4 Fluorescent Lamp 1
Choice: 5 Fluorescent Lamp 2
Choice: 6 Fluorescent Lamp 3
Choice: 7 Shade
Choice: 8 Choose Color Temperature
Choice: 9 Preset Custom 1
Choice: 10 Preset Custom 2
Choice: 11 Preset Custom 3


Image Size(0x5003):(readwrite) (type=0xffff) Enumeration [
  '3008x2000',
  '3008x1688',
  '2000x2000',
  '4240x2832',
  '4240x2384',
  '2832x2832',
  '6000x4000',
  '6000x3376',
  '4000x4000'
  ] value: '6000x4000'  -> 0a3600300030003000780034003000300030000000
  
#define PTP_DPC_FUJI_LensNameAndSerial      0xD36D  -> 032c002c000000


*/

}

module.exports = props