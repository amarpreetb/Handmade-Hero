cl -GR -EHa -Oi -wd4201 -wd4100 -wd4189 -wd4800 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE-WIN32=1 -FC -Zi win32_handmade.cpp user32.lib gdi32.lib winmm.lib

REM 32-bit build
REM cl %CommonCompilerFlags% win32_handmade.cpp /link %CommonCompilerFlags%

REM 64-bit build
cl %CommonCompilerFlags% handmade.cpp -Fmhanmade.map /link /DLL /EXPORT:GameGetSoundSamples /EXPORT:GameUpdateAndRender
cl %CommonCompilerFlags% win32_handmade.cpp -Fmwin32_hanmade.map /link %CommonCompilerFlags%