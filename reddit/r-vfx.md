# r/vfx

## r/vfx

**Relevance:** MEDIUM-HIGH - not a “Resolve plugin” sub, but this is a nice technical rabbit hole: animation-style line boil, sampling/stepping strategies, and real-world performance constraints.
**Rules check:** ✅ ALLOWED (with care)
- Use proper flair and keep it as discussion/learning.
- Avoid tutorial framing (r/vfx often redirects tutorials elsewhere); keep it as “here’s what I learned + questions”.
- Don’t lead with links; if asked, share in a comment.
**Best post type:** B (Technical deep-dive).
**Optimal timing:** Weekday morning US / afternoon EU overlap (heuristic).

---
**POST:**

Title: Line boil (hand-drawn jitter) as a displacement field: stepped time vs smooth time, and a hashing gotcha

I went down a rabbit hole implementing an “animation line boil” look as a simple displacement (I built a small OpenFX prototype for it): sample a 2D noise field per pixel, use it as (dx, dy), and resample the source.

The part that made it *feel* like line boil (instead of generic wobble) wasn’t the noise type - it was time.

Two approaches I tried:

1) Smooth time: noise phase changes continuously
2) Stepped time: quantize time so the distortion holds for a few frames (Posterize Time style)

Stepped time instantly reads more hand-drawn.

Interesting gotcha I hit: a classic GPU-style `sin()` hash works fine for small time indices, but starts to degrade once the “step” grows (floating point precision / periodicity issues). Fix was to:

- Wrap step (`step % 32768`)
- Use an integer bit-mix hash instead of trig

Questions for people who’ve shipped similar “boil / jitter / hand-animated” distortion:

- Do you prefer stepping time globally, stepping just the seed/phase, or stepping per-octave?
- For a boil look, do you bias toward smooth Perlin-style noise, ridged noise, or something like curl noise?
- Any recommendations for avoiding edge artifacts when you displace in screen-space (especially over text / UI / high-contrast lines)?

I’m trying to keep it editor-friendly too, so I’m curious what your go-to performance levers are for CPU/GPU per-pixel effects.

---
**Your to-do before posting:**
- [ ] Pick an appropriate flair (Discussion / Learning)
- [ ] Don’t include links in the post body
- [ ] Be ready to answer technical questions quickly

**First comment to add (if needed):**
If anyone wants the reference implementation (OpenFX, MIT), I can share the repo.
