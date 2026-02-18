## post 1: hook type d (i got tired of)

**viral potential:** high - instantly relatable (rebuilding fusion stacks), clear niche (davinci resolve line boil), concrete knobs

**seo keywords:** line boil, davinci resolve, openfx; fusion, ofx plugin; open source, vfx

**engagement trigger:** what's the one fusion setup you keep rebuilding?

---

davinci resolve line boil is one of those looks that’s weirdly hard to get “just right”.

i got tired of rebuilding the same fusion setup every time i needed that hand-drawn jitter.

not heat haze.

not smooth wobble.

the key is the frame *holds*.

so i built a tiny openfx (ofx) plugin for resolve that does “line boil” as a displacement field with posterize-time style stepping.

the knobs i kept reaching for ended up being the whole ui:

• strength (px)
• size (noise scale, px)
• speed
• boil fps (the hold rate — typical: 4–12)
• complexity (noise octaves)
• noise type (smooth / ridged)
• seed
• animate toggle
• quality (fast / high)

what surprised me: changing the *time behavior* (boil fps) matters more than swapping noise types. once the motion holds, it reads “drawn” immediately.

it’s mit-licensed and i’m trying to tune defaults so “strength 5 feels normal” on real footage.

what’s the one fusion setup you keep rebuilding that you wish was just a simple ofx knob?

link here: [**https://github.com/microck/boilify**](https://github.com/Microck/boilify)

---

**first comment (post immediately after):**

latest release + builds (win/mac/linux) are here: [**https://github.com/microck/boilify/releases/tag/v1.1.0**](https://github.com/Microck/boilify/releases/tag/v1.1.0)

**hashtags:**

[**#davinciresolve**](https://www.linkedin.com/feed/hashtag/?keywords=davinciresolve) [**#fusion**](https://www.linkedin.com/feed/hashtag/?keywords=fusion) [**#openfx**](https://www.linkedin.com/feed/hashtag/?keywords=openfx) [**#vfx**](https://www.linkedin.com/feed/hashtag/?keywords=vfx) [**#opensource**](https://www.linkedin.com/feed/hashtag/?keywords=opensource)
