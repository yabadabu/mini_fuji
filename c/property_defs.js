const props = {
  Quality : {
    id : 'd018',
    dt : 'U16',
    values : {
      '0001' : 'Raw',
      '0002' : 'Fine',
      '0003' : 'Normal',
      '0004' : 'Fine Raw',
      '0005' : 'Normal Raw',
    }
  },
  
  "Priority Mode" : {
    id : 'd207',
    dt : 'U16',
    values : {
      '0001' : 'Camera',
      '0002' : 'USB',
    }
  },

  "Capture Control" : {
    id : 'd208',
    dt : 'U16',
    values : {
      '0200' : 'AutoFocus',
      '0304' : 'Shoot',
      //'0500' : 'Bulb On',
      //'000c' : 'Bulb Off',
    }
  },

  "Pending Events" : {      // CurrentState in other sources
    id : 'd212',
    dt : 'ARRAY_PROP_VALUES',
    read_only : true,
  },

  "Pre Capture Delay" : {      // CurrentState in other sources
    id : '5012',
    dt : 'U16',
    values : {
      '07d0' : '10 Secs',
      '2710' : '2 Secs',
      '0000' : 'Off',
    }
  },

  "Focus Mode" : {      // CurrentState in other sources
    id : '500a',
    dt : 'U16',
    values : {
      '0001' : 'Manual',
      '8001' : 'Single Auto',
      '8002' : 'Continuous Auto',
    }
  },

  "Exposure Time" : {
    id : '500d',
    dt : 'U32',
    //factory_val: 'f4000000',
    values : {
      /*
      '7a000000', '99000000', 'c1000000', 'f4000000',
      '33010000', '83010000', 'e8010000', '67020000',
      '07030000', 'd0030000', 'ce040000', '0e060000',
      'a1070000', '9c090000', '1c0c0000', '420f0000',
      '39130000', '38180000', '841e0000', 
      '00002673' : "1/100s", /// OK
      '71300000', '093d0000', 'e64c0000', 'e3600000',
      '127a0000', 'cc990000', 'c6c10000', '24f40000',
      '99330100', '8c830100', '48e80100', '32670200',
      '19070300', '90d00300', '64ce0400', '320e0600',
      //2.5, 3.4,5,6,8,,10,13, 
      '20a10700' : "1/2s", 
      'c89c0900', 
      '641c0c00', 
      '40420f00' : "1 Sec",
      '91391300' : "1.3 Secs"
      'c9381800' : "1.5 Secs"
      '80841e00' : "2 Secs"
      '22732600' : "2.5 Secs"
      '92713000' : "3 Secs"
      '00093d00' : "4 Secs"
      */
      '004ce644' : "5 secs",    // <<< This is host .   network is 44 e6 4c 00
      '0060e324' : "6.5 secs",
      '007a1200' : "8 secs",
      '0099cc88' : "10 secs",
      '00c1c648' : "13 secs",
      '00f42400' : "15 secs",
      '01339910' : "20 secs",
      '01838c90' : "25 secs",
      '01e84800' : "30 secs",
      '02673221' : "40 secs",
      '03071921' : "50 secs",
      '03d09000' : "60 secs",
      '03d0901e' : "2 mins",
      '03d0903c' : "4 mins",
      '03d0905a' : "8 mins",
      '03d09078' : "15 mins",
      //'ffffffff'
      /*
      enums: [
    '7a000000', '99000000', 'c1000000', 'f4000000',
    '33010000', '83010000', 'e8010000', '67020000',
    '07030000', 'd0030000', 'ce040000', '0e060000',
    'a1070000', '9c090000', '1c0c0000', '420f0000',
    '39130000', '38180000', '841e0000', '73260000',
    '71300000', '093d0000', 'e64c0000', 'e3600000',
    '127a0000', 'cc990000', 'c6c10000', '24f40000',
    '99330100', '8c830100', '48e80100', '32670200',
    '19070300', '90d00300', '64ce0400', '320e0600',
    '20a10700', 'c89c0900', '641c0c00', '40420f00',
    '91391300', 'c9381800', '80841e00', '22732600',
    '92713000', '00093d00', '44e64c00', '24e36000',
    '00127a00', '88cc9900', '48c6c100', '0024f400',
    '10993301', '908c8301', '0048e801', '21326702',
    '21190703', '0090d003', '1e90d003', '3c90d003',
    '5a90d003', '7890d003', 'ffffffff'
*/
    }
  },

}

module.exports = props