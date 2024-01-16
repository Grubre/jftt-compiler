include(FetchContent)

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v5.0.0)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  message(STATUS "Fetching ftxui...")
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_executable(debugger debugger.cpp debugger-wm.cpp)
target_link_libraries(
  debugger
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component Common)
