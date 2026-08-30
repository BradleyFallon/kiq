# Scripts

- `build/build.sh` builds the standalone and tests, runs CTest, then builds and
  validates the VST3 when `external/vst3sdk` exists.
- `build/build_standalone.sh` builds only the macOS standalone app.
- `run_standalone.sh` builds the standalone when needed and launches it.

Component-level shell compile tests from the retired synthesis engine were
removed. Current behavior is covered by the CMake/Google Test suite.
