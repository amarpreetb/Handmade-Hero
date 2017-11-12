@echo off

set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4456 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FC -Z7  
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

REM TODO: - can we just build both with one exe?

REM 32-bit build
REM cl %CommonCompilerFlags% ..\code\win32_handmade.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
cl %CommonCompilerFlags% handmade.cpp -Fmwin32_handmade.map  /LD /link /EXPORT:GameGetSoundSamples /EXPORT:gameUpdateAndRender
cl %CommonCompilerFlags% win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags% 
