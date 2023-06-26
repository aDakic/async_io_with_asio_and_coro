find_package(fmt  REQUIRED)
find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)

add_library(exe INTERFACE)
add_library(${PROJECT_NAME}::exe ALIAS exe)

target_link_libraries(exe INTERFACE fmt::fmt Boost::boost spdlog::spdlog)
target_compile_features(exe INTERFACE cxx_std_20)
target_include_directories(exe INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/common/exe/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)