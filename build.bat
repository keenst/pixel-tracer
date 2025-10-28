@echo off

cd build
cl ..\src\main.c /Zi /Fd:pixel_tracer /Fe:pixel_tracer
del main.obj
del pixel_tracer.ilk
cd ..
