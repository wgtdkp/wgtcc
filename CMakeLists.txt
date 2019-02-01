project(WGTCC)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS "-g -std=c++11 -Wall -Wfatal-errors -DDEBUG")

set(WGTCC_EXECUTALBE_NAME wgtcc)
aux_source_directory(src SRC_FILES)
add_executable(${WGTCC_EXECUTALBE_NAME} ${SRC_FILES})

# install path : ${CMAKE_INSTALL_PREFIX}
# default value is `/usr/local`
install(
    TARGETS ${WGTCC_EXECUTALBE_NAME}
    RUNTIME DESTINATION bin
)
install(
    DIRECTORY include
    DESTINATION wgtcc
)

# cmake doesn't provide the `make uninstall` target.
# so we create a customerized uninstall target.
# - https://gitlab.kitware.com/cmake/community/wikis/FAQ#can-i-do-make-uninstall-with-cmake
add_custom_target(uninstall
    COMMAND rm ${CMAKE_INSTALL_PREFIX}/bin/${WGTCC_EXECUTALBE_NAME}
    COMMAND rm -rf ${CMAKE_INSTALL_PREFIX}/wgtcc)

# run tests
enable_testing()
aux_source_directory(test TEST_FILES)
set(WGTCC_EXECUTABLE ${CMAKE_BINARY_DIR}/${WGTCC_EXECUTALBE_NAME})
set(WGTCC_HEADER_FILES ${CMAKE_SOURCE_DIR}/include)
set(TEST_BINARY_DIR ${CMAKE_BINARY_DIR}/tests)
file(MAKE_DIRECTORY ${TEST_BINARY_DIR})
add_test(
    NAME check_wgtcc_executable
    COMMAND ${WGTCC_EXECUTABLE} -h
)
foreach(TEST_FILE ${TEST_FILES})
    get_filename_component(TEST_FILE_NAME ${TEST_FILE} NAME_WE)
    add_test(
        NAME "COMPILE--${TEST_FILE_NAME}"
        COMMAND ${WGTCC_EXECUTABLE} -I ${WGTCC_HEADER_FILES} ${CMAKE_SOURCE_DIR}/${TEST_FILE}
                -o ${TEST_BINARY_DIR}/${TEST_FILE_NAME}
    )
    add_test(
        NAME "RUN--${TEST_FILE_NAME}"
        COMMAND ${TEST_BINARY_DIR}/${TEST_FILE_NAME}
    )
endforeach()
