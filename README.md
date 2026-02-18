<div align="center">
  <img src="Boilify.ofx.bundle/Contents/Resources/com.boilify.effect.png" width="150" alt="boilify logo" />

  <h1>Boilify</h1>

  <p><strong>heat distortion OpenFX plugin for DaVinci Resolve Studio 20+</strong></p>

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

1. download zip
2. extract `.ofx.bundle`
3. copy to `/Library/OFX/Plugins` (macOS), `C:\Program Files\Common Files\OFX\Plugins` (windows), or `/usr/OFX/Plugins` (linux)
4. restart resolve

## settings

| parameter | default | effect |
|-----------|---------|--------|
| strength | 1.0 | how far pixels shift |
| density | 2.0 | noise scale. higher = smaller bubbles |
| speed | 1.0 | animation rate |
| seed | 0 | noise seed. change for different patterns |
| animate | on | toggle time-based motion |
| quality | fast | `fast` for responsive preview, `high` for smoother detail |

## algorithm

```
for each pixel:
  noise = perlin(position * density + time * speed)
  offset = (cos(noise), sin(noise)) * strength
  output[x,y] = input[x + offset.x, y + offset.y]
```

perlin noise gives organic motion. trig functions make it circular. looks like heat haze.

## build

```bash
git clone https://github.com/AcademySoftwareFoundation/openfx.git
cmake -S . -B build -DOPENFX_ROOT=./openfx
cmake --build build
```

needs cmake 3.20+, c++17 compiler, OpenFX SDK.

## license

MIT
