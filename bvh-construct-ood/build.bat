@echo off

cl -nologo -EHsc -Zi -std:c++17 main.cpp -link -out:bvh-construct.exe
