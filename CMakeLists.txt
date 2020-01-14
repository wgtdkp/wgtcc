cmake_minimum_required(VERSION 3.13.1)
project(wgtcc)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

option(WGTCC_COVERAGE "Enable code coverage" OFF)

add_subdirectory(src)

install(TARGETS wgtcc
    DESTINATION bin)

install(DIRECTORY include
    DESTINATION wgtcc)

# cmake doesn't provide the `make uninstall` target.
# so we create a customerized uninstall target.
# - https://gitlab.kitware.com/cmake/community/wikis/FAQ#can-i-do-make-uninstall-with-cmake
add_custom_target(uninstall
    COMMAND rm ${CMAKE_INSTALL_PREFIX}/bin/${WGTCC_EXECUTALBE_NAME}
    COMMAND rm -rf ${CMAKE_INSTALL_PREFIX}/wgtcc)
