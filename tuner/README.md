# Boilify Tuner

Small static web page for dialing in Boilify parameter defaults.

## Run locally

From the repo root:

```bash
python3 -m http.server 5173 --directory tuner
```

Then open:

`http://127.0.0.1:5173/`

## Usage

- Use the controls to tweak `strength`, `size`, `speed`, `frames`, `complexity`, `noise`, `seed`, `animate`, `useAlpha`.
- When it looks right, click `Copy JSON` and send me the output. That becomes the recommended defaults.

## Notes

- This is meant to match the *feel* of the OFX plugin and help choose defaults/ranges.
- Hosts and pixel formats can behave slightly differently than a browser canvas.
