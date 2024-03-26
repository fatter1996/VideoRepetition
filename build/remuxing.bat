@echo off
ffmpeg.exe  -i %1 %2
move %2 %3
del %1
del %2