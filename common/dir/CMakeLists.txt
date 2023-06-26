find_package(fmt  REQUIRED)
find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)

add_library(dir INTERFACE)
add_library(${PROJECT_NAME}::dir ALIAS dir)

target_link_libraries(dir INTERFACE fmt::fmt Boost::boost spdlog::spdlog)
target_compile_features(dir INTERFACE cxx_std_20)
target_include_directories(dir INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/common/dir/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)