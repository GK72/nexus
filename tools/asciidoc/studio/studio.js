// Part of Nexus Library
//
// The core of the studios (`studio.rb`): the tokens of the page as state, its live preview, the
// font list, the export. The Color and the Font Studio edit the same state, so one export holds
// both.
//
// The state is the tokens of `res/adoc/tokens.css`: the color seeds (OKLCH numbers, from which
// `palette.css` derives every color), the fonts and the sizes. Edits go into a <style> after the
// theme, so the whole page shows them; they are kept in localStorage for this viewer until "Reset".

(function () {
  if (window.NxStudio) return;

  // ------------------------------------------------------------------------ colors

  // A CSS color as the browser draws it in sRGB (OKLCH outside sRGB gets mapped), through a canvas
  const ctx = document.createElement("canvas").getContext("2d", { willReadFrequently: true });
  function toHex(css) {
    ctx.clearRect(0, 0, 1, 1);
    ctx.fillStyle = "#000";
    ctx.fillStyle = css;
    ctx.fillRect(0, 0, 1, 1);
    return "#" + [...ctx.getImageData(0, 0, 1, 1).data.slice(0, 3)].map((x) => x.toString(16).padStart(2, "0")).join("");
  }

  // WCAG 2 relative luminance and contrast ratio
  const toLin = (c) => (c <= 0.04045 ? c / 12.92 : ((c + 0.055) / 1.055) ** 2.4);
  const lum = (hex) => {
    const [r, g, b] = [1, 3, 5].map((i) => toLin(parseInt(hex.slice(i, i + 2), 16) / 255));
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
  };
  const ratio = (a, b) => { const x = lum(a), y = lum(b); return (Math.max(x, y) + 0.05) / (Math.min(x, y) + 0.05); };

  // The colors `tokens` of one theme, with the live edits: `data-theme` switched for a moment
  let probe = null;
  function measure(mode, tokens) {
    if (!probe) { probe = Object.assign(document.createElement("span"), { hidden: true }); document.body.append(probe); }
    const prev = root.getAttribute("data-theme"), out = {};
    root.setAttribute("data-theme", mode);
    for (const t of tokens) { probe.style.color = `var(--${t})`; out[t] = toHex(getComputedStyle(probe).color); }
    if (prev === null) root.removeAttribute("data-theme"); else root.setAttribute("data-theme", prev);
    return out;
  }

  const color = { toHex, lum, ratio, measure };

  // ------------------------------------------------------------------------ fonts

  // Google Fonts; `w` the weights requested (all of these exist for the family)
  const W4 = "400;500;600;700";
  const FONTS = [
    ["Inter Tight", "sans", W4], ["Archivo", "sans", W4], ["IBM Plex Sans", "sans", W4],
    ["IBM Plex Sans Condensed", "sans", W4], ["Geist", "sans", W4], ["Source Sans 3", "sans", W4],
    ["Barlow", "sans", W4], ["Barlow Semi Condensed", "sans", W4], ["Public Sans", "sans", W4],
    ["Atkinson Hyperlegible", "sans", "400;700"], ["Bricolage Grotesque", "sans", W4], ["Space Grotesk", "sans", W4],
    ["Schibsted Grotesk", "sans", W4], ["Instrument Sans", "sans", W4], ["Manrope", "sans", W4],
    ["Fira Sans", "sans", W4], ["Fira Sans Condensed", "sans", W4], ["Roboto Condensed", "sans", W4],
    ["Work Sans", "sans", W4],
    ["Source Serif 4", "serif", W4], ["IBM Plex Serif", "serif", W4], ["Newsreader", "serif", W4],
    ["Literata", "serif", W4], ["Fraunces", "serif", W4],
    ["JetBrains Mono", "mono", "400;500"], ["IBM Plex Mono", "mono", "400;500"], ["Geist Mono", "mono", "400;500"],
    ["Fira Code", "mono", "400;500"], ["Source Code Pro", "mono", "400;500"], ["Red Hat Mono", "mono", "400;500"],
    ["DM Mono", "mono", "400;500"], ["Inconsolata", "mono", "400;500"], ["Space Mono", "mono", "400;700"],
  ].map(([name, cat, w]) => ({ name, cat, w }));
  const FALLBACK = {
    sans: '"Segoe UI", system-ui, sans-serif',
    serif: 'Georgia, "Times New Roman", serif',
    mono: 'ui-monospace, "SFMono-Regular", Menlo, monospace',
  };
  const fontInfo = (name) => FONTS.find((f) => f.name === name);
  const fontSpec = (f) => f.name.replace(/ /g, "+") + ":wght@" + f.w;
  const fontsUrl = (names) => {
    const specs = [...new Set(names)].map(fontInfo).filter(Boolean).map((f) => "family=" + fontSpec(f));
    return specs.length ? "https://fonts.googleapis.com/css2?" + specs.join("&") + "&display=swap" : null;
  };
  const loaded = new Set();
  function loadFont(name) {
    const f = fontInfo(name);
    if (!f || loaded.has(name)) return;
    loaded.add(name);
    document.head.append(Object.assign(document.createElement("link"), { rel: "stylesheet", href: fontsUrl([name]) }));
  }
  const firstFamily = (stack) => (stack || "").split(",")[0].trim().replace(/["']/g, "");

  // ------------------------------------------------------------------------ tokens of the page

  // The color seeds: the theme has them when it has `--accent-l` (see `res/adoc/tokens.css`)
  const HUES = ["red", "orange", "yellow", "green", "teal", "blue", "purple"];
  const GLOBAL = ["neutral-h", "neutral-c", "accent-l", "tint-l", "tint-c", "text-l"];
  const PER_HUE = ["h", "c", "l", "tint-l"];
  const SEEDS = [...GLOBAL, ...HUES.flatMap((hu) => PER_HUE.map((k) => `${hu}-${k}`))];
  const TYPE = { text: "px", leading: "", scale: "", gap: "rem", pad: "rem", radius: "px", measure: "rem" };
  const ROLES = ["display", "body", "mono"];

  const root = document.documentElement;
  const prop = (name) => getComputedStyle(root).getPropertyValue("--" + name).trim();

  // The tokens of the theme, with the live edits switched off
  function readPage() {
    const live = document.getElementById("nx-studio-live");
    if (live) live.disabled = true;
    const seeds = {}, type = {}, fonts = {};
    for (const k of SEEDS) { const v = parseFloat(prop(k)); if (!Number.isNaN(v)) seeds[k] = v; }
    for (const [k, unit] of Object.entries(TYPE)) { const v = parseFloat(prop(k)); if (!Number.isNaN(v)) type[k] = { v, unit }; }
    for (const r of ROLES) { const stack = prop("font-" + r); if (stack) fonts[r] = { name: firstFamily(stack), stack }; }
    if (live) live.disabled = false;
    return { seeds, type, fonts };
  }

  // ------------------------------------------------------------------------ state

  const KEY = "nx-studio-seeds";
  const clone = (x) => JSON.parse(JSON.stringify(x));
  let page = null, state = null;
  const listeners = [];

  function init() {
    if (state) return;
    page = readPage();
    state = clone(page);
    try {
      const saved = JSON.parse(localStorage.getItem(KEY) || "null");
      if (saved) { state = merge(state, saved); apply(); }
    } catch { /* no storage: start from the page */ }
    for (const r of ROLES) if (state.fonts[r]) loadFont(state.fonts[r].name);
  }

  // Saved values only for the tokens the page has: a theme without them keeps working
  function merge(base, saved) {
    const out = clone(base);
    for (const g of ["seeds", "type", "fonts"]) for (const k of Object.keys(out[g])) if (saved[g]?.[k] !== undefined) out[g][k] = saved[g][k];
    return out;
  }

  const stackOf = (f) => {
    const info = fontInfo(f.name);
    return info ? `"${f.name}", ${FALLBACK[info.cat]}` : f.stack;
  };
  const num = (v) => String(+v.toFixed(3));

  // The base sizes of the size picker (`--text`), the same as `:theme-size:` in `theme.rb`
  const SIZES = { small: 15, normal: 16, large: 17 };

  // The tokens file: the layout and the comments of `res/adoc/tokens.css`, the values of the state
  function tokensCss({ imports }) {
    const s = state, sd = s.seeds, lines = [];
    const has = (k) => sd[k] !== undefined;
    const decl = (k) => `  --${k}: ${num(sd[k])};`;
    if (imports) {
      const url = fontsUrl(ROLES.map((r) => s.fonts[r]?.name).filter(Boolean));
      if (url) lines.push(`@import url("${url}");`, "");
      lines.push("/*", " * Part of Nexus Library", " *",
        " * Nexus design tokens: the seeds of the colors, the fonts and the spacing. `palette.css` derives",
        " * every color from the seeds in OKLCH; the styles of `styles/` read the colors and these. The",
        " * Color and Font Studio (`studio::color[]`, `studio::font[]`) export a file in this format.", " */", "");
    }
    lines.push(":root {");
    const group = (comment, body) => { if (body.length) lines.push(...(lines.at(-1) === ":root {" ? [] : [""]), ...comment, ...body); };
    group(["  /* Neutrals: OKLCH hue and chroma of the greys of both themes */"], ["neutral-h", "neutral-c"].filter(has).map(decl));
    group(["  /* Accents: one lightness for the bases (fills, strokes, charts), one for the tints (text in the",
      "     dark theme, code), which keep a part of the chroma, and one for text on the light page */"],
    ["accent-l", "tint-l", "tint-c", "text-l"].filter(has).map(decl));
    group(["  /* Hues: angle and chroma; `-l`, `-tint-l` for a hue off the common lightness */"],
      HUES.filter((hu) => has(`${hu}-h`)).map((hu) => "  " + PER_HUE.filter((k) => has(`${hu}-${k}`)).map((k) => decl(`${hu}-${k}`).trim()).join("  ")));
    group(["  /* Type and spacing */"], [
      ...ROLES.filter((r) => s.fonts[r]).map((r) => `  --font-${r}: ${stackOf(s.fonts[r])};`),
      ...Object.entries(s.type).map(([k, t]) => `  --${k}: ${num(t.v)}${t.unit};`),
    ]);
    lines.push("}");
    return lines.join("\n") + "\n";
  }

  function apply() {
    let el = document.getElementById("nx-studio-live");
    if (!el) { el = Object.assign(document.createElement("style"), { id: "nx-studio-live" }); document.head.append(el); }
    el.textContent = tokensCss({ imports: false });
  }

  // Applies an edit: `fn` changes the state; `source` names the editor, which need not redraw
  function commit(fn, source) {
    if (fn) fn(state);
    apply();
    try { localStorage.setItem(KEY, JSON.stringify(state)); } catch { /* not kept */ }
    for (const l of listeners) l(source);
  }

  function reset() {
    state = clone(page);
    document.getElementById("nx-studio-live")?.remove();
    try { localStorage.removeItem(KEY); } catch { /* nothing kept */ }
    for (const l of listeners) l("reset");
  }

  // ------------------------------------------------------------------------ shared UI

  const esc = (s) => String(s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" })[c]);
  const h = (html) => { const t = document.createElement("template"); t.innerHTML = html.trim(); return t.content.firstElementChild; };

  async function copyText(btn, text) {
    try { await navigator.clipboard.writeText(text); }
    catch {
      const t = Object.assign(document.createElement("textarea"), { value: text });
      document.body.append(t); t.select(); document.execCommand("copy"); t.remove();
    }
    btn.classList.add("done");
    setTimeout(() => btn.classList.remove("done"), 900);
  }

  function themeMode() { return root.getAttribute("data-theme") || "auto"; }
  function setThemeMode(mode) {
    if (mode === "auto") root.removeAttribute("data-theme"); else root.setAttribute("data-theme", mode);
    for (const l of listeners) l("mode");
  }
  const effectiveMode = () => {
    const m = themeMode();
    return m !== "auto" ? m : matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
  };

  // The bar on top of each studio: theme, reset, export
  function toolbar(container, title) {
    const bar = h(`<div class="sx-bar">
      <strong class="sx-title">${esc(title)}</strong>
      <span class="sx-seg" role="group" aria-label="Theme">
        <button type="button" data-mode="auto">Auto</button><button type="button" data-mode="light">Light</button><button type="button" data-mode="dark">Dark</button>
      </span>
      <button type="button" class="sx-btn" data-act="export" aria-expanded="false">Export tokens</button>
      <button type="button" class="sx-btn" data-act="reset" title="Back to the theme of the page">Reset</button>
    </div>`);
    const out = h(`<div class="sx-export" hidden>
      <div class="sx-row"><span class="sx-hint">The tokens file of the theme (format of <code>res/adoc/tokens.css</code>); use it with <code>ADOC_TOKENS=FILE tools/doc.sh</code>.</span>
        <button type="button" class="sx-btn" data-act="copy">Copy</button><button type="button" class="sx-btn" data-act="download">Download</button></div>
      <textarea class="sx-code" rows="16" readonly spellcheck="false"></textarea>
    </div>`);
    const area = out.querySelector("textarea");
    const sync = () => {
      bar.querySelectorAll("[data-mode]").forEach((b) => b.setAttribute("aria-pressed", b.dataset.mode === themeMode()));
      if (!out.hidden) area.value = tokensCss({ imports: true });
    };
    bar.addEventListener("click", (e) => {
      const b = e.target.closest("button"); if (!b) return;
      if (b.dataset.mode) setThemeMode(b.dataset.mode);
      if (b.dataset.act === "reset") reset();
      if (b.dataset.act === "export") { out.hidden = !out.hidden; b.setAttribute("aria-expanded", !out.hidden); sync(); }
    });
    out.addEventListener("click", (e) => {
      const b = e.target.closest("button"); if (!b) return;
      if (b.dataset.act === "copy") copyText(b, area.value);
      if (b.dataset.act === "download") {
        const a = Object.assign(document.createElement("a"), {
          href: URL.createObjectURL(new Blob([area.value], { type: "text/css" })), download: "tokens.css",
        });
        a.click(); setTimeout(() => URL.revokeObjectURL(a.href), 1000);
      }
    });
    listeners.push(sync);
    container.append(bar, out);
    sync();
  }

  window.NxStudio = {
    color, FONTS, FALLBACK, HUES, SIZES, fontInfo, loadFont,
    init, commit, reset, toolbar, esc, h, copyText, effectiveMode,
    get state() { return state; },
    get page() { return page; },
    get hasSeeds() { return state.seeds["accent-l"] !== undefined; },
    on: (fn) => listeners.push(fn),
  };
})();
