// Part of Nexus Library
//
// The Font Studio, `studio::font[]`: the display, body and mono families (Google Fonts, loaded on
// pick), the base size, leading, heading scale and the spacing tokens. Edits restyle the whole page;
// the export is a tokens file with the font import (see `studio.js`).

(function () {
  const S = window.NxsStudio, { h, esc } = S;

  const ROLES = [["display", "Display", "Headings"], ["body", "Body", "Text"], ["mono", "Mono", "Code"]];
  const RANGES = {
    text: ["Size", 12, 18, 0.5, "px"], leading: ["Leading", 1.2, 1.8, 0.05, ""], scale: ["Heading scale", 1.05, 1.4, 0.01, ""],
    gap: ["Block gap", 0.2, 1.6, 0.05, "rem"], pad: ["Padding", 0.15, 1, 0.05, "rem"], radius: ["Radius", 0, 12, 1, "px"],
    measure: ["Measure", 40, 90, 1, "rem"],
  };
  const PRESETS = {
    Theme: null,
    Dense: [["IBM Plex Sans Condensed", "IBM Plex Sans", "IBM Plex Mono"], { text: 14, leading: 1.35, scale: 1.15, gap: 0.4, pad: 0.3, radius: 2, measure: 62 }],
    Technical: [["Schibsted Grotesk", "Geist", "Geist Mono"], { text: 15, leading: 1.45, scale: 1.2, gap: 0.6, pad: 0.4, radius: 4, measure: 58 }],
    Readable: [["Source Sans 3", "Source Serif 4", "Source Code Pro"], { text: 16, leading: 1.55, scale: 1.22, gap: 0.85, pad: 0.5, radius: 4, measure: 46 }],
    Airy: [["Bricolage Grotesque", "Geist", "Geist Mono"], { text: 16, leading: 1.6, scale: 1.25, gap: 1.25, pad: 0.65, radius: 10, measure: 62 }],
  };
  const SAMPLE = {
    display: "Rendering pipeline 0123",
    body: "The quick brown fox jumps over the lazy dog; 1 Il| 0O.",
    mono: "auto frame = renderer.begin(\"main\", 1.0f); // 0O 1l",
  };

  const card = (title, hint) => h(`<section class="sx-card"><h4>${esc(title)}</h4>${hint ? `<p class="sx-hint">${hint}</p>` : ""}</section>`);

  function fontSelect(role, current) {
    const cats = role === "mono" ? ["mono"] : ["sans", "serif"];
    const groups = cats.map((cat) => `<optgroup label="${cat}">` +
      S.FONTS.filter((f) => f.cat === cat).map((f) => `<option${f.name === current ? " selected" : ""}>${esc(f.name)}</option>`).join("") + "</optgroup>");
    const known = S.fontInfo(current);
    return h(`<select>${known ? "" : `<option selected>${esc(current)}</option>`}${groups.join("")}</select>`);
  }

  function build(el) {
    S.init();
    el.textContent = "";
    S.toolbar(el, "Font Studio");
    const grid = h('<div class="sx-grid"></div>');
    el.append(grid);

    // ---- Families

    const fam = card("Families", "Google Fonts, loaded when picked. The page falls back to the system font until one arrives.");
    const famBox = h('<div class="sx-fonts"></div>');
    const presets = h('<div class="sx-row sx-presets"><span class="sx-label">Preset</span></div>');
    fam.append(presets, famBox);

    function setFont(role, name) {
      S.loadFont(name);
      const info = S.fontInfo(name);
      S.commit((s) => { s.fonts[role] = { name, stack: info ? `"${name}", ${S.FALLBACK[info.cat]}` : name }; }, "font");
    }
    function drawFamilies() {
      famBox.textContent = "";
      for (const [role, label, use] of ROLES) {
        const f = S.state.fonts[role];
        if (!f) continue;
        const row = h(`<div class="sx-font"><label class="sx-label">${label} <small>${use}</small></label>
          <div class="sx-fsample sx-fsample-${role}">${esc(SAMPLE[role])}</div></div>`);
        const sel = fontSelect(role, f.name);
        sel.setAttribute("aria-label", label + " font");
        sel.addEventListener("change", () => setFont(role, sel.value));
        row.querySelector("label").after(sel);
        famBox.append(row);
      }
    }

    // ---- Size and spacing

    const size = card("Size and spacing", "Size sets the rem of the page; the gap separates blocks, the padding sits inside cells and boxes.");
    const sizeBox = h('<div class="sx-ranges"></div>');
    const sizeSeg = h('<div class="sx-row"><span class="sx-label">Base size</span><span class="sx-seg" role="group" aria-label="Base size"></span></div>');
    for (const [name, px] of Object.entries(S.SIZES)) {
      const b = h(`<button type="button" class="sx-btn" data-px="${px}" title="${px}px">${name[0].toUpperCase() + name.slice(1)}</button>`);
      b.addEventListener("click", () => { S.commit((s) => { s.type.text = { v: px, unit: "px" }; }, "preset"); });
      sizeSeg.lastElementChild.append(b);
    }
    const syncSize = () => sizeSeg.querySelectorAll("button").forEach((b) => b.setAttribute("aria-pressed", +b.dataset.px === S.state.type.text?.v));
    size.append(sizeSeg, sizeBox);
    function drawRanges() {
      sizeSeg.hidden = !S.state.type.text;
      sizeBox.textContent = "";
      for (const [key, [label, min, max, step, unit]] of Object.entries(RANGES)) {
        const t = S.state.type[key];
        if (!t) continue;
        const digits = String(step).split(".")[1]?.length || 0;
        const r = h(`<label class="sx-range"><span>${label}</span><input type="range" min="${min}" max="${max}" step="${step}" value="${t.v}"><output>${t.v.toFixed(digits)}${unit}</output></label>`);
        const input = r.querySelector("input"), out = r.querySelector("output");
        input.addEventListener("input", () => {
          out.textContent = (+input.value).toFixed(digits) + unit;
          S.commit((s) => { s.type[key] = { v: +input.value, unit: s.type[key].unit }; }, "size");
        });
        sizeBox.append(r);
      }
      if (!sizeBox.children.length) sizeBox.append(h('<p class="sx-hint">This theme has no size tokens (<code>--text</code>, <code>--gap</code>, …); only the families apply.</p>'));
    }

    Object.entries(PRESETS).forEach(([name, preset]) => {
      const b = h(`<button type="button" class="sx-btn">${name}</button>`);
      b.addEventListener("click", () => {
        if (!preset) {
          const p = S.page;
          S.commit((s) => { s.fonts = JSON.parse(JSON.stringify(p.fonts)); s.type = JSON.parse(JSON.stringify(p.type)); }, "preset");
        } else {
          const [fonts, type] = preset;
          fonts.forEach((n) => S.loadFont(n));
          S.commit((s) => {
            ROLES.forEach(([role], i) => { if (s.fonts[role]) { const info = S.fontInfo(fonts[i]); s.fonts[role] = { name: fonts[i], stack: `"${fonts[i]}", ${S.FALLBACK[info.cat]}` }; } });
            for (const [k, v] of Object.entries(type)) if (s.type[k]) s.type[k] = { v, unit: s.type[k].unit };
          }, "preset");
        }
      });
      presets.append(b);
    });

    // ---- Specimen: the elements of the page with the classes of Asciidoctor, so the style level shows

    const spec = card("Specimen", "The page's own elements at the current tokens; the whole page follows.");
    spec.append(h(`<div class="sx-spec">
      <div class="sx-h sx-h1">Heading one</div><div class="sx-h sx-h2">Heading two</div>
      <div class="sx-h sx-h3">Heading three</div><div class="sx-h sx-h4">Heading four</div>
      <div class="paragraph"><p>Body text with <strong>bold</strong>, <em>italic</em>, <code>inline code</code> and
        <a href="#">a link</a>. Dense pages read well when the leading stays near 1.4 and the blocks sit close.</p></div>
      <div class="ulist"><ul><li><p>A list item</p></li><li><p>Another one, close to the first</p></li></ul></div>
      <table class="tableblock frame-all grid-all stretch"><thead><tr><th class="tableblock halign-left">Token</th><th class="tableblock halign-left">Value</th></tr></thead>
        <tbody><tr><td class="tableblock"><p class="tableblock"><code>--gap</code></p></td><td class="tableblock"><p class="tableblock">Space between blocks</p></td></tr>
        <tr><td class="tableblock"><p class="tableblock"><code>--pad</code></p></td><td class="tableblock"><p class="tableblock">Padding inside cells</p></td></tr></tbody></table>
      <div class="listingblock"><div class="content"><pre class="rouge highlight"><code><span class="c1">// Clears the target</span>
<span class="k">constexpr</span> <span class="n">std</span><span class="o">::</span><span class="kt">uint32_t</span> <span class="n">navy</span> <span class="o">=</span> <span class="mh">0x031926ff</span><span class="p">;</span>
<span class="k">auto</span> <span class="n">frame</span> <span class="o">=</span> <span class="n">renderer</span><span class="p">.</span><span class="n">begin</span><span class="p">(</span><span class="s">"main"</span><span class="p">);</span></code></pre></div></div>
    </div>`));

    grid.append(fam, size, spec);
    S.on((source) => {
      if (source === "reset" || source === "preset") { drawFamilies(); drawRanges(); }
      syncSize();
    });
    syncSize();
    drawFamilies();
    drawRanges();
  }

  document.querySelectorAll('.studio[data-studio="font"]').forEach(build);
})();
