// Part of Nexus Library
//
// The Color Studio, `studio::color[]`: edits the color seeds of the tokens (OKLCH numbers), from
// which `palette.css` derives the palette and both themes, and checks the contrast of every text
// color. The swatches are the page's own CSS colors, so they follow every edit without math here.

(function () {
  const S = window.NxStudio, C = S.color, { h, esc } = S;

  const GLOBAL = [
    ["neutral-h", "Neutral hue", 0, 360, 1], ["neutral-c", "Neutral chroma", 0, 0.04, 0.001],
    ["accent-l", "Base lightness", 0.45, 0.8, 0.01], ["tint-l", "Tint lightness", 0.7, 0.92, 0.01],
    ["tint-c", "Tint chroma ×", 0.3, 1, 0.05], ["text-l", "Text lightness", 0.4, 0.65, 0.01],
  ];
  // Changes over the seeds of the page; `cx` scales the chroma of every hue
  const PRESETS = {
    Theme: {},
    Paper: { "neutral-h": 75, "neutral-c": 0.014 },
    Graphite: { "neutral-c": 0, cx: 0.7 },
    Vivid: { cx: 1.3, "accent-l": 0.62, "text-l": 0.5 },
    Pastel: { cx: 0.6, "accent-l": 0.72, "tint-l": 0.86 },
  };
  // Text colors and the backgrounds they sit on
  const TEXT = ["ink", "muted", "link", ...S.HUES];
  const CODE = ["code-fg", "code-comment", "code-keyword", "code-type", "code-number", "code-string", "code-preproc"];

  const digits = (step) => String(step).split(".")[1]?.length || 0;
  const card = (title, hint) => h(`<section class="sx-card"><h4>${esc(title)}</h4>${hint ? `<p class="sx-hint">${hint}</p>` : ""}</section>`);
  const grade = (r) => (r >= 4.5 ? "pass" : r >= 3 ? "large" : "fail");

  function build(el) {
    S.init();
    el.textContent = "";
    S.toolbar(el, "Color Studio");
    if (!S.hasSeeds) {
      el.append(h('<p class="sx-hint">This theme has no color seeds (<code>--accent-l</code>, <code>--&lt;hue&gt;-h</code>, …; see <code>res/adoc/tokens.css</code>), so there is nothing to edit; the Font Studio still works.</p>'));
      return;
    }
    const grid = h('<div class="sx-grid"></div>');
    el.append(grid);
    const set = (k, v) => S.commit((s) => { s.seeds[k] = v; }, "seed");

    // ---- Seeds

    const seeds = card("Seeds", "OKLCH lightness 0–1, chroma about 0–0.37, hue 0–360°. Every color of the page derives from these.");
    const presets = h('<div class="sx-row sx-presets"><span class="sx-label">Preset</span></div>');
    const ranges = h('<div class="sx-ranges"></div>');
    seeds.append(presets, ranges);
    const sliders = GLOBAL.map(([k, label, min, max, step]) => {
      const r = h(`<label class="sx-range"><span>${esc(label)}</span><input type="range" min="${min}" max="${max}" step="${step}"><output></output></label>`);
      const input = r.querySelector("input"), out = r.querySelector("output");
      r.sync = () => { input.value = S.state.seeds[k]; out.textContent = (+input.value).toFixed(digits(step)); };
      input.addEventListener("input", () => { out.textContent = (+input.value).toFixed(digits(step)); set(k, +input.value); });
      ranges.append(r);
      return r;
    });
    for (const [name, p] of Object.entries(PRESETS)) {
      const b = h(`<button type="button" class="sx-btn">${name}</button>`);
      b.addEventListener("click", () => S.commit((s) => {
        s.seeds = JSON.parse(JSON.stringify(S.page.seeds));
        for (const [k, v] of Object.entries(p)) if (k !== "cx") s.seeds[k] = v;
        if (p.cx) for (const hu of S.HUES) if (s.seeds[`${hu}-c`] !== undefined) s.seeds[`${hu}-c`] = +(s.seeds[`${hu}-c`] * p.cx).toFixed(3);
      }, "preset"));
      presets.append(b);
    }

    // ---- Hues: angle and chroma; the swatches are the page's colors

    const hues = card("Hues", "Base: fills and charts. Tint: text in the dark theme and code. Text: the hue on the light page.");
    const table = h(`<table class="sx-table sx-hues"><thead><tr><th>Hue</th><th>Angle</th><th>Chroma</th>
      <th>Base</th><th>Tint</th><th>Text</th></tr></thead><tbody></tbody></table>`);
    hues.append(table);
    const inputs = [];
    for (const hu of S.HUES.filter((x) => S.state.seeds[`${x}-h`] !== undefined)) {
      const sw = (css) => `<td><i class="sx-sw" style="background:${css}"></i></td>`;
      const row = h(`<table><tr><td>${hu}</td>
        <td><input type="number" min="0" max="360" step="1" aria-label="${hu} angle"></td>
        <td><input type="number" min="0" max="0.37" step="0.005" aria-label="${hu} chroma"></td>
        ${sw(`var(--pal-${hu})`)}${sw(`var(--pal-${hu}-tint)`)}${sw(`oklch(var(--text-l) var(--${hu}-c) var(--${hu}-h))`)}</tr></table>`).rows[0];
      row.querySelectorAll("input").forEach((input, i) => {
        const k = `${hu}-${i ? "c" : "h"}`;
        input.sync = () => { input.value = S.state.seeds[k]; };
        input.addEventListener("input", () => { if (input.value !== "" && !Number.isNaN(+input.value)) set(k, +input.value); });
        inputs.push(input);
      });
      table.tBodies[0].append(row);
    }

    // ---- Contrast: WCAG 2 of each text color on its backgrounds, both themes

    const contrast = card("Contrast", "WCAG 2: 4.5:1 for text, 3:1 for large text and strokes. Measured as drawn, after sRGB mapping.");
    const box = h('<div class="sx-contrast"></div>');
    contrast.append(box);
    contrast.classList.add("sx-wide");
    let colors = null, pending = false;
    function matrix(rows, cols) {
      const head = cols.map(([label]) => `<th>${label}</th>`).join("");
      const body = rows.map((t) => `<tr><td>${t}</td>` + cols.map(([, mode, bg]) => {
        const fg = colors[mode][t], back = colors[mode][bg], r = C.ratio(fg, back);
        return `<td class="${grade(r)}"><span class="sx-sample" style="color:${fg};background:${back}">Aa</span>${r.toFixed(1)}</td>`;
      }).join("") + "</tr>").join("");
      return `<table class="sx-table"><thead><tr><th>Text</th>${head}</tr></thead><tbody>${body}</tbody></table>`;
    }
    function drawContrast() {
      pending = false;
      const all = [...TEXT, ...CODE, "bg", "surface", "code-bg"];
      colors = { light: C.measure("light", all), dark: C.measure("dark", all) };
      box.innerHTML = matrix(TEXT, [["Light page", "light", "bg"], ["Light surface", "light", "surface"], ["Dark page", "dark", "bg"], ["Dark surface", "dark", "surface"]]) +
        matrix(CODE, [["Code, light", "light", "code-bg"], ["Code, dark", "dark", "code-bg"]]);
    }
    const later = () => { if (!pending) { pending = true; requestAnimationFrame(drawContrast); } };

    grid.append(seeds, hues, contrast);
    const sync = () => { sliders.forEach((s) => s.sync()); inputs.forEach((i) => i.sync()); };
    S.on((source) => {
      if (source === "reset" || source === "preset") sync();
      if (source !== "mode") later();
    });
    sync();
    drawContrast();
  }

  document.querySelectorAll('.studio[data-studio="color"]').forEach(build);
})();
