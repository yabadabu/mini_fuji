

storage_ids_t storage_ids;
int cmd_get_storage_ids( conn, &storage_ids );




function cmdGetStorageIds( ) {
  return {
    id : '1004',
    name : 'get_storage_ids',

    create( conn ) {
      return Conn.makeMsg( this.id )
    },

    parse( args ) {
      const count = args.readInt32LE( 0 )
      const ids = []
      for( let idx = 0; idx < count; ++idx )
        ids.push( args.readInt32LE( 4 + idx * 4 ))
      return ids;
    },

    test( ) {
      // Full Ans: 18000000 0200 0410 05000000 020000000100001002000010 0c0000000300012005000000
      const ids = this.parse( Buffer.from( '0200000001000000ffffffff', 'hex' ) )
      checkEqual( ids.length, 2, 'cmdGetStorageIds. Invalid length')
      checkEqual( ids[0], 1, 'cmdGetStorageIds. Invalid idx 0')
      checkEqual( ids[1], -1, 'cmdGetStorageIds. Invalid idx 1')
      const ids2 = this.parse( Buffer.from( '02000000'+'01000010'+'02000010', 'hex' ) )
      console.log( ids2 )
    }
  }
}