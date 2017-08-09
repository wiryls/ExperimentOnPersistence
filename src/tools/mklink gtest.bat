@echo off
:begin
set gtestdir=
cls
echo Please input gtest directory.
echo     e.g. D:\git\googletest\googletest
set /p gtestdir=:
if not defined gtestdir   goto begin
set gtestdir=%gtestdir:"=%
if not exist "%gtestdir%" goto begin
echo.

mklink /D "%~dp0"\..\external\include\gtest     "%gtestdir%"\include\gtest
mklink /D "%~dp0"\..\module\gtest\gest          "%gtestdir%"\include\gtest
mklink /D "%~dp0"\..\module\gtest\src           "%gtestdir%"\src
mklink    "%~dp0"\..\module\gtest\gtest_main.cc "%gtestdir%"\src\gtest_main.cc
mklink    "%~dp0"\..\module\gtest\gtest-all.cc  "%gtestdir%"\src\gtest-all.cc

pause