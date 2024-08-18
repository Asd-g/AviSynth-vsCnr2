## Description

Cnr2 is a temporal denoiser designed to denoise only the chroma.

According to the original author, this filter is suited for stationary rainbows or noisy analog captures.

Due to the way it works, Cnr2 is forced to run in a single thread. Cnr2 will also bottleneck the entire script, preventing it from using all the available CPU cores. One way to work around this issue is splitting the video into two or three chunks at scene changes, and filtering them in parallel with two or three instances of vspipe.

This is [a port of the VapourSynth plugin Cnr2](https://github.com/dubhater/vapoursynth-cnr2).

### Requirements:

- AviSynth 2.60 / AviSynth+ 3.4 or later

- Microsoft VisualC++ Redistributable Package 2022 (can be downloaded from [here](https://github.com/abbodi1406/vcredist/releases))

### Usage:

```
vsCnr2 (clip input, string "mode", float "scdthr", int "ln", int "lm", int "un", int "um", int "vn", int "vm", bool "sceneChroma")
```

### Parameters:

- input<br>
    A clip to process.<br>
    It must be in YUV 8..16-bit planar format with chroma subsampling 420, 422, 440 or 444.

- mode<br>
    Mode for each plane.<br>
    The letter `o` means wide mode, which is less sensitive to changes in the pixels, and more effective.<br>
    The letter `x` means narrow mode, which is less effective.<br>
    Default: "oxx".

- scdthr<br>
    Scene change detection threshold as percentage of maximum possible change.<br>
    Lower values make it more sensitive.<br>
    Must be between 0.0 and 100.0.<br>
    Default: 10.0.

- ln, un, vn<br>
    Sensitivity to movement in the Y, U, and V planes, respectively.<br>
    Higher values will denoise more, at the risk of introducing ghosting in the chroma.<br>
    Must be between 0 and 255.<br>
    Default: ln = 35; un = vn = 47.

- lm, um, vm<br>
    Strength of the denoising.<br>
    Higher values will denoise harder.<br>
    Must be between 0 and 255.<br>
    Default: lm = 192; um = vm = 255.

- sceneChroma<br>
    If True, the chroma is considered in the scene change detection.<br>
    Default: False.

### Building:

```
Requirements:
- Git
- C++17 compiler
- CMake >= 3.25
- Ninja
```
```
git clone https://github.com/Asd-g/AviSynth-vsCnr2
cd AviSynth-vsCnr2
cmake -B build -G Ninja
ninja -C build
```
