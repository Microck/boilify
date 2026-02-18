<div align="center">
  <img src="Boilify.ofx.bundle/Contents/Resources/com.boilify.effect.png" width="150" alt="boilify logo" />

  <h1>Boilify</h1>

  <p><strong>line boil / hand-drawn jitter OpenFX plugin for DaVinci Resolve Studio 20+</strong></p>

  <p>
    <a href="LICENSE">
      <img src="https://img.shields.io/badge/license-MIT-blue.svg?style=flat-square" alt="license" />
    </a>
    <img src="https://img.shields.io/badge/host-davinci%20resolve-111111.svg?style=flat-square" alt="DaVinci Resolve" />
    <img src="https://img.shields.io/badge/build-cmake-064F8C.svg?style=flat-square" alt="cmake" />
    <img src="https://img.shields.io/badge/lang-c%2B%2B-00599C.svg?style=flat-square" alt="c++" />
  </p>

  <br />
</div>

## install

1. download zip from [releases](https://github.com/AcademySoftwareFoundation/boilify/releases)
2. extract `.ofx.bundle`
3. copy to `/Library/OFX/Plugins` (macOS), `C:\Program Files\Common Files\OFX\Plugins` (windows), or `/usr/OFX/Plugins` (linux)
4. restart resolve

## tuner

open `tuner/index.html` in a browser to preview and dial in settings before applying in resolve. adjust sliders, copy the json, and use as your preferred defaults.

## settings

| parameter | default | effect |
|-----------|---------|--------|
| strength | 5.0 | line jitter amount in pixels |
| size | 30.0 | noise scale in pixels. higher = larger wiggle chunks |
| speed | 1.0 | how quickly the pattern changes |
| boil fps | 12 | posterize-time style. typical: 4-12 |
| complexity | 3 | noise layers. higher = more detail (slower) |
| noise | smooth | `smooth` for classic boil, `ridged` for sharper texture |
| seed | 0 | random seed. change for different patterns |
| animate | on | toggle time-based boiling |
| quality | fast | `fast` for preview, `high` for nicer noise |

## algorithm

```
for each pixel:
  step = floor((time / frameRate) * boilFps * speed)
  seed' = seed + step
  dx = fbm(position / size, seed') * 2 - 1
  dy = fbm(position / size, seed' + const) * 2 - 1
  output[x,y] = input[x + dx * strength, y + dy * strength]
```

frame holds make the motion feel hand-drawn ("line boil") instead of smoothly interpolated.

## build

```bash
git clone https://github.com/AcademySoftwareFoundation/openfx.git
cmake -S . -B build -DOPENFX_ROOT=./openfx
cmake --build build
```

needs cmake 3.20+, c++17 compiler, OpenFX SDK.

## license

MIT
