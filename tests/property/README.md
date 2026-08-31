# Property tests

This directory reserves the JavaScript property-test setup. The active DSP
behavior suite currently lives in `tests/unit` and runs through CTest. Add new
property tests here only when they cover invariants that are clearer with
generated inputs than with focused C++ examples.
