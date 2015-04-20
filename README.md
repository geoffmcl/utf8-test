# UTF8 TEST Project

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

Have FUN!

Geoff.  
20150420

; eof
