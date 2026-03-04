@echo off

set ARGS=-I ..\external -l user32.lib -g -std=c99 -Wno-format-security

IF NOT EXIST build (mkdir build)
pushd build

If NOT EXIST pixel_tracer_temp.dll ("temp" > "pixel_tracer_temp.dll")

:: win32
:: cl ..\src\win32_main.c /Zi /Fd:win32_pixel_tracer /Fe:win32_pixel_tracer /I ..\external /link user32.lib
IF EXIST "win32_pixel_tracer.exe" (
	COPY /B "win32_pixel_tracer.exe"+NUL "win32_pixel_tracer.exe" >NUL || (
		GOTO :SKIP_EXE
	)
)
clang ..\src\win32_build.c ..\src\win32_platform.c -o win32_pixel_tracer.exe %ARGS%
:SKIP_EXE

:: app
:: cl ..\src\main.c /Zi /Fd:pixel_tracer /Fe:pixel_tracer /LD
clang ..\src\build.c ..\src\win32_platform.c -o pixel_tracer.dll -shared %ARGS%

:: cleanup
IF EXIST *.obj (del *.obj)
IF EXIST *.ilk (del *.ilk)
IF EXIST *.exp (del *.exp)
IF EXIST *.lib (del *.lib)

:: shaders
slangc ..\src\triangle.slang -g -target spirv -o ..\data\shaders\triangle.spv
slangc ..\src\main_entry.slang -g -target spirv -o ..\data\shaders\main.spv
slangc ..\src\debug_entry.slang -g -target spirv -o ..\data\shaders\debug.spv

popd
