# Contributing to Kick Drum Synthesizer

Thank you for your interest in contributing to the Kick Drum Synthesizer project!

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/yourusername/kick-drum-synthesizer.git`
3. Create a feature branch: `git checkout -b feature/my-feature`
4. Make your changes
5. Run tests to ensure everything works
6. Commit your changes: `git commit -am 'Add new feature'`
7. Push to your fork: `git push origin feature/my-feature`
8. Create a Pull Request

## Development Setup

See [BUILDING.md](BUILDING.md) for detailed build instructions.

Quick setup:
```bash
# Configure the portable core engine and tests
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_VST3=OFF -DBUILD_STANDALONE=OFF
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure
```

## Code Style

### C++ Code Style

- Use C++17 features
- Follow the existing code style
- Use meaningful variable and function names
- Add comments for complex logic
- Keep functions focused and small

Example:
```cpp
namespace KickDrum {

class SineDriver {
public:
    // Generate the next audio sample
    float generate();
    
    // Set the oscillator frequency in Hz
    void setFrequency(float frequency);
    
private:
    float phase_ = 0.0f;
    float frequency_ = 440.0f;
    float sampleRate_ = 48000.0f;
};

} // namespace KickDrum
```

### Naming Conventions

- Classes: `PascalCase` (e.g., `SineDriver`, `AudioEngine`)
- Functions: `camelCase` (e.g., `generate()`, `setFrequency()`)
- Private members: `camelCase_` with trailing underscore (e.g., `phase_`, `frequency_`)
- Constants: `kPascalCase` (e.g., `kMaxPolyphony`)
- Namespaces: `PascalCase` (e.g., `KickDrum`)

### File Organization

- Header files: `.h` extension
- Implementation files: `.cpp` extension
- One class per file (generally)
- Header guards: `#pragma once`

## Testing

### Writing Tests

All new features should include tests:

1. **Unit Tests** (C++ with Google Test)
   - Test specific functionality
   - Test edge cases
   - Test error handling

### Test Example

```cpp
// tests/unit/generators/SineDriverTest.cpp
#include <gtest/gtest.h>
#include "generators/SineDriver.h"

TEST(SineDriverTest, GeneratesCorrectFrequency) {
    SineDriver driver;
    driver.initialize(48000.0f);
    driver.setFrequency(440.0f);
    
    // Test that frequency is correct
    // ... test implementation
}

TEST(SineDriverTest, HandlesZeroFrequency) {
    SineDriver driver;
    driver.initialize(48000.0f);
    driver.setFrequency(0.0f);
    
    // Should not crash or produce NaN
    float sample = driver.generate();
    EXPECT_FALSE(std::isnan(sample));
}
```

### Running Tests

```bash
# C++ unit tests
cd build
ctest --output-on-failure

# Run specific test
./bin/kick_drum_tests --gtest_filter=SineDriverTest.*
```

## Pull Request Guidelines

### Before Submitting

- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] New features include tests
- [ ] Documentation is updated
- [ ] Code follows project style
- [ ] Commit messages are clear

### PR Description

Include in your PR description:
- What changes were made
- Why the changes were made
- How to test the changes
- Any breaking changes
- Related issues (if any)

### Example PR Description

```markdown
## Refine Pitch Trajectory Curvature

### Changes
- Refined the per-segment curve mapping
- Added endpoint and monotonicity coverage

### Why
Makes trajectory shaping easier to control while preserving node values.

### Testing
- Run unit tests: `./build/bin/kick_drum_tests --gtest_filter=TrajectoryTest.*`

### Breaking Changes
Changes the sound of non-zero curve values.

### Related Issues
Closes #42
```

## Areas for Contribution

### High Priority

- Core DSP implementation (generators, envelopes, effects)
- Test coverage improvements
- Documentation improvements
- Bug fixes

### Medium Priority

- UI implementation
- Preset library expansion
- Performance optimizations
- Platform support (Linux, Windows)

### Low Priority

- Additional features
- Code refactoring
- Build system improvements

## Reporting Bugs

### Before Reporting

1. Check if the bug has already been reported
2. Try to reproduce with the latest version
3. Gather relevant information

### Bug Report Template

```markdown
## Bug Description
Clear description of the bug

## Steps to Reproduce
1. Step one
2. Step two
3. ...

## Expected Behavior
What should happen

## Actual Behavior
What actually happens

## Environment
- OS: macOS 13.0
- Build type: Release
- Version: 1.0.0
- DAW (if VST3): Ableton Live 11

## Additional Context
Any other relevant information
```

## Feature Requests

We welcome feature requests! Please:

1. Check if the feature has already been requested
2. Explain the use case
3. Describe the proposed solution
4. Consider implementation complexity

### Feature Request Template

```markdown
## Feature Description
Clear description of the feature

## Use Case
Why is this feature needed?

## Proposed Solution
How should it work?

## Alternatives Considered
Other ways to achieve the same goal

## Additional Context
Any other relevant information
```

## Code Review Process

1. Maintainers will review your PR
2. Feedback will be provided
3. Make requested changes
4. Once approved, PR will be merged

### Review Criteria

- Code quality and style
- Test coverage
- Documentation
- Performance impact
- Compatibility

## Communication

- GitHub Issues: Bug reports and feature requests
- GitHub Discussions: General questions and discussions
- Pull Requests: Code contributions

## License

By contributing, you agree that your contributions will be licensed under the same license as the project.

## Questions?

If you have questions about contributing, feel free to:
- Open a GitHub Discussion
- Comment on an existing issue
- Reach out to the maintainers

Thank you for contributing! 🎵
