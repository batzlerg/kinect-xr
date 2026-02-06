#!/bin/bash
# Run Kinect XR tests with hardware access (via sudo)
# Use after running setup-test-permissions.sh
#
# Usage:
#   ./scripts/run-hardware-tests.sh          # Run all tests
#   ./scripts/run-hardware-tests.sh unit     # Run unit tests only
#   ./scripts/run-hardware-tests.sh integration  # Run integration tests only
#   ./scripts/run-hardware-tests.sh runtime  # Run runtime tests only

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check build exists
if [ ! -d "$BUILD_DIR/bin" ]; then
    echo -e "${RED}Error: Build directory not found. Run 'cmake --build build' first.${NC}"
    exit 1
fi

run_test() {
    local test_name=$1
    local test_binary="$BUILD_DIR/bin/${test_name}"

    if [ ! -f "$test_binary" ]; then
        echo -e "${YELLOW}Warning: $test_binary not found, skipping.${NC}"
        return 0
    fi

    echo ""
    echo -e "${GREEN}Running $test_name...${NC}"
    echo "=========================================="

    # Run with sudo (should be passwordless after setup)
    if sudo "$test_binary"; then
        echo -e "${GREEN}✅ $test_name passed${NC}"
        return 0
    else
        echo -e "${RED}❌ $test_name failed${NC}"
        return 1
    fi
}

case "${1:-all}" in
    unit)
        run_test "unit_tests"
        ;;
    integration)
        run_test "integration_tests"
        ;;
    runtime)
        run_test "runtime_tests"
        ;;
    all)
        echo "Running all Kinect XR tests with hardware access"
        echo ""

        FAILED=0

        run_test "unit_tests" || FAILED=1
        run_test "integration_tests" || FAILED=1
        run_test "runtime_tests" || FAILED=1

        echo ""
        echo "=========================================="
        if [ $FAILED -eq 0 ]; then
            echo -e "${GREEN}✅ All tests passed!${NC}"
        else
            echo -e "${RED}❌ Some tests failed${NC}"
            exit 1
        fi
        ;;
    *)
        echo "Usage: $0 [unit|integration|runtime|all]"
        exit 1
        ;;
esac
