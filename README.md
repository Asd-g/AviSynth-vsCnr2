# Description

Cnr2 is a temporal denoiser designed to denoise only the chroma.

According to the original author, this filter is suited for stationary rainbows or noisy analog captures.

Due to the way it works, Cnr2 is forced to run in a single thread. Cnr2 will also bottleneck the entire script, preventing it from using all the available CPU cores. One way to work around this issue is splitting the video into two or three chunks at scene changes, and filtering them in parallel with two or three instances of vspipe.

This is [a port of the VapourSynth plugin Cnr2](https://github.com/dubhater/vapoursynth-cnr2).

# Usage

```
vsCnr2 (clip input, string "mode", float "scdthr", int "ln", int "lm", int "un", int "um", int "vn", int "vm", bool "sceneChroma")
```

## Parameters:

- input\
    A clip to process. It must be in YUV 8..16-bit planar format with chroma subsampling 420, 422, 440 or 444.
    
- mode\
    Mode for each plane.\
    The letter `o` means wide mode, which is less sensitive to changes in the pixels, and more effective.\
    The letter `x` means narrow mode, which is less effective.\
    Default: "oxx".
    
- scdthr\
    Scene change detection threshold as percentage of maximum possible change.\
    Lower values make it more sensitive.\
    Must be between 0.0 and 100.0.\
    Default: 10.0.

- ln, un, vn\
    Sensitivity to movement in the Y, U, and V planes, respectively.\
    Higher values will denoise more, at the risk of introducing ghosting in the chroma.\
    Must be between 0 and 255.\
    Default: ln = 35; un = vn = 47.
    
- lm, um, vm\
    Strength of the denoising.\
    Higher values will denoise harder.\
    Must be between 0 and 255.\
    Default: lm = 192; um = vm = 255.
    
- sceneChroma\
    If True, the chroma is considered in the scene change detection.\
    Default: False.
    
# Building

## Windows

Use solution files.

## Linux

### Requirements

- Git
- C++11 compiler
- CMake >= 3.16

```
git clone https://github.com/Asd-g/AviSynth-vsCnr2 && \
cd AviSynth-vsCnr2 && \
mkdir build && \
cd build && \
cmake .. && \
make -j$(nproc) && \
sudo make install
```
