# Testing Framework Setup

# Google Test for C++ unit tests
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()
include(GoogleTest)

# C++ Unit Tests
add_subdirectory(unit)

# Property-Based Tests (using fast-check via Node.js)
# Note: Property tests will be run separately via npm/node
# See tests/property/README.md for setup instructions
add_subdirectory(property)
