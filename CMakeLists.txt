cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
    JfttCompiler
    VERSION 0.1
    LANGUAGES CXX
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    include(CTest)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND CMAKE_CXX_COMPILER_ID MATCHES
                                                "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic )
endif()

option(ENABLE_SANITIZERS "Enables AddressSanitizer and UndefinedBehaviorSanitizer." OFF)
if (ENABLE_SANITIZERS)
    add_compile_options(
        -fno-omit-frame-pointer
        -fsanitize=address
        -fsanitize=undefined
        -fsanitize=float-cast-overflow
        -fsanitize-address-use-after-scope
        -O1
        -fno-sanitize-recover
        )
endif()

option(ENABLE_SAFE_STACK "Enables the SafeStack instrumentation pass by the Code Pointer Integrity Project" OFF)
if (ENABLE_SAFE_STACK)
    if (ENABLE_SANITIZERS)
        message(FATAL_ERROR "ENABLE_SAFE_STACK cannot be used in combination with ENABLE_SANITIZERS")
    endif()
    add_compile_options(-fsanitize=safe-stack)
endif()

add_subdirectory(external)
add_subdirectory(src)

option(ENABLE_TESTS "Enables tests" ON)
if (ENABLE_TESTS)
    add_subdirectory(tests)
endif()

option(BUILD_DEBUGGER "Builds debugger" OFF)
if (BUILD_DEBUGGER)
    add_subdirectory(debugger)
endif()
# option(ENABLE_EXAMPLES "Enables examples" ON)
# if (ENABLE_EXAMPLES)
#     add_subdirectory(examples)
# endif()

