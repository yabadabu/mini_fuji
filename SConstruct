#!/usr/bin/env python
import os
import sys

libname = "fuji"

env = SConscript("godot-cpp/SConstruct")
#env['IPHONEMINVERSION'] = '17.5'
env['IPHONEMINVERSION'] = '18.0'

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["plugins/", "c/"])

fuji_sources = [ f"c/{f}.c" for f in [ "connection", "actions", "discovery", "properties", "blob", "channel", "commands", "dbg" ] ]

all_sources = Glob("plugins/*.cpp") + fuji_sources
ios_sources = Glob("plugins/*.iOS.mm")
apple_sources = Glob("plugins/*.Apple.mm")
windows_sources = Glob("plugins/*.Windows.cpp")
android_sources = Glob("plugins/*.Android.cpp")
platform_sources = apple_sources + windows_sources + ios_sources + android_sources
common_sources = [item for item in all_sources if item not in platform_sources]

if env["platform"] == "macos":

    # Add MacOS frameworks
    frameworks = [ 'AuthenticationServices'
                 , 'StoreKit'
                 ]
    for framework in frameworks:
        env.Append(LINKFLAGS=['-framework', framework])

    library = env.SharedLibrary(
        "game/bin/lib{}.{}.{}.framework/lib{}.{}.{}".format(
            libname, env["platform"], env["target"], libname, env["platform"], env["target"]
        ),
        source=common_sources + apple_sources,
    )

elif env["platform"] == "ios":

    # Add IOS frameworks
    frameworks = [ 'CoreVideo', 'AVFoundation', 'CoreMedia', 'Foundation', 'CoreMotion', 'CoreLocation'
                 , 'AuthenticationServices'
                 , 'StoreKit'
                 , 'UIKit'
                 ]
    for framework in frameworks:
        env.Append(LINKFLAGS=['-framework', framework])

    # The symbol AVCaptureDeviceTypeExternal requires ios 17.0
    env.Append(LINKFLAGS=['-weak_framework', 'AVFoundation'])

    library = env.SharedLibrary(
        "game/bin/lib{}{}{}".format(libname, env["suffix"], env["SHLIBSUFFIX"]),
        source=common_sources + apple_sources + ios_sources,
    )

elif env["platform"] == "android":
    library = env.SharedLibrary(
        "game/bin/lib{}{}{}".format(libname, env["suffix"], env["SHLIBSUFFIX"]),
        source=common_sources + android_sources,
    )

else:
    env.Append(CFLAGS=["/std:c11"])
    library = env.SharedLibrary(
        "game/bin/lib{}{}{}".format(libname, env["suffix"], env["SHLIBSUFFIX"]),
        source=common_sources + windows_sources,
    )

Default(library)
