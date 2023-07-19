include(CMakeParseArguments)

find_package(Git REQUIRED)

function(git_submodule)
  set(oneValueArgs PATH WORKING_DIRECTORY)
  cmake_parse_arguments(GitSubmodule "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})

  if(NOT GitSubmodule_PATH)
    message(FATAL_ERROR "Path to git submodule required for the update")
  endif()
  if(NOT GitSubmodule_WORKING_DIRECTORY)
    set(GitSubmodule_WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    message(
      AUTHOR_WARNING
        "Updating submodule at path ${GitSubmodule_PATH} relative to ${GitSubmodule_WORKING_DIRECTORY}"
    )
  endif()

  message(VERBOSE "Trying to update submodule in ${GitSubmodule_PATH}")
  if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${GitSubmodule_PATH}/CMakeLists.txt")
    execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init -- ${GitSubmodule_PATH}
            WORKING_DIRECTORY ${GitSubmodule_WORKING_DIRECTORY}
            RESULT_VARIABLE GIT_SUBMOD_RESULT)
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
      message(
              FATAL_ERROR
              "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules"
      )
  endif ()
  endif()
endfunction()
