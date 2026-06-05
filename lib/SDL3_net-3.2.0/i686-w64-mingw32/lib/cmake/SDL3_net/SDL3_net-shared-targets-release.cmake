#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SDL3_net::SDL3_net-shared" for configuration "Release"
set_property(TARGET SDL3_net::SDL3_net-shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL3_net::SDL3_net-shared PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libSDL3_net.dll.a"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "SDL3::SDL3-shared"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SDL3_net.dll"
  )

list(APPEND _cmake_import_check_targets SDL3_net::SDL3_net-shared )
list(APPEND _cmake_import_check_files_for_SDL3_net::SDL3_net-shared "${_IMPORT_PREFIX}/lib/libSDL3_net.dll.a" "${_IMPORT_PREFIX}/bin/SDL3_net.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
