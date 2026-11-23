@echo off

if NOT "%Platform%" == "X64" IF NOT "%Platform%" == "x64" (call vcvarsall x64)

cl.exe 86Decode.c /nologo /Zi /FC