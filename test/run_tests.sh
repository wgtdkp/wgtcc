#!/bin/bash

readonly CUR_DIR=$(dirname "$(realpath -s $0)")

readonly WGTCC=${CUR_DIR=}/../build/src/wgtcc

run_test_case() {
    set -e

    test_case=$1

    echo "====== test case: [ ${test_case} ] ======"
    ${WGTCC} -no-pie -I${CUR_DIR=}/../include ${test_case}
    ./a.out
}

main () {
    test_case_to_run=""

    if [ ! -z "$#" ]; then
        test_case_to_run="$1"
    fi

    total_case_count=0
    failed_case_count=0

    echo "###### tests begin ######"
    for test_case in ${CUR_DIR}/*.c; do
        if [ ! -z ${test_case_to_run} ] && [ ${test_case} != ${test_case_to_run} ]; then
            continue
        fi

        run_test_case ${test_case}
        failed_case_count=$((failed_case_count + $?))
        total_case_count=$((total_case_count + 1))
    done

    echo "###### tests end ######"

    if [ ${failed_case_count} != 0 ]; then
        echo "${failed_case_count}/${total_case_count} cases failed"

        echo "###################################"
        return 1
    else
        echo "all(${total_case_count}) test cases passed"

        echo "###################################"
        return 0
    fi
}

main "$@"
