message("${Esc}[32mConfiguring ITK external library${Esc}[m")

## #############################################################################
## Look for Git
## #############################################################################
find_program(GIT_BIN NAMES git)
if (NOT GIT_BIN)
  find_package(Git)
  if(Git_FOUND)
    set(GIT_BIN ${GIT_EXECUTABLE})
  else()
    message(SEND_ERROR  "You need to install Git and add it to the PATH environment variable.")  
  endif()
else()
  mark_as_advanced(GIT_BIN)
endif()

if("${ITK_ROOT}" STREQUAL "")
  set(ITK_ROOT ${PROJECT_BINARY_DIR}/externals/itk-build)
endif()

set(ITK_ROOT "${ITK_ROOT}" CACHE PATH "" FORCE)

set(git_url https://github.com/InsightSoftwareConsortium/ITK.git)
set(git_tag v5.0.0)

ExternalProject_Add(
  itk
 
  GIT_REPOSITORY ${git_url}
  GIT_TAG ${git_tag}

  CMAKE_GENERATOR Ninja
  
  BINARY_DIR "${ITK_ROOT}"
  CMAKE_ARGS -DBUILD_SHARED_LIBS:BOOL=ON -DBUILD_EXAMPLES:BOOL=OFF -DBUILD_TESTING:BOOL=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo 
  
  INSTALL_COMMAND ""
  TEST_COMMAND ""
)

message("${Esc}[32mConfiguring ITK external library -- ${Esc}[1;32mDone${Esc}[m")
message("")
