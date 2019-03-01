project "SDL2"
    location "../build/SDL2"
    warnings "Off"
    kind "SharedLib"
    language "C"
    targetsuffix ""
    flags { 
        -- This should speed up initial compile
        "MultiProcessorCompile",
        "NoMinimalRebuild",
    }

    filter "system:windows"
        exceptionhandling "Off"
    filter "system:macosx"
        exceptionhandling "On"
    filter {}

    includedirs {
        "SDL-mirror/include", 
        "../build/assimp/inc"
    }

    files {
        "SDL-mirror/src/*.c",
        "SDL-mirror/src/*.h",

        "SDL-mirror/src/atomic/*.c",

        "SDL-mirror/src/audio/*.c",
        "SDL-mirror/src/audio/*.h",
        
        "SDL-mirror/src/audio/disk/SDL_diskaudio.c", "SDL-mirror/src/audio/disk/SDL_diskaudio.h",
        
        "SDL-mirror/src/audio/dummy/SDL_dummyaudio.c", "SDL-mirror/src/audio/dummy/SDL_dummyaudio.h",
    
        "SDL-mirror/src/cpuinfo/SDL_cpuinfo.c",

        "SDL-mirror/src/dynapi/*.c",
        "SDL-mirror/src/dynapi/*.h",

        "SDL-mirror/src/events/*.c",
        "SDL-mirror/src/events/*.h", 

        "SDL-mirror/src/file/SDL_rwops.c",

        "SDL-mirror/src/haptic/*.c",
        "SDL-mirror/src/haptic/*.h",
        "SDL-mirror/src/haptic/dummy/SDL_syshaptic.c", 

        "SDL-mirror/src/joystick/*.c",
        "SDL-mirror/src/joystick/*.h",
        "SDL-mirror/src/joystick/dummy/SDL_sysjoystick.c",

        "SDL-mirror/src/libm/*.c",
        "SDL-mirror/src/libm/*.h",

        "SDL-mirror/src/power/SDL_power.c", 

        "SDL-mirror/src/render/*.c",
        "SDL-mirror/src/render/*.h",

        "SDL-mirror/src/render/software/*.c",
        "SDL-mirror/src/render/software/*.h",

        "SDL-mirror/src/render/opengl/*.c",
        "SDL-mirror/src/render/opengl/*.h",

        "SDL-mirror/src/stdlib/*.c",

        "SDL-mirror/src/thread/*.c",
        "SDL-mirror/src/thread/*.h",
        
        "SDL-mirror/src/timer/SDL_timer.c", 
        "SDL-mirror/src/timer/SDL_timer_c.h",

        "SDL-mirror/src/video/*.c",
        "SDL-mirror/src/video/*.h",
 
        "SDL-mirror/src/video/dummy/*.c",
        "SDL-mirror/src/video/dummy/*.h",
    }

    filter "system:windows"
        defines { "_WINDOWS" }
        links {"imm32", "oleaut32", "winmm", "version", "kernel32", "user32", "gdi32", "winspool", "comdlg32", "advapi32", "shell32", "ole32", "oleaut32", "uuid", "odbc32", "odbccp32", "OpenGL32", "OpenGL32", "xaudio2"}
        files {
            "SDL-mirror/src/audio/winmm/SDL_winmm.c",
            "SDL-mirror/src/audio/winmm/SDL_winmm.h",
            "SDL-mirror/src/audio/directsound/*.c",
            "SDL-mirror/src/audio/directsound/*.h",
            "SDL-mirror/src/audio/xaudio2/*.c",
            "SDL-mirror/src/audio/xaudio2/*.h",

            "SDL-mirror/src/core/windows/*.c",
            "SDL-mirror/src/core/windows/*.h",

            "SDL-mirror/src/filesystem/windows/*.c",
            "SDL-mirror/src/filesystem/windows/*.h",

            "SDL-mirror/src/haptic/windows/*.c",
            "SDL-mirror/src/haptic/windows/*.h",

            "SDL-mirror/src/joystick/windows/*.c",
            "SDL-mirror/src/joystick/windows/*.h",

            "SDL-mirror/src/loadso/windows/SDL_sysloadso.c", 
            "SDL-mirror/src/power/windows/SDL_syspower.c",

            "SDL-mirror/src/render/direct3d/*.c",
            "SDL-mirror/src/render/direct3d11/*.c",

            "SDL-mirror/src/render/opengles/*.h",
            "SDL-mirror/src/render/opengles/*.c",

            "SDL-mirror/src/render/opengles2/*.h",
            "SDL-mirror/src/render/opengles2/*.c",

            "SDL-mirror/src/thread/generic/SDL_syscond.c",
            "SDL-mirror/src/thread/generic/SDL_sysmutex_c.h",

            "SDL-mirror/src/thread/windows/*.c",
            "SDL-mirror/src/thread/windows/*.h",

            "SDL-mirror/src/timer/windows/SDL_systimer.c",

            "SDL-mirror/src/video/windows/*.c",
            "SDL-mirror/src/video/windows/*.h"
        }

    filter "system:macosx"
        links {"Cocoa.framework", "CoreAudio.framework",  "AudioToolbox.framework", "CoreVideo.framework", "IOKit.framework", "Carbon.framework", "ForceFeedback.framework", "CoreFoundation.framework"}
        includedirs {"/usr/X11R6/include"}
        files {
            "SDL-mirror/src/audio/coreaudio/*.m",
            "SDL-mirror/src/audio/coreaudio/*.h",

            "SDL-mirror/src/core/linux/*.c",
            "SDL-mirror/src/core/linux/*.h",

            "SDL-mirror/src/file/cocoa/*.m",
            "SDL-mirror/src/file/cocoa/*.h",

            "SDL-mirror/src/filesystem/cocoa/*.m",

            "SDL-mirror/src/haptic/darwin/*.h",
            "SDL-mirror/src/haptic/darwin/*.c",

            "SDL-mirror/src/joystick/darwin/*.h",
            "SDL-mirror/src/joystick/darwin/*.c",

            "SDL-mirror/src/loadso/dlopen/*.c",

            "SDL-mirror/src/power/macosx/*.c",

            "SDL-mirror/src/thread/pthread/*.c",
            "SDL-mirror/src/thread/pthread/*.h",

            "SDL-mirror/src/timer/unix/*.c",

            "SDL-mirror/src/video/cocoa/*.h",
            "SDL-mirror/src/video/cocoa/*.m",

            "SDL-mirror/src/render/opengles/*.h",
            "SDL-mirror/src/render/opengles/*.c",

            "SDL-mirror/src/render/opengles2/*.h",
            "SDL-mirror/src/render/opengles2/*.c",

            "SDL-mirror/src/video/x11/*.c",
            "SDL-mirror/src/video/x11/*.h",
        }

    filter {"configurations:debug", "system:windows"}
        links { "ucrtd", "vcruntimed" }
    
    filter {"configurations:release","system:windows"}
        links {  "ucrt",  "vcruntime"}

    filter "configurations:release"
        optimize "Speed"
    
    filter {}

    local ok, err = os.mkdir('./../build/SDL2/inc')
    if (not ok) then
        printf("error %s", err)
    end

    if os.istarget("windows") then
        ok, err = os.copyfile("./SDL-mirror/include/SDL_config_windows.h", './../build/SDL2/inc/SDL_config.h')
        if (not ok) then
            printf("error %s", err)
        end
    elseif os.istarget("macosx") then
        ok, err = os.copyfile("./SDL-mirror/include/SDL_config_macosx.h", './../build/SDL2/inc/SDL_config.h')
        if (not ok) then
            printf("error %s", err)
        end
    end
