@echo off

cd build

:: win32
:: cl ..\src\win32_main.c /Zi /Fd:win32_pixel_tracer /Fe:win32_pixel_tracer /I ..\external /link user32.lib
IF EXIST "win32_pixel_tracer.exe" (
	COPY /B "win32_pixel_tracer.exe"+NUL "win32_pixel_tracer.exe" >NUL || (
		ECHO Skipping EXE
		GOTO :SKIP_EXE
	)
)
clang -std=c99 ..\src\win32_main.c -o win32_pixel_tracer.exe -I ..\external -l user32.lib -g -MJ win32_compile_commands.json
:SKIP_EXE

:: app
:: cl ..\src\main.c /Zi /Fd:pixel_tracer /Fe:pixel_tracer /LD
clang -std=c99 ..\src\main.c -o pixel_tracer.dll -shared -I ..\external -g -MJ app_compile_commands.json
:: cleanup
:: del *.obj
:: del *.ilk
del *.exp
del *.lib

:: merge compile commands
echo [> ..\compile_commands.json
type win32_compile_commands.json >> ..\compile_commands.json
type app_compile_commands.json >> ..\compile_commands.json
echo { "directory": "c:\\pixel-tracer\\build", "file": "..\\src\\win32_vulkan.c", "arguments": ["C:\\Program Files\\LLVM\\bin\\clang.exe", "-I", "..\\external"] },>>..\compile_commands.json
echo ]>> ..\compile_commands.json

del win32_compile_commands.json
del app_compile_commands.json

cd ..
