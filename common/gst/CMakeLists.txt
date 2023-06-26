find_package(fmt  REQUIRED)
find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)

add_library(gst INTERFACE)
add_library(${PROJECT_NAME}::gst ALIAS gst)

target_link_libraries(gst INTERFACE fmt::fmt Boost::boost spdlog::spdlog PkgConfig::gstreamer)
target_compile_features(gst INTERFACE cxx_std_20)
target_include_directories(gst INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/common/gst/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
