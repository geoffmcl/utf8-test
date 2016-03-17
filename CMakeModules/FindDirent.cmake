# Try to find Dirent header and library
#
# Once done this will define
#  DIRENT_FOUND - System has dirent
#  DIRENT_INCLUDE_DIRS - The dirent.h include directories
#  DIRENT_LIBRARIES - The static library needed to use dirent
# 

message(STATUS "*** Looking for dirent...")
find_path(DIRENT_INCLUDE_DIR dirent.h
          HINTS ENV DIRENT_ROOT_DIR
          PATH_SUFFIXES include 
          )

if (MSVC)
########################################################################
find_library(DIRENT_LIBRARY_DEBUG NAMES direntd
             HINTS ENV DIRENT_ROOT_DIR
             PATH_SUFFIXES lib 
             )
find_library(DIRENT_LIBRARY_RELEASE NAMES dirent
             HINTS ENV DIRENT_ROOT_DIR
             PATH_SUFFIXES lib 
             )
if (DIRENT_LIBRARY_DEBUG AND DIRENT_LIBRARY_RELEASE)
    set(DIRENT_LIBRARIES 
        debug ${DIRENT_LIBRARY_DEBUG}
        optimized ${DIRENT_LIBRARY_RELEASE} )
elseif (DIRENT_LIBRARY_RELEASE)
    set(DIRENT_LIBRARIES ${DIRENT_LIBRARY_RELEASE} )
endif ()
########################################################################
else ()
########################################################################
find_library(DIRENT_LIBRARY NAMES dirent libdirent
             HINTS ENV DIRENT_ROOT_DIR
             PATH_SUFFIXES lib 
             )
set(DIRENT_LIBRARIES ${DIRENT_LIBRARY} )
########################################################################
endif ()
set(DIRENT_INCLUDE_DIRS ${DIRENT_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set DIRENT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(DIRENT  DEFAULT_MSG
                                  DIRENT_LIBRARIES DIRENT_INCLUDE_DIRS)

mark_as_advanced(DIRENT_INCLUDE_DIRS DIRENT_LIBRARIES)

# eof
