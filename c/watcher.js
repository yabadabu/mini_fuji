const fs = require('fs');
const os = require('os');
const path = require('path');
const { exec } = require('child_process');
const chokidar = require('chokidar');

const directory_to_watch = '.';
const verbose = process.argv.includes( '-v' );
const running = {};
const is_win32 = (os.platform() == 'win32')

const recompile_cmd = is_win32 ? 
                      (process.argv.includes( 'release' ) ? 'msbuild /property:Configuration=Release' : 'msbuild') :
                      'make'

function compile(eventType, filename, folder, notify) {
  if( running[ folder ] )
    return;
  if( filename.match( /\\\.vs/))
    return;
  console.log(`File:${filename}, event:${eventType} Folder:${folder} Running:${running[folder]}`);
  if( eventType == 'change' || eventType == 'rename' ) {

    // Check if the changed file has a specific extension or meets other criteria
    // This is just a placeholder condition, modify it based on your specific needs
    if (filename.endsWith('.c') || filename.endsWith( '.h')) {
      running[ folder ] = true;
      
      const cmd = is_win32
                ? ( `cd ${folder} && ` + recompile_cmd ) 
                : ( `make --no-print-directory -C ${folder}`)
      console.log( "Compiling...", cmd );
      exec( cmd, (error, stdout, stderr) => {
        running[ folder ] = false;
        if (error) {
          if( is_win32 )
            exec( `powershell -Command "[console]::beep(150,100)"` );
          console.error(`Error executing '${recompile_cmd}': ${error}\n${stdout}`);
          return;
        }
        if( verbose ) {
          console.log(stdout);
        } else {
          if( is_win32 && notify) 
            exec( `powershell -Command "[console]::beep(1500,50)"` );
          console.log( `OK ${folder}`);
        }
        if( stderr )
          console.error(`stderr: ${stderr}`);
      });

    }
  }
}

fs.readdir(directory_to_watch, (err, files) => {
  // const dirs = files.filter( f => fs.lstatSync(f).isDirectory() && f != 'common' )
  //                   .filter( f => (!is_win32) || fs.existsSync( `${f}/${f}.vcxproj` ) )
  const dirs = files.filter( f => fs.lstatSync(f).isDirectory() && f != 'common' && f != 'node_modules')
  dirs.forEach( f => running[f] = false )

  // Watch for changes in the specified directory
  const watcher = fs.watch(directory_to_watch, { recursive: true }, (eventType, filename) => {
    console.log( filename )
    if( !filename )
      return;
    console.log( filename )
    const dir = path.basename(path.dirname(filename))
    if( verbose )
      console.log( filename, "=>", dir, dirs.includes( dir ))
    if( !dirs.includes( dir ))
      return;
    compile( eventType, filename, dir, true );
  });

  watcher.on('error', (error) => {
    console.error(`Watcher encountered an error: ${error}`);
  });

  const watcher2 = chokidar.watch(directory_to_watch, {
      ignored: /(^|[\/\\])\../, // ignore dotfiles
      persistent: true,
      usePolling: true, // Enable polling
      interval: 100 // Polling interval in milliseconds
  });

  watcher2.on('all', (eventType, filename) => {
      if( !filename )
        return;
      const dir = path.basename(path.dirname(filename))
      if( verbose )
        console.log( filename, "=>", dir, dirs.includes( dir ))
      if( !dirs.includes( dir ) && dir != '.' )
        return;
      compile( eventType, filename, dir, true );
  });


  // Trigger fake compilation of all plugins
  dirs.forEach( f => compile( 'change', ".c", f, false ) )



})

