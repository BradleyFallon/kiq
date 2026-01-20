# External Dependencies

This directory contains external dependencies required for building the Kick Drum Synthesizer.

## VST3 SDK

The VST3 SDK is required to build the VST3 plugin version of the synthesizer.

### Download and Setup

```bash
# From the project root directory
cd external
git clone --recursive https://github.com/steinbergmedia/vst3sdk.git
cd ..
```

### Version

The project is tested with VST3 SDK version 3.7.7 and later.

### License

The VST3 SDK is licensed under the Steinberg VST3 License. Please review the license terms at:
https://github.com/steinbergmedia/vst3sdk/blob/master/LICENSE.txt

### Alternative: Custom VST3 SDK Location

If you have the VST3 SDK installed elsewhere, you can specify its location when configuring CMake:

```bash
cmake .. -DVST3_SDK_PATH=/path/to/your/vst3sdk
```

## Directory Structure

After setup, this directory should contain:

```
external/
├── README.md (this file)
└── vst3sdk/
    ├── base/
    ├── pluginterfaces/
    ├── public.sdk/
    └── ...
```

## Notes

- The VST3 SDK is not included in this repository due to licensing restrictions
- You must download it separately from the official Steinberg repository
- The SDK is only required if you want to build the VST3 plugin (`BUILD_VST3=ON`)
- The standalone application can be built without the VST3 SDK
