find_package(fmt  REQUIRED)
find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)

add_library(net INTERFACE)
add_library(${PROJECT_NAME}::net ALIAS net)

target_link_libraries(net INTERFACE fmt::fmt Boost::boost spdlog::spdlog)
target_compile_features(net INTERFACE cxx_std_20)
target_include_directories(net INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/common/net/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)