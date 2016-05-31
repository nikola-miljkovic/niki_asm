cmake_minimum_required(VERSION 3.5)
project(niki_asm)

find_package(Doxygen)

include_directories(.)

set(SOURCE_FILES_AS
        nas/instruction.h
        nas/nas.c nas/nas.h
        nas/as.h nas/as.c
        nas/reloc.c nas/reloc.h
        nas/symbol.c nas/symbol.h

        parser/parser.c parser/parser.h
        parser/string_util.c parser/string_util.h)

set(SOURCE_FILES_EMU
        nas/instruction.h
        nas_run/nas_run.c nas_run/nas_run.h
        nas/as.h nas/as.c
        nas/reloc.c nas/reloc.h
        nas/symbol.c nas/symbol.h

        parser/parser.c parser/parser.h
        parser/string_util.c parser/string_util.h nas_run/nas_util.c nas_run/nas_util.h)

if(DOXYGEN_FOUND)
    set(DOXYGEN_INPUT ${SOURCE_FILES_AS})
    set(DOXYGEN_OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Doxyfile)

    add_custom_command(
            OUTPUT ${DOXYGEN_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E echo_append "Building API Documentation..."
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_INPUT}
            COMMAND ${CMAKE_COMMAND} -E echo "Done."
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${DOXYGEN_INPUT}
    )

    add_custom_target(apidoc ALL DEPENDS ${DOXYGEN_OUTPUT})

    add_custom_target(apidoc_forced
            COMMAND ${CMAKE_COMMAND} -E echo_append "Building API Documentation..."
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_INPUT}
            COMMAND ${CMAKE_COMMAND} -E echo "Done."
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )

endif(DOXYGEN_FOUND)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

add_executable(nas ${SOURCE_FILES_AS})
add_executable(nas_run ${SOURCE_FILES_EMU})