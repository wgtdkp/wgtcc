#!/bin/bash

upload_coverage() {
    set -e

    # Create lcov report
    # capture coverage info
    lcov --directory build/src --capture --output-file coverage.info

    # filter out system and extra files.
    # To also not include test code in coverage add them with full path to the patterns: '*/tests/*'
    #lcov --remove coverage.info '/usr/*' "${HOME}"'/.cache/*' --output-file coverage.info

    # output coverage data for debugging (optional)
    lcov --list coverage.info

    # Uploading to CodeCov
    # '-f' specifies file(s) to use and disables manual coverage gathering and file search which has already been done above
    bash <(curl -s https://codecov.io/bash) -f coverage.info || echo "Codecov did not collect coverage reports"
}

CODECOV_TOKEN="d1f63060-c156-4aa2-a117-10e608268edd" upload_coverage
