@echo off

setlocal EnableDelayedExpansion

REM Files & directories
set BUILD_DIR=build
set OBJS_DIR=%BUILD_DIR%\obj
set EXE_OUTPUT=logs.exe
set VSWHERE_EXE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

REM If known, the path to VsDevCmd can be directly set to skip calling vswhere
set VSDEVCMD=""

REM Compilation options
set COMP_ARCH=x64
set COMP_FLAGS=/nologo               ^
               /DWIN32_LEAN_AND_MEAN ^
               /DNOMINMAX            ^
               /DLOGS_ENABLED        ^
               /Fo%OBJS_DIR%\        ^
               /GS-                  ^
               /Gs2147483647         ^
               /W4                   ^
               /O2                   ^
               /std:c11              ^
               /utf-8
set LINK_FLAGS=/link                 ^
               /subsystem:console    ^
               /entry:mainCRTStartup ^
               /nodefaultlib         ^
               /opt:icf              ^
               /opt:ref              ^
               /incremental:no       ^
               /fixed                ^
               /out:%BUILD_DIR%\%EXE_OUTPUT%
set SOURCES=to_str_utilities.c ^
            logs.c             ^
            example.c
set LIBRARIES=kernel32.lib

pushd %~dp0
  goto :arg_%1
  exit /b 0
popd

:arg_
:arg_x64
  REM Unless VsDevCmd.bat was already called, initialize a context for cl.exe to run in
  if "%VSCMD_VER%"=="" (
    call :initialize_env
  )

  if not exist %BUILD_DIR% (
    mkdir %BUILD_DIR%
  )

  if not exist %OBJS_DIR% (
    mkdir %OBJS_DIR%
  )

  call cl.exe %COMP_FLAGS% %SOURCES% %LINK_FLAGS% %LIBRARIES%
  echo Done.
  goto :eof

:arg_run
  call %BUILD_DIR%\%EXE_OUTPUT% %2 %3 %4 %5 %6 %7 %8 %9
  goto :eof

:arg_clean
  if exist %BUILD_DIR% (
    rmdir /Q /S %BUILD_DIR%
    echo Removed %BUILD_DIR%
  )
  goto :eof

:arg_unknown
  echo Unknown argument %1
  exit /b 1


REM Function
REM ========
:initialize_env
  if not exist %VSDEVCMD% (
    echo Looking for VsDevCmd.bat through vswhere.exe
    
    if not exist %VSWHERE_EXE% (
      echo Error: could not find vswhere.exe
      goto :eof
    )

    REM Find the latest version of Visual Studio Build Tools
    set FIND_VS_INSTALL_DIR_CMD=%VSWHERE_EXE% -latest                                             ^
                                              -products Microsoft.VisualStudio.Product.BuildTools ^
                                              -property installationPath

    for /f "tokens=*" %%i in ('!FIND_VS_INSTALL_DIR_CMD!') do set VS_INSTALL_DIR=%%i

    if not exist !VS_INSTALL_DIR! (
      set FIND_VS_INSTALL_DIR_CMD=%VSWHERE_EXE% -latest                                                 ^
                                                -requires Microsoft.VisualStudio.Workload.NativeDesktop ^
                                                -property installationPath

      for /f "tokens=*" %%i in ('!FIND_VS_INSTALL_DIR_CMD!') do set VS_INSTALL_DIR=%%i

      if not exist !VS_INSTALL_DIR! (
        echo Error: could not find Visual Studio Build Tools nor the Visual Studio Native Desktop workload
        goto :eof
      )
    )

    set VSDEVCMD="!VS_INSTALL_DIR!\Common7\Tools\VsDevCmd.bat"
    if not exist !VSDEVCMD! (
      REM Search around VS_INSTALL_DIR if the path to VsDevCmd.bat doesn't exist
      pushd !VS_INSTALL_DIR!
        for /f "tokens=*" %%i in ('dir /s /b /a-d "VsDevCmd.bat"') do set VSDEVCMD=%%i
      popd

      if not exist !VSDEVCMD! (
        echo Error: could not find VsDevCmd.bat
        goto :eof
      )
    )

    echo Found VsDevCmd.bat at !VSDEVCMD!
  )

  REM Gain some time by using environment variables instead of passing arguments
  set VSCMD_SKIP_SENDTELEMETRY=1
  set __VSCMD_ARG_NO_LOGO=1
  set __VSCMD_ARG_TGT_ARCH=%COMP_ARCH%
  set __VSCMD_ARG_HOST_ARCH=%COMP_ARCH%
  echo Calling VsDevCmd.bat
  call !VSDEVCMD!

  where /Q cl.exe || (
    echo Error: could not find cl.exe
  )
  
  goto :eof
