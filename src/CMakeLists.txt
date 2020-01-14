add_executable(wgtcc
    ast.cc
    code_gen.cc
    cpp.cc
    encoding.cc
    error.cc
    evaluator.cc
    main.cc
    parser.cc
    scanner.cc
    scope.cc
    token.cc
    type.cc)

if (CMAKE_BUILD_TYPE AND WGTCC_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(wgtcc PRIVATE -g -O0 --coverage)
    target_link_options(wgtcc PRIVATE --coverage)
endif()

target_compile_features(wgtcc PRIVATE cxx_std_11)
target_compile_options(wgtcc PRIVATE -Wall -Wfatal-errors)
