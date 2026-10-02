cmake_minimum_required(VERSION 3.22)
string(ASCII 27 ESC)

foreach(ACTION IN ITEMS parse dump)
  set(ACTION_OPTION)
  if(ACTION STREQUAL "dump")
    set(ACTION_OPTION --dump-tokens)
  endif()

  foreach(MODE IN ITEMS auto always never)
    set(COLOR_OPTION)
    if(MODE STREQUAL "always")
      set(COLOR_OPTION -fcolor-diagnostics)
    elseif(MODE STREQUAL "never")
      set(COLOR_OPTION -fno-color-diagnostics)
    endif()

    execute_process(
      COMMAND "${CC1}" ${COLOR_OPTION} ${ACTION_OPTION} "${SOURCE}"
      RESULT_VARIABLE STATUS
      OUTPUT_VARIABLE TOKENS
      ERROR_VARIABLE DIAGNOSTIC
      TIMEOUT 4
    )
    if(NOT "${STATUS}" STREQUAL "1")
      message(FATAL_ERROR "${ACTION}/${MODE}: expected exit status 1, got ${STATUS}")
    endif()
    if(NOT "${TOKENS}" STREQUAL "")
      message(FATAL_ERROR "${ACTION}/${MODE}: tokens were printed for an invalid input")
    endif()

    string(FIND "${DIAGNOSTIC}" "${ESC}[" COLOR_POSITION)
    if(MODE STREQUAL "always")
      if(COLOR_POSITION EQUAL -1)
        message(FATAL_ERROR "Forced color did not emit ANSI sequences")
      endif()
    elseif(NOT COLOR_POSITION EQUAL -1)
      message(FATAL_ERROR "${ACTION}/${MODE}: ANSI sequences in captured output")
    endif()

    # Check the same layout regardless of whether color is enabled.
    string(REGEX REPLACE "${ESC}\\[[0-9;]*m" "" PLAIN "${DIAGNOSTIC}")
    string(FIND "${PLAIN}" "${SOURCE}:2:10: error: unknown character" HEADER)
    string(FIND "${PLAIN}" "    2 |   return @;\n      |          ^\n" EXCERPT)
    if(HEADER EQUAL -1 OR EXCERPT EQUAL -1)
      message(FATAL_ERROR "${ACTION}/${MODE}: incorrect diagnostic layout:\n${PLAIN}")
    endif()
  endforeach()
endforeach()
