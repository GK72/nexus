// The `line` model of `documenting.adoc`: y = a·x + b.

AdocInteractive.model('line', ({ a, b }) => {
  const xs = Array.from({ length: 21 }, (_, i) => i - 10);
  return {
    chart: {
      xs,
      xTitle: 'x',
      series: [],
      lines: [{ name: 'y', values: xs.map((x) => a * x + b) }],
    },
    summary: [['The equation', `y = ${a}·x ${b < 0 ? '−' : '+'} ${Math.abs(b)}`]],
  };
});
