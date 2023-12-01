@echo off
REM This is an example build script. This is not meant to work on every Windows machine

setlocal EnableDelayedExpansion

set BUILD_DIR=build
set OBJ_DIR=%BUILD_DIR%\obj
set EXE_OUTPUT=out.exe
set VSWHERE_EXE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

pushd %~dp0
  goto :arg_%1
  exit /b 0
popd

:arg_
  REM Unless VsDevCmd.bat was already called, initialize a context for cl.exe to run
  if "%VSCMD_VER%"=="" (
    call :initialize_env
  )

  if not exist %BUILD_DIR% (
    mkdir %BUILD_DIR%
  )

  if not exist %OBJ_DIR% (
    mkdir %OBJ_DIR%
  )

  set COMP_FLAGS=/nologo               ^
                 /DWIN32_LEAN_AND_MEAN ^
                 /DNOMINMAX            ^
                 /DENABLE_LOGS         ^
                 /Fo%OBJ_DIR%\         ^
                 /GS-                  ^
                 /W4
  SET LINK_FLAGS=/link                         ^
                 /subsystem:console            ^
                 /nodefaultlib                 ^
                 /entry:mainCRTStartup         ^
                 /out:%BUILD_DIR%\%EXE_OUTPUT%
  set SOURCES=num_to_str.c logs.c example.c
  set LIBRARIES=kernel32.lib
  
  call cl.exe %COMP_FLAGS% %SOURCES% %LINK_FLAGS% %LIBRARIES%
              
  goto :eof

:arg_run
  call %BUILD_DIR%\%EXE_OUTPUT% %*
  goto :eof

:arg_clean
  rmdir /Q /S %BUILD_DIR%
  echo Removed %BUILD_DIR%
  goto :eof

:arg_unknown
  echo Unknown argument %1
  exit /b 1


REM Function
REM ========
:initialize_env
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
    echo Error: could not find Visual Studio installation
    goto :eof
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

  REM If you know the location of VsDevCmd.bat on your machine, you can just delete the code above
  REM in this function, set VSDEVCMD directly with an absolute path, replace "call !VSDEVCMD!" with
  REM "call %VSDEVCMD%", and remove "setlocal EnableDelayedExpansion" at the top of the file.
  
  REM Gain some time by using global variables instead of passing arguments
  set VSCMD_SKIP_SENDTELEMETRY=1
  set __VSCMD_ARG_NO_LOGO=1
  set __VSCMD_ARG_TGT_ARCH=x64
  set __VSCMD_ARG_HOST_ARCH=x64
  echo Calling VsDevCmd.bat...
  call !VSDEVCMD!

  where /Q cl.exe || (
    echo Error: could not find cl.exe
  )
  
  goto :eof
