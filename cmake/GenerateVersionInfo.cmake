set(VERSION_HEADER "${OUTPUT_FILE}")

find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY ${WORKSPACE_ROOT}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --format=%ci
    WORKING_DIRECTORY ${WORKSPACE_ROOT}
    OUTPUT_VARIABLE GIT_COMMIT_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${WORKSPACE_ROOT}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
else()
  set(GIT_COMMIT_HASH "unknown")
  set(GIT_COMMIT_DATE "unknown")
  set(GIT_BRANCH "unknown")
endif()

set(EXTERNALS_FILE "${WORKSPACE_ROOT}/externals.yml")
set(DEPENDENCY_LIST "")

if(EXISTS ${EXTERNALS_FILE})
  file(READ ${EXTERNALS_FILE} EXTERNALS_CONTENT)
  string(REGEX REPLACE "\r" "" EXTERNALS_CONTENT "${EXTERNALS_CONTENT}")
  # Split into lines
  string(REGEX REPLACE "\n" ";" EXTERNALS_LINES "${EXTERNALS_CONTENT}")

  set(CURRENT_NAME "")
  set(CURRENT_URL "")
  set(CURRENT_BRANCH "")
  set(CURRENT_COMMIT "")
  set(CURRENT_TAG "")

  foreach(LINE ${EXTERNALS_LINES})
    if(LINE MATCHES "^  - name: (.+)$")
      set(NEW_NAME "${CMAKE_MATCH_1}")
      string(STRIP "${NEW_NAME}" NEW_NAME)

      if(NOT CURRENT_NAME STREQUAL "" AND NOT CURRENT_URL STREQUAL "")
        set(VERSION_INFO "")
        if(NOT CURRENT_TAG STREQUAL "")
          set(VERSION_INFO "tag:${CURRENT_TAG}")
        elseif(NOT CURRENT_COMMIT STREQUAL "")
          set(VERSION_INFO "commit:${CURRENT_COMMIT}")
        elseif(NOT CURRENT_BRANCH STREQUAL "")
          set(VERSION_INFO "branch:${CURRENT_BRANCH}")
        else()
          string(REGEX MATCH "/([^/]+)/?$" URL_VERSION "${CURRENT_URL}")
          if(URL_VERSION)
            set(VERSION_INFO "${CMAKE_MATCH_1}")
          else()
            set(VERSION_INFO "unknown")
          endif()
        endif()
        string(APPEND DEPENDENCY_LIST "  {\"${CURRENT_NAME}\", \"${VERSION_INFO}\"},\n")
      endif()

      set(CURRENT_NAME "${NEW_NAME}")
      set(CURRENT_URL "")
      set(CURRENT_BRANCH "")
      set(CURRENT_COMMIT "")
      set(CURRENT_TAG "")
    elseif(LINE MATCHES "^    url: (.+)$")
      set(CURRENT_URL "${CMAKE_MATCH_1}")
      string(STRIP "${CURRENT_URL}" CURRENT_URL)
    elseif(LINE MATCHES "^    branch: (.+)$")
      set(CURRENT_BRANCH "${CMAKE_MATCH_1}")
      string(STRIP "${CURRENT_BRANCH}" CURRENT_BRANCH)
    elseif(LINE MATCHES "^    commit: (.+)$")
      set(CURRENT_COMMIT "${CMAKE_MATCH_1}")
      string(STRIP "${CURRENT_COMMIT}" CURRENT_COMMIT)
    elseif(LINE MATCHES "^    tag: (.+)$")
      set(CURRENT_TAG "${CMAKE_MATCH_1}")
      string(STRIP "${CURRENT_TAG}" CURRENT_TAG)
    endif()

  endforeach()

  if(NOT CURRENT_NAME STREQUAL "" AND NOT CURRENT_URL STREQUAL "")
    set(VERSION_INFO "")
    if(NOT CURRENT_TAG STREQUAL "")
      set(VERSION_INFO "tag:${CURRENT_TAG}")
    elseif(NOT CURRENT_COMMIT STREQUAL "")
      set(VERSION_INFO "commit:${CURRENT_COMMIT}")
    elseif(NOT CURRENT_BRANCH STREQUAL "")
      set(VERSION_INFO "branch:${CURRENT_BRANCH}")
    else()
      string(REGEX MATCH "/([^/]+)/?$" URL_VERSION "${CURRENT_URL}")
      if(URL_VERSION)
        set(VERSION_INFO "${CMAKE_MATCH_1}")
      else()
        set(VERSION_INFO "unknown")
      endif()
    endif()
    string(APPEND DEPENDENCY_LIST "  {\"${CURRENT_NAME}\", \"${VERSION_INFO}\"},\n")
  endif()
endif()

if(DEPENDENCY_LIST STREQUAL "")
  set(DEPENDENCY_LIST "  // no external dependency")
else() 
  string(REGEX REPLACE ",\n$" "\n" DEPENDENCY_LIST "${DEPENDENCY_LIST}")
endif()

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")

configure_file(${TEMPLATE_FILE} ${VERSION_HEADER} @ONLY)