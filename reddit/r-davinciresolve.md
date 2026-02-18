# r/davinciresolve

## r/davinciresolve

**Relevance:** HIGH - this is the main community for Resolve workflows, Fusion tricks, and OFX discussions.
**Rules check:** ⚠️ RESTRICTED
- Sub has an active “no selling / no soliciting” style rule; plugin/tool promotion and direct “here’s my plugin” posts tend to get removed.
- Keep it as a workflow question + share a useful technique without linking.
**Best post type:** D (Question post) with a concrete Fusion technique included.
**Optimal timing:** Tue-Thu, morning US time (heuristic: when you can reply quickly for 30-60 min).

---
**POST:**

Title: How do you do “line boil” / hand-drawn jitter in Resolve (Fusion / OFX / workflow)?

I’m trying to get that classic animation “line boil” look in DaVinci Resolve: subtle hand-drawn jitter where the frame holds a bit (like Posterize Time), not smooth wobble.

One thing that helped me get closer (Fusion): you can posterize *any* animated control by quantizing the time you sample it at.

Example (TimeStretcher → Source Time expression):
`floor(time / N) * N`

So if you drive a displacement/noise with a value sampled at that quantized time, the distortion “holds” for N frames and reads way more hand-drawn.

Questions for people who do this a lot:

- What’s your go-to node stack for line boil in Resolve? (Displace driven by FastNoise? Something better?)
- Do you posterize the *time* of the noise, or do you prefer stepping the *seed* / phase instead?
- Any gotchas to avoid (edge tearing, text shimmer, bad sampling) when you put it on top of graded footage?

I’m especially curious what settings you reach for first: strength (px), noise scale (px), and “boil fps” (hold rate).

---
**Your to-do before posting:**
- [ ] Re-check current subreddit rules + pinned threads (they change)
- [ ] Don’t include links in the post body
- [ ] Post when you can answer comments quickly

**First comment to add (if needed):**
No link unless someone explicitly asks. If asked, point them to r/ResolvePlugins.
