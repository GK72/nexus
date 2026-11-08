// Part of Nexus Library
//
// The interactive blocks of `interactive.rb`: panels of inputs over a chart (`chart.js`) that
// redraw on every change. A model turns the values of the inputs into the spec of the chart and
// a summary:
//
//   AdocInteractive.model('name', (params) => ({ chart: { xs, series, ... }, summary: [[label, value]] }));
//
// `params` has one entry per input (`-` in names becomes `_`; percents are fractions, checkboxes
// booleans) and the `unit` of the block. "Duplicate" copies a panel with its current values (A, B,
// ...); all panels of a block share the y scale. Models register in their own scripts, which come
// after this one; the blocks start once the page is loaded.

window.AdocInteractive = (() => {
  const models = {};
  const LETTERS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';

  const setup = (block) => {
    const model = models[block.dataset.model];
    if (!model) {
      block.textContent = `Unknown model: ${block.dataset.model}`;
      return;
    }
    const panels = () => [...block.querySelectorAll('.ix-panel')];

    const read = (panel) => {
      const params = { unit: block.dataset.unit ?? '' };
      for (const input of panel.querySelectorAll('input[name]')) {
        if (input.type === 'checkbox') {
          params[input.name] = input.checked;
          continue;
        }
        const value = Number(input.value);
        if (input.value === '' || !Number.isFinite(value)) return null;
        params[input.name.replace(/-/g, '_')] = input.dataset.kind === 'percent' ? value / 100 : value;
      }
      return params;
    };

    const showSummary = (panel, items) => {
      panel.querySelector('.ix-summary').replaceChildren(
        ...items.map(([label, value]) => {
          const item = document.createElement('div');
          const strong = document.createElement('strong');
          strong.textContent = value;
          const span = document.createElement('span');
          span.textContent = label;
          item.append(strong, span);
          return item;
        }),
      );
    };

    // All panels, on one y scale.
    let pending = false;
    const drawAll = () => {
      pending = false;
      const outs = panels().map((panel) => {
        const params = read(panel);
        panel.classList.toggle('ix-invalid', params === null);
        return params && model(params);
      });
      const extents = outs.filter(Boolean).map((out) => AdocChart.extent(out.chart));
      const span = [Math.min(...extents.map((e) => e[0])), Math.max(...extents.map((e) => e[1]))];
      panels().forEach((panel, k) => {
        const out = outs[k];
        if (!out) return;
        AdocChart.render(panel.querySelector('.ix-chart'), { ...out.chart, yInclude: span });
        showSummary(panel, out.summary);
      });
    };
    const schedule = () => {
      if (!pending) requestAnimationFrame(drawAll);
      pending = true;
    };

    const relabel = () => {
      const all = panels();
      all.forEach((panel, k) => {
        panel.querySelector('.ix-name').textContent = LETTERS[k] ?? String(k + 1);
        panel.querySelector('.ix-remove').hidden = all.length < 2;
      });
    };

    const wire = (panel) => {
      const form = panel.querySelector('.ix-inputs');
      form.addEventListener('input', schedule);
      form.addEventListener('submit', (event) => event.preventDefault());
      panel.querySelector('.ix-dup').addEventListener('click', () => {
        const copy = panel.cloneNode(true);
        // the clone keeps the value attributes, not what was typed since
        const sources = panel.querySelectorAll('input[name]');
        copy.querySelectorAll('input[name]').forEach((input, k) => {
          input.value = sources[k].value;
          input.checked = sources[k].checked;
        });
        copy.querySelector('.ix-chart').replaceChildren();
        panel.after(copy);
        wire(copy);
        relabel();
        drawAll();
        copy.querySelector('input[name]').focus();
      });
      panel.querySelector('.ix-remove').addEventListener('click', () => {
        if (panels().length < 2) return;
        panel.remove();
        relabel();
        drawAll();
      });
    };

    panels().forEach(wire);
    relabel();
    drawAll();
  };

  document.addEventListener('DOMContentLoaded', () => document.querySelectorAll('.ix[data-model]').forEach(setup));

  return {
    model: (name, fn) => {
      models[name] = fn;
    },
  };
})();
