## post 3: hook type b (relatable pain)

**viral potential:** medium-high - it worked... until it didn't bug story + numbers; invites comments from graphics folks

**seo keywords:** shader hash, floating point precision, webgl, openfx; noise, perlin, displacement; performance

**engagement trigger:** what's the weirdest precision bug you’ve hit?

---

webgl shaders failing at step 60 is a special kind of pain.

everything looks fine.

then the motion stops “boiling” and starts repeating like it’s stuck.

i hit this while building a minimal browser tuner for a line boil effect (so i could record a clean demo and dial defaults).

root cause was embarrassingly common:

i used a classic `sin()`-based hash for noise.

it behaves… until your “time/step” grows.

then float precision + periodicity start to leak into the randomness, and your noise field turns into patterns.

the fix was simple but non-obvious if you haven’t been burned before:

• wrap the step (`step % 32768`) so the domain stays sane
• switch to integer bit-mixing for hashing (no trig)

after that, the boil loops forever without degrading, and the “boil fps” stepping stays stable.

this also reinforced something i keep forgetting:

for hand-drawn jitter, the algorithm isn’t the hard part.

the boring engineering is.

hashing.

bounds.

sampling.

and making sure the “preview path” doesn’t lie to you.

what’s the weirdest floating point / precision bug you’ve hit in a shader or per-pixel effect?

link here: [**https://github.com/microck/boilify**](https://github.com/Microck/boilify)

---

**first comment (post immediately after):**

the effect is openfx for resolve, but the hashing lesson applies to anything (webgl, hlsl, glsl, metal).

**hashtags:**

[**#webgl**](https://www.linkedin.com/feed/hashtag/?keywords=webgl) [**#graphicsprogramming**](https://www.linkedin.com/feed/hashtag/?keywords=graphicsprogramming) [**#shaders**](https://www.linkedin.com/feed/hashtag/?keywords=shaders) [**#vfx**](https://www.linkedin.com/feed/hashtag/?keywords=vfx) [**#opensource**](https://www.linkedin.com/feed/hashtag/?keywords=opensource)
