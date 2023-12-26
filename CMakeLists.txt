cmake_minimum_required(VERSION 3.14)

# Use `ccache` if it is installed in system's PATH
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()
# Export compilation database in JSON format.

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

project(CLox C)

add_executable(
  salmon
  src/main.c
  src/import.c
  src/memory.c
  src/value.c
  src/chunk.c
  src/compiler.c
  src/object.c
  src/scanner.c
  src/table.c
  src/vm.c
  src/debug.c
)
