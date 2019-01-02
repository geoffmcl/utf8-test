
# CMakeLists.txt, generated gencmake.pl, on 2015/01/31 13:25:20
cmake_minimum_required( VERSION 2.8.8 )

# CMakeScripts or use the ones that come by default with CMake.
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)

project( utf8-test )

# The version number.
set( utf8-test_MAJOR 2 )
set( utf8-test_MINOR 3 )
set( utf8-test_POINT 4 )


# Allow developer to select is Dynamic or static library built
set( LIB_TYPE STATIC )  # set default static
option( BUILD_SHARED_LIB "Build Shared Library" OFF )
# EXPERIMENTAL - Turn ON to combine library into EXE - above shared library MUST be OFF
option( BUILD_AS_ONE "Build as one. Combine library into EXE" OFF )


if(CMAKE_COMPILER_IS_GNUCXX)
    set( WARNING_FLAGS -Wall )
endif(CMAKE_COMPILER_IS_GNUCXX)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") 
   set( WARNING_FLAGS "-Wall -Wno-overloaded-virtual" )
endif() 

if(WIN32 AND MSVC)
    # turn off various warnings - none needed in this compile
    set(WARNING_FLAGS "${WARNING_FLAGS} /wd4996")
    # foreach(warning 4244 4251 4267 4275 4290 4786 4305)
    #     set(WARNING_FLAGS "${WARNING_FLAGS} /wd${warning}")
    # endforeach(warning)
    set( MSVC_FLAGS "-DNOMINMAX -D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS" )
    # if (${MSVC_VERSION} EQUAL 1600)
    #    set( MSVC_LD_FLAGS "/FORCE:MULTIPLE" )
    # endif (${MSVC_VERSION} EQUAL 1600)
    #set( NOMINMAX 1 )
    # to distinguish between debug and release libs
    set( CMAKE_DEBUG_POSTFIX "d" )
else()
    # items for unix
    set(WARNING_FLAGS "${WARNING_FLAGS} -Wno-write-strings -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unused-function")
endif()

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS} -D_REENTRANT" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS} -D_REENTRANT" )
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MSVC_LD_FLAGS}" )

# configuration file, if needed
# configure_file( ${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h )
# add_definitions( -DHAVE_CONFIG_H )
# include_directories( ${CMAKE_BINARY_DIR} )

if(BUILD_SHARED_LIB)
   set(LIB_TYPE SHARED)
   message(STATUS "*** Building DLL library ${LIB_TYPE}")
else()
   message(STATUS "*** Option BUILD_SHARED_LIB is OFF ${LIB_TYPE}")
endif()

#  DIRENT_FOUND - System has dirent
#  DIRENT_INCLUDE_DIRS - The dirent.h include directories
#  DIRENT_LIBRARIES - The static library needed to use dirent
find_package( Dirent )
if (DIRENT_FOUND)
    message(STATUS "*** Found Dirent inc ${DIRENT_INCLUDE_DIRS}, lib ${DIRENT_LIBRARIES}")
    include_directories( ${DIRENT_INCLUDE_DIRS} )
    list(APPEND add_LIBS ${DIRENT_LIBRARIES} )
    add_definitions( -DHAVE_DIRENT_H=1 )
else ()
    message(STATUS "*** Dirent NOT FOUND!")
endif ()

# for utf8.h header
include_directories( src/source )

#################################################
##### LIBRARIES #####
#################################################
set(name langdict)
set(dir src)
set(${name}_SRCS
    ${dir}/${name}.cxx
    )
set(${name}_HDRS
    ${dir}/${name}.h
    )
add_library( ${name} ${LIB_TYPE} ${${name}_SRCS} ${${name}_HDRS} )
list(APPEND add_LIBS ${name})
# deal with install, if any...
#install( TARGETS ${name} DESTINATION lib )

#################################################
##### EXECUTABLES #####
#################################################

# utf8-test EXECUTABLE from [F:\Projects\utf8\src],
# have 1 C/C++ sources, 0 headers
set(name utf8-test)
set(dir src)
set(${name}_SRCS
    ${dir}/utf8-test.cxx
    )
add_executable( ${name} ${${name}_SRCS} )
if (add_LIBS)
    target_link_libraries( ${name} ${add_LIBS} )
endif ()
if (MSVC)
    set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
endif ()
# deal with install, if any...
install( TARGETS ${name} DESTINATION bin )

set(name chk-utf8)
set(dir src)
set(${name}_SRCS
    ${dir}/${name}.cxx
    )
add_executable( ${name} ${${name}_SRCS} )
if (add_LIBS)
    target_link_libraries( ${name} ${add_LIBS} )
endif ()
if (MSVC)
    set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
endif ()
# deal with install, if any...
install( TARGETS ${name} DESTINATION bin )

set(name uni2utf8)
set(dir src)
set(${name}_SRCS
    ${dir}/${name}.cxx
    )
add_executable( ${name} ${${name}_SRCS} )
if (add_LIBS)
    target_link_libraries( ${name} ${add_LIBS} )
endif ()
if (MSVC)
    set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
endif ()
# deal with install, if any...
install( TARGETS ${name} DESTINATION bin )

set(name chk-BOM)
set(dir src)
set(${name}_SRCS
    ${dir}/${name}.cxx
    )
add_executable( ${name} ${${name}_SRCS} )
if (add_LIBS)
    target_link_libraries( ${name} ${add_LIBS} )
endif ()
if (MSVC)
    set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
endif ()
# deal with install, if any...
install( TARGETS ${name} DESTINATION bin )

if (WIN32)
    set(name chk-con)
    set(dir src)
    set(${name}_SRCS
        ${dir}/${name}.cxx
        )
    add_executable( ${name} WIN32 ${${name}_SRCS} )
    if (add_LIBS)
        target_link_libraries( ${name} ${add_LIBS} )
    endif ()
    if (MSVC)
        set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
    endif ()
    # deal with install, if any...
    install( TARGETS ${name} DESTINATION bin )
endif ()

# eof
