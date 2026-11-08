// Part of Nexus Library
//
// Column charts in the browser for the interactive blocks (`models.js`): stacked columns, lines
// over them, a legend, a hover tooltip and a table view. Hover or arrow keys (with focus) pick a
// column; the tooltip lists every series there, then the extra rows.

window.AdocChart = (() => {
  const MINUS = '−';

  const esc = (text) =>
    String(text).replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' })[c]);
  const num = (v) => String(Math.round(v * 10) / 10);
  const group = (digits) => {
    const [int, frac] = digits.split('.');
    return int.replace(/\B(?=(\d{3})+(?!\d))/g, ',') + (frac ? `.${frac}` : '');
  };
  const trim = (v, digits) => {
    let text = Math.abs(v).toFixed(digits);
    if (text.includes('.')) text = text.replace(/\.?0+$/, '');
    return text;
  };

  // Number formats: `tick` for axis labels, `value` for tooltips, `full` for tables. `money` is
  // whole, compact from 10,000 (12.5K); a currency sign goes before the number, a code (HUF) after
  // it. `plain` keeps up to two decimals.
  class Format {
    constructor(kind = 'plain', { unit = '' } = {}) {
      this.kind = kind;
      this.unitText = unit;
    }

    unit(text) {
      if (!this.unitText) return text;
      return /^[A-Za-z]+$/.test(this.unitText) ? `${text} ${this.unitText}` : this.unitText + text;
    }

    sign(v, text) {
      return v < 0 && /[1-9]/.test(text) ? MINUS + text : text;
    }

    money(v, compact) {
      const abs = Math.abs(v);
      let text;
      if (Math.round(abs) === 0) text = '0';
      else if (compact && abs >= 1e6) text = `${trim(abs / 1e6, 2)}M`;
      else if (compact) text = `${trim(abs / 1e3, abs >= 1e5 ? 0 : 1)}K`;
      else text = group(String(Math.round(abs)));
      return this.sign(v, this.unit(text));
    }

    tick(v, axisMax) {
      return this.kind === 'money' ? this.money(v, Math.abs(axisMax) >= 1e4) : this.full(v);
    }

    value(v) {
      if (v === null || v === undefined) return '—';
      return this.kind === 'money' ? this.money(v, Math.abs(v) >= 1e4) : this.full(v);
    }

    full(v) {
      if (v === null || v === undefined) return '—';
      if (this.kind !== 'money') return this.sign(v, group(trim(v, 2)));
      return this.sign(v, this.unit(group(String(Math.round(Math.abs(v))))));
    }
  }

  // Ticks at 1, 2, 2.5 or 5 times a power of ten, covering min..max.
  const ticks = (min, max, count = 5) => {
    if (max <= min) max = min + 1;
    const raw = (max - min) / count;
    const mag = 10 ** Math.floor(Math.log10(raw));
    const step = [1, 2, 2.5, 5, 10].find((s) => s * mag >= raw) * mag;
    const first = Math.floor(min / step) * step;
    const last = Math.ceil(max / step) * step;
    const out = [];
    for (let i = 0; i <= Math.round((last - first) / step); i++) out.push(+(first + i * step).toFixed(10));
    return out;
  };

  const sum = (values) => values.reduce((a, b) => a + b, 0);

  // [min, max] of what a chart draws: the stacked columns, the lines and zero.
  const extent = (spec) => {
    const values = spec.xs.flatMap((_, i) => {
      const vals = spec.series.map((s) => s.values[i] ?? 0);
      return [sum(vals.filter((v) => v > 0)), sum(vals.filter((v) => v < 0))];
    });
    values.push(...(spec.lines ?? []).flatMap((s) => s.values), 0);
    return [Math.min(...values), Math.max(...values)];
  };

  const WIDTH = 880;
  const HEIGHT = 320;
  const LEFT = 64;
  const RIGHT = WIDTH - 20;
  const TOP = 18;
  const BOTTOM = HEIGHT - 40;
  const GAP = 2;
  const BAR_MAX = 24;

  // The chart as HTML into `host`. Spec:
  //   xs        x values, one column each
  //   series    [{ name, values, slot }] stacked columns; `slot` 1..3 is the color
  //   lines     [{ name, values }] lines over the columns, in ink
  //   extras    [{ name, values }] more tooltip and table rows, not drawn
  //   x, y      { kind: 'plain' | 'money', unit }; xTitle, xLabels (tooltip titles), label
  //   yInclude  values the y axis covers too, e.g. the extent of other charts to compare with
  const render = (host, spec) => {
    const { xs, series } = spec;
    const lines = spec.lines ?? [];
    const extras = spec.extras ?? [];
    const slots = series.map((s, k) => s.slot ?? k + 1);
    const xf = new Format(spec.x?.kind, spec.x ?? {});
    const yf = new Format(spec.y?.kind, spec.y ?? {});

    const yt = ticks(...[extent(spec), spec.yInclude ?? []].flat().reduce(([lo, hi], v) => [Math.min(lo, v), Math.max(hi, v)], [Infinity, -Infinity]));
    const ymin = yt[0];
    const ymax = yt[yt.length - 1];
    const axisMax = Math.max(Math.abs(ymin), Math.abs(ymax));
    const y = (v) => BOTTOM - ((v - ymin) / (ymax - ymin)) * (BOTTOM - TOP);
    const band = (RIGHT - LEFT) / xs.length;
    const x = (i) => LEFT + (i + 0.5) * band;

    const parts = [];
    for (const t of yt) {
      parts.push(`<line class="${t === 0 ? 'axis' : 'grid'}" x1="${LEFT}" x2="${RIGHT}" y1="${num(y(t))}" y2="${num(y(t))}"/>`);
      parts.push(`<text class="tick" x="${LEFT - 8}" y="${num(y(t))}" text-anchor="end" dy="0.32em">${esc(yf.tick(t, axisMax))}</text>`);
    }

    // Stacked columns: a surface gap between segments, rounded at the outer end, square at zero.
    const w = Math.max(1, Math.min(BAR_MAX, band * 0.62));
    const gap = band >= 6 ? GAP : 0;
    xs.forEach((_, i) => {
      let pos = 0;
      let neg = 0;
      const vals = series.map((s) => s.values[i] ?? 0);
      const lastPos = vals.map((v) => v > 0).lastIndexOf(true);
      const lastNeg = vals.map((v) => v < 0).lastIndexOf(true);
      const segs = vals.map((v, k) => {
        if (v === 0) return '';
        const from = v > 0 ? pos : neg;
        const to = from + v;
        if (v > 0) pos = to;
        else neg = to;
        const up = to > from;
        let y0 = y(from);
        const y1 = y(to);
        if (from !== 0) y0 += up ? -gap : gap;
        const h = Math.abs(y0 - y1);
        if (h < 0.5) return '';
        const r = k === (v > 0 ? lastPos : lastNeg) ? Math.min(4, h, w / 2) : 0;
        const l = x(i) - w / 2;
        const rr = l + w;
        const ry = up ? y1 + r : y1 - r;
        const d = r
          ? `M${num(l)},${num(y0)}V${num(ry)}Q${num(l)},${num(y1)} ${num(l + r)},${num(y1)}H${num(rr - r)}Q${num(rr)},${num(y1)} ${num(rr)},${num(ry)}V${num(y0)}Z`
          : `M${num(l)},${num(y0)}V${num(y1)}H${num(rr)}V${num(y0)}Z`;
        return `<path class="bar f${slots[k]}" d="${d}"/>`;
      });
      parts.push(`<g class="col" data-i="${i}">${segs.join('')}</g>`);
    });

    for (const line of lines) {
      const d = line.values.map((v, i) => `${i === 0 ? 'M' : 'L'}${num(x(i))},${num(y(v))}`).join('');
      parts.push(`<path class="line ink" d="${d}"/>`);
    }

    // x ticks every round number of columns
    const step = [1, 2, 3, 6, 12, 24, 60, 120].find((s) => xs.length / s <= 13) ?? Math.ceil(xs.length / 12);
    xs.forEach((v, i) => {
      if (i % step === 0) parts.push(`<text class="tick" x="${num(x(i))}" y="${BOTTOM + 16}" text-anchor="middle">${esc(xf.tick(v, 0))}</text>`);
    });
    parts.push(`<text class="tick" x="${RIGHT}" y="${HEIGHT - 4}" text-anchor="end">${esc(spec.xTitle ?? '')}</text>`);

    lines.forEach(() => parts.push('<circle class="hover-dot ink" r="4.5" hidden/>'));
    parts.push(`<rect class="hit" x="${LEFT}" y="${TOP}" width="${RIGHT - LEFT}" height="${BOTTOM - TOP}"/>`);

    const rows = [...series, ...lines, ...extras];
    const data = {
      x: xs.map((_, i) => x(i)),
      labels: spec.xLabels ?? xs.map((v) => `${spec.xTitle ?? ''} ${xf.value(v)}`.trim()),
      names: rows.map((s) => s.name),
      classes: [...slots.map((slot) => `s${slot}`), ...lines.map(() => 'ink'), ...extras.map(() => 'extra')],
      values: xs.map((_, i) => rows.map((s) => yf.value(s.values[i]))),
      ys: xs.map((_, i) => lines.map((s) => y(s.values[i]))),
    };

    const keys = [
      ...series.map((s, k) => `<span class="chart-key"><span class="key-rect s${slots[k]}"></span>${esc(s.name)}</span>`),
      ...lines.map((s) => `<span class="chart-key"><span class="key-line ink"></span>${esc(s.name)}</span>`),
    ];
    const legend = keys.length < 2 ? '' : `<div class="chart-legend">${keys.join('')}</div>`;

    const head = [spec.xTitle ?? '', ...rows.map((s) => s.name)].map((h) => `<th class="tableblock">${esc(h)}</th>`).join('');
    const body = xs
      .map((v, i) => `<tr>${[xf.full(v), ...rows.map((s) => yf.full(s.values[i]))].map((c) => `<td class="tableblock">${esc(c)}</td>`).join('')}</tr>`)
      .join('');
    const open = host.querySelector('.chart-table')?.open ? ' open' : '';

    host.innerHTML = `${legend}
<div class="chart-plot">
<svg class="chart-svg" viewBox="0 0 ${WIDTH} ${HEIGHT}" role="img" tabindex="0" aria-label="${esc(spec.label ?? rows.map((s) => s.name).join(', '))}">
${parts.join('\n')}
</svg>
<div class="chart-tip" hidden></div>
</div>
<details class="chart-table"${open}><summary>Table</summary>
<table class="tableblock frame-all grid-all stretch"><thead><tr>${head}</tr></thead><tbody>${body}</tbody></table>
</details>`;
    hover(host.querySelector('svg.chart-svg'), data);
  };

  const el = (tag, cls, text) => {
    const node = document.createElement(tag);
    if (cls) node.className = cls;
    if (text !== undefined) node.textContent = text;
    return node;
  };

  // The hover layer: the column nearest the pointer lifts, the dots follow the lines, one tooltip.
  const hover = (svg, data) => {
    const plot = svg.parentElement;
    const tip = plot.querySelector('.chart-tip');
    const dots = [...svg.querySelectorAll('.hover-dot')];
    const cols = [...svg.querySelectorAll('.col')];
    let current = -1;

    const nearest = (vx) => data.x.reduce((best, x, i) => (Math.abs(x - vx) < Math.abs(data.x[best] - vx) ? i : best), 0);

    const show = (i) => {
      current = i;
      const x = data.x[i];
      // SVG elements have no `hidden` property, only the attribute
      dots.forEach((dot, k) => {
        dot.removeAttribute('hidden');
        dot.setAttribute('cx', x);
        dot.setAttribute('cy', data.ys[i][k]);
      });
      cols.forEach((col, k) => col.classList.toggle('on', k === i));

      tip.replaceChildren(el('div', 'tip-head', data.labels[i]));
      data.names.forEach((name, k) => {
        const row = el('div', `tip-row ${data.classes[k]}`);
        row.append(el('strong', '', data.values[i][k]), el('span', '', name));
        tip.append(row);
      });
      tip.hidden = false;

      // the tooltip beside the column, flipped near the right edge
      const box = svg.getBoundingClientRect();
      const px = (x * box.width) / WIDTH + box.left - plot.getBoundingClientRect().left;
      const left = px + 14 + tip.offsetWidth > plot.clientWidth ? px - 14 - tip.offsetWidth : px + 14;
      tip.style.transform = `translate(${Math.max(0, left)}px, 8px)`;
    };

    const hide = () => {
      current = -1;
      tip.hidden = true;
      dots.forEach((dot) => dot.setAttribute('hidden', ''));
      cols.forEach((col) => col.classList.remove('on'));
    };

    svg.addEventListener('pointermove', (event) => {
      const point = new DOMPoint(event.clientX, event.clientY).matrixTransform(svg.getScreenCTM().inverse());
      show(nearest(point.x));
    });
    svg.addEventListener('pointerleave', hide);
    svg.addEventListener('focus', () => show(current < 0 ? data.x.length - 1 : current));
    svg.addEventListener('blur', hide);
    svg.addEventListener('keydown', (event) => {
      const step = { ArrowLeft: -1, ArrowRight: 1 }[event.key];
      if (!step) return;
      event.preventDefault();
      show(Math.min(data.x.length - 1, Math.max(0, current + step)));
    });
  };

  return { Format, extent, render };
})();
