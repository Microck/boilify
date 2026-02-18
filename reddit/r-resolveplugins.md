# r/ResolvePlugins

## r/ResolvePlugins

**Relevance:** HIGH - this is literally the “share Resolve plugins/macros/fuses” lane.
**Rules check:** ✅ ALLOWED
- This sub exists for plugin sharing; still keep the post useful and ask for feedback.
**Best post type:** A (Problem-solved post) with a clear feedback ask.
**Optimal timing:** Weekend or weekday evening (heuristic) + respond fast in the first hour.

---
**POST:**

Title: Line boil / hand-drawn jitter OFX for Resolve (Boilify) + looking for default tuning feedback

I’m trying to get that “line boil” look in Resolve - the subtle hand-drawn jitter you see in animation where the movement *holds* for a few frames (Posterize Time style), not a smooth heat-haze wobble.

After rebuilding Fusion setups one too many times, I built a small OpenFX plugin that packages the core idea.

The controls are basically the knobs I kept reaching for:

- Strength (px)
- Size (noise scale, px)
- Speed
- Boil FPS (posterize-time style frame holds)
- Complexity (noise octaves)
- Noise type (Smooth / Ridged)
- Seed
- Animate toggle
- Quality (Fast / High)

What I’d love feedback on from people who use this look in real timelines:

- Do the defaults feel sane? (especially “Strength 5 should feel normal”)
- Any artifacts you see on edges / text / fine line art?
- Any parameter you *wish* was there for a boil effect?

If you try it, it would help a lot if you mention: Resolve version (Studio/free), timeline FPS, resolution, and whether you’re stacking NR/grading.

---
**Your to-do before posting:**
- [ ] Add the right flair (if your post UI requires it)
- [ ] Be ready to answer install + compatibility questions quickly
- [ ] Don’t argue with critical feedback; treat it like bug reports

**First comment to add (if needed):**
GitHub + releases: https://github.com/Microck/boilify

Install (Windows): download the latest `boilify-*-windows.zip`, extract `Boilify.ofx.bundle`, copy to:
`C:\\Program Files\\Common Files\\OFX\\Plugins`

macOS: `/Library/OFX/Plugins`

Linux: `/usr/OFX/Plugins`
