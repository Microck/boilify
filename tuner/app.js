const DEFAULTS = {
  strength: 5.0,
  size: 30,
  speed: 1.0,
  frames: 12,
  complexity: 3,
  noise: "smooth",
  seed: 0,
  animate: true,
  quality: "fast",
};

const PREVIEW_TIMELINE_FPS = 24;
const STEP_LOOP = 32768;

const $ = (id) => document.getElementById(id);

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function toast(message) {
  const el = $("toast");
  el.textContent = message;
  clearTimeout(toast._t);
  toast._t = setTimeout(() => el.textContent = "", 1200);
}

function createDefaultImageBitmap(width, height) {
  const canvas = document.createElement("canvas");
  canvas.width = width;
  canvas.height = height;
  const ctx = canvas.getContext("2d");
  
  const g = ctx.createLinearGradient(0, 0, width, height);
  g.addColorStop(0, "#1a2340");
  g.addColorStop(0.35, "#254b72");
  g.addColorStop(0.7, "#2a7db0");
  g.addColorStop(1, "#f0c9a6");
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, width, height);

  ctx.strokeStyle = "rgba(255, 255, 255, 0.85)";
  ctx.lineWidth = 5;
  ctx.beginPath();
  ctx.moveTo(width * 0.08, height * 0.20);
  ctx.bezierCurveTo(width * 0.28, height * 0.08, width * 0.50, height * 0.32, width * 0.70, height * 0.18);
  ctx.stroke();

  ctx.strokeStyle = "rgba(0, 0, 0, 0.45)";
  ctx.lineWidth = 10;
  ctx.beginPath();
  ctx.moveTo(width * 0.12, height * 0.70);
  ctx.lineTo(width * 0.88, height * 0.72);
  ctx.stroke();

  ctx.fillStyle = "rgba(0, 0, 0, 0.22)";
  ctx.fillRect(width * 0.07, height * 0.76, width * 0.42, height * 0.18);

  ctx.fillStyle = "rgba(255, 255, 255, 0.92)";
  ctx.font = "700 44px system-ui";
  ctx.fillText("LINE BOIL", width * 0.10, height * 0.85);
  ctx.font = "400 20px system-ui";
  ctx.fillStyle = "rgba(255, 255, 255, 0.85)";
  ctx.fillText("Frames + Strength are the main knobs", width * 0.10, height * 0.90);

  return createImageBitmap(canvas);
}

function createGl(canvas) {
  const gl = canvas.getContext("webgl2", { antialias: false, premultipliedAlpha: false });
  if (!gl) throw new Error("WebGL2 not available");
  return gl;
}

function compileShader(gl, type, src) {
  const shader = gl.createShader(type);
  gl.shaderSource(shader, src);
  gl.compileShader(shader);
  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    const info = gl.getShaderInfoLog(shader) || "";
    gl.deleteShader(shader);
    throw new Error(info);
  }
  return shader;
}

function createProgram(gl, vsSrc, fsSrc) {
  const vs = compileShader(gl, gl.VERTEX_SHADER, vsSrc);
  const fs = compileShader(gl, gl.FRAGMENT_SHADER, fsSrc);
  const program = gl.createProgram();
  gl.attachShader(program, vs);
  gl.attachShader(program, fs);
  gl.linkProgram(program);
  gl.deleteShader(vs);
  gl.deleteShader(fs);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
    const info = gl.getProgramInfoLog(program) || "";
    gl.deleteProgram(program);
    throw new Error(info);
  }
  return program;
}

const VS = `#version 300 es
precision highp float;
layout(location = 0) in vec2 aPos;
out vec2 vUv;
void main() {
  vUv = (aPos + 1.0) * 0.5;
  gl_Position = vec4(aPos, 0.0, 1.0);
}`;

const FS = `#version 300 es
precision highp float;
precision highp int;

uniform sampler2D uTex;
uniform vec2 uRes;
uniform float uStrength;
uniform float uSize;
uniform int uComplexity;
uniform int uNoiseType;
uniform int uSeed;
uniform int uAnimate;
uniform int uQuality;
uniform int uStep;

in vec2 vUv;
out vec4 outColor;

float fade(float t) { return t * t * (3.0 - 2.0 * t); }

uint mixBitsFast(uint v) {
  v ^= v >> 16; v *= 0x7feb352du; v ^= v >> 15; v *= 0x846ca68bu; v ^= v >> 16;
  return v;
}

uint mixBitsHigh(uint v) {
  v ^= v >> 17; v *= 0xed5ad4bbu; v ^= v >> 11; v *= 0xac4c1b51u; v ^= v >> 15;
  v *= 0x31848babu; v ^= v >> 14;
  return v;
}

float hashCell(ivec2 p, int seed) {
  uint h = uint(p.x) * 0x1f123bb5u ^ uint(p.y) * 0x59d2f15du ^ uint(seed) * 0x6c8e9cf5u;
  h = (uQuality == 1) ? mixBitsHigh(h) : mixBitsFast(h);
  return float(h & 0x00ffffffu) * (1.0 / 16777216.0);
}

float perlin(vec2 p, int seed) {
  ivec2 i = ivec2(floor(p));
  vec2 f = fract(p);
  vec2 s = vec2(fade(f.x), fade(f.y));
  float n00 = hashCell(i, seed), n10 = hashCell(i + ivec2(1, 0), seed);
  float n01 = hashCell(i + ivec2(0, 1), seed), n11 = hashCell(i + ivec2(1, 1), seed);
  return mix(mix(n00, n10, s.x), mix(n01, n11, s.x), s.y);
}

float applyNoiseType(float n) {
  return (uNoiseType == 0) ? n : 1.0 - abs(2.0 * n - 1.0);
}

float fbm(vec2 p, int seed) {
  float sum = 0.0, amp = 1.0, ampSum = 0.0, freq = 1.0;
  for (int i = 0; i < 6; i++) {
    if (i >= uComplexity) break;
    float n = applyNoiseType(perlin(p * freq, seed + i * 101));
    sum += n * amp; ampSum += amp; amp *= 0.5; freq *= 2.0;
  }
  return (ampSum > 0.0) ? sum / ampSum : 0.0;
}

void main() {
  vec2 dstPx = vUv * uRes;
  vec2 p = dstPx / max(1.0, uSize);
  int stepIndex = (uAnimate == 1) ? uStep : 0;
  int seedBase = uSeed + stepIndex * 1013;
  float nx = fbm(p, seedBase) * 2.0 - 1.0;
  float ny = fbm(p, seedBase + 1999) * 2.0 - 1.0;
  vec2 srcPx = clamp(dstPx + vec2(nx, ny) * uStrength, vec2(0.0), uRes - vec2(1.0));
  outColor = texture(uTex, (srcPx + vec2(0.5)) / uRes);
}`;

function createRenderer(canvas) {
  const gl = createGl(canvas);
  const program = createProgram(gl, VS, FS);
  const vao = gl.createVertexArray();
  gl.bindVertexArray(vao);
  const quad = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, quad);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1, -1, 1, 1, -1, 1, 1]), gl.STATIC_DRAW);
  gl.enableVertexAttribArray(0);
  gl.vertexAttribPointer(0, 2, gl.FLOAT, false, 0, 0);
  const tex = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, tex);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

  const uni = {
    uTex: gl.getUniformLocation(program, "uTex"),
    uRes: gl.getUniformLocation(program, "uRes"),
    uStrength: gl.getUniformLocation(program, "uStrength"),
    uSize: gl.getUniformLocation(program, "uSize"),
    uComplexity: gl.getUniformLocation(program, "uComplexity"),
    uNoiseType: gl.getUniformLocation(program, "uNoiseType"),
    uSeed: gl.getUniformLocation(program, "uSeed"),
    uAnimate: gl.getUniformLocation(program, "uAnimate"),
    uQuality: gl.getUniformLocation(program, "uQuality"),
    uStep: gl.getUniformLocation(program, "uStep"),
  };

  function setImage(bitmap) {
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, 1);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, bitmap);
  }

  function render(settings, stepIndex) {
    const w = canvas.width, h = canvas.height;
    gl.viewport(0, 0, w, h);
    gl.useProgram(program);
    gl.bindVertexArray(vao);
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.uniform1i(uni.uTex, 0);
    gl.uniform2f(uni.uRes, w, h);
    gl.uniform1f(uni.uStrength, settings.strength);
    gl.uniform1f(uni.uSize, settings.size);
    gl.uniform1i(uni.uComplexity, settings.complexity);
    gl.uniform1i(uni.uNoiseType, settings.noise === "ridged" ? 1 : 0);
    gl.uniform1i(uni.uSeed, settings.seed);
    gl.uniform1i(uni.uAnimate, settings.animate ? 1 : 0);
    gl.uniform1i(uni.uQuality, settings.quality === "high" ? 1 : 0);
    gl.uniform1i(uni.uStep, stepIndex);
    gl.drawArrays(gl.TRIANGLES, 0, 6);
  }

  return { setImage, render };
}

function main() {
  const canvas = $("after");
  const renderer = createRenderer(canvas);
  let bitmap = null, lastFrame = performance.now(), timeFrames = 0, stepIndex = 0;

  const ui = {
    strength: $("strength"), size: $("size"), speed: $("speed"), frames: $("frames"),
    complexity: $("complexity"), noise: $("noise"), seed: $("seed"), animate: $("animate"), quality: $("quality"),
    strengthOut: $("strengthOut"), sizeOut: $("sizeOut"), speedOut: $("speedOut"), framesOut: $("framesOut"),
    complexityOut: $("complexityOut"), seedOut: $("seedOut"), exportBox: $("exportBox"),
    copyJsonBtn: $("copyJsonBtn"), fileInput: $("fileInput"),
  };

  ui.strength.value = DEFAULTS.strength;
  ui.size.value = DEFAULTS.size;
  ui.speed.value = DEFAULTS.speed;
  ui.frames.value = DEFAULTS.frames;
  ui.complexity.value = DEFAULTS.complexity;
  ui.noise.value = DEFAULTS.noise;
  ui.seed.value = DEFAULTS.seed;
  ui.animate.checked = DEFAULTS.animate;
  ui.quality.value = DEFAULTS.quality;

  function readSettings() {
    return {
      strength: Number(ui.strength.value), size: Number(ui.size.value), speed: Number(ui.speed.value),
      frames: Number(ui.frames.value), complexity: Number(ui.complexity.value), noise: ui.noise.value,
      seed: Number(ui.seed.value), animate: Boolean(ui.animate.checked), quality: ui.quality.value,
    };
  }

  function syncOutputs(s) {
    ui.strengthOut.textContent = s.strength.toFixed(1);
    ui.sizeOut.textContent = s.size;
    ui.speedOut.textContent = s.speed.toFixed(2);
    ui.framesOut.textContent = s.frames;
    ui.complexityOut.textContent = s.complexity;
    ui.seedOut.textContent = s.seed;
    ui.exportBox.value = JSON.stringify(s, null, 2);
  }

  function resize() {
    const rect = canvas.getBoundingClientRect();
    const dpr = Math.min(2, window.devicePixelRatio || 1);
    const w = Math.round(rect.width * dpr), h = Math.round(rect.height * dpr);
    if (canvas.width !== w || canvas.height !== h) {
      canvas.width = w; canvas.height = h;
      if (bitmap) renderer.setImage(bitmap);
    }
  }

  function draw(now) {
    resize();
    const dt = (now - lastFrame) / 1000;
    lastFrame = now;
    const s = readSettings();
    if (s.animate) timeFrames += dt * PREVIEW_TIMELINE_FPS;
    const stepRaw = s.animate ? Math.floor(timeFrames / PREVIEW_TIMELINE_FPS * s.frames * s.speed) : 0;
    stepIndex = ((stepRaw % STEP_LOOP) + STEP_LOOP) % STEP_LOOP;
    syncOutputs(s);
    if (bitmap) renderer.render(s, stepIndex);
    requestAnimationFrame(draw);
  }

  ui.copyJsonBtn.addEventListener("click", async () => {
    await navigator.clipboard.writeText(ui.exportBox.value);
    toast("Copied");
  });

  ui.fileInput.addEventListener("change", async () => {
    const file = ui.fileInput.files?.[0];
    if (!file) return;
    const img = new Image();
    img.src = URL.createObjectURL(file);
    await img.decode();
    bitmap = await createImageBitmap(img);
    renderer.setImage(bitmap);
    toast("Loaded");
  });

  createDefaultImageBitmap(1600, 900).then(bmp => {
    bitmap = bmp;
    renderer.setImage(bmp);
    requestAnimationFrame(draw);
  });
}

main();
