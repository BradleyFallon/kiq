# Property-Based Tests
# These tests use fast-check (JavaScript/TypeScript) and are run via Node.js

# Add custom target to run property tests
add_custom_target(run_property_tests
    COMMAND npm test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running property-based tests with fast-check..."
)

# Add custom target to install property test dependencies
add_custom_target(install_property_test_deps
    COMMAND npm install
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Installing property test dependencies..."
)
