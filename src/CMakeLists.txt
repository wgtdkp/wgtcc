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

target_compile_features(wgtcc PRIVATE cxx_std_11)
target_compile_options(wgtcc PRIVATE -Wall -Wfatal-errors)
