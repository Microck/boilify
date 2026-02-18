## post 2: hook type c (contrarian take)

**viral potential:** high - contrarian + teachable insight (worse time beats better noise), encourages saves

**seo keywords:** posterize time, line boil, davinci resolve; fusion expressions, displacement; animation, vfx

**engagement trigger:** do you step time or step seed/phase?

---

posterize time is the secret ingredient for “line boil”.

not better noise.

worse time.

i kept trying to get that hand-drawn jitter look in davinci resolve by tweaking noise types and octaves.

it always looked like… generic wobble.

then i flipped the problem: line boil isn’t about smooth motion.

it’s about tiny holds.

the most useful mental model i found:

1) build a displacement field (dx, dy) from 2d noise
2) quantize time so the field changes in steps

in fusion you can literally do it by sampling time in chunks.

example (timestretcher → source time expression):
`floor(time / n) * n`

once you force the “hold”, the same distortion suddenly reads like animation.

then you can dial the “boil fps” (4–12 feels like the sweet spot) instead of obsessing over noise flavor.

i baked that idea into an openfx plugin so i can keep the workflow dead simple, but the takeaway is bigger than any plugin:

if you want a hand-made feel, stop interpolating everything.

do you step time globally, or do you prefer stepping the seed/phase instead (and why)?

link here: [**https://github.com/microck/boilify**](https://github.com/Microck/boilify)

---

**first comment (post immediately after):**

if anyone wants, i can share a minimal fusion node graph version of this (no plugin) as a screenshot + expressions.

**hashtags:**

[**#animation**](https://www.linkedin.com/feed/hashtag/?keywords=animation) [**#davinciresolve**](https://www.linkedin.com/feed/hashtag/?keywords=davinciresolve) [**#fusion**](https://www.linkedin.com/feed/hashtag/?keywords=fusion) [**#vfx**](https://www.linkedin.com/feed/hashtag/?keywords=vfx) [**#videoediting**](https://www.linkedin.com/feed/hashtag/?keywords=videoediting)
