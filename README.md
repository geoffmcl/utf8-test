# UTF8 TEST Project 20160127

Found a header only utf8 cpp source at https://sourceforge.net/projects/utfcpp/, and this small project generates a utf8-test app using that header source.

### Prerequisites:

 1. git - http://git-scm.com/downloads
 2. cmake - http://www.cmake.org/download/
 3. Native build tools to suit generator used.

### Building:

This project uses the cmake build file generator.

#### In Unix/Linux

 1. cd build
 2. cmake ..
 3. make
 
#### In Windows

 1. cd build
 2. cmake ..
 3. cmake --build . --config Release
 
The 'build' directory contains convenient build scripts - build-me.bat and build-me.sh - It should be relatively easy to modify these to suit your particular environment.
 
Of course the cmake GUI can also be used, setting the source directory, and the binary directory to the 'build' folder. And in Windows, the MSVC IDE can be used if this is the chosen generator.

### Executables

All binaries are experimental. They started as an exercise to understand utf-8 character sequencing. All are WIP!

#### utf8-test.exe

Given an input file, check if there are any invalid utf-8 sequences in the file.

#### uni2utf8.exe

Given an a unicode input file, write out a utf-8.

#### chk-utf8.exe

More tests on being able to recognise, and correctly step over utf-8 character sequences.

#### chk-con.exe

For WIN32 only - Is a WinMain app, but opens a console, and re-sets stdout and stdin so printf output has somewhere to go.

#### chk-BOM.exe

Read the first up to 8 bytes from the input files, and attempt to identify if it starts with a known BOM.

#### langdict.lib

Some experiments including and compiling and outputting UTF-8 characters. Used by chk-con only.

Have FUN!

Geoff.  
20160127 - 20151209 - 20150420

; eof
