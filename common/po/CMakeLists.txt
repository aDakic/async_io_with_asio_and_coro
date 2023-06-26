find_package(fmt  REQUIRED)
find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)

add_library(po INTERFACE)
add_library(${PROJECT_NAME}::po ALIAS po)

target_link_libraries(po INTERFACE fmt::fmt Boost::program_options spdlog::spdlog)
target_compile_features(po INTERFACE cxx_std_20)
target_include_directories(po INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/common/po/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
