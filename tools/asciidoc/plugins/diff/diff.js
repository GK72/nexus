/*
 * Part of Nexus Library
 *
 * The toolbar of `tools/doc.sh --diff` (plugin `diff.rb`): the number of changes, the previous and
 * next change, marks and deletions on or off, and the changed pages of the book. The marks are the
 * elements with `data-nx-change`; the counts and the commit are attributes of <body>. Keys: `n` and
 * `p` (or `]` and `[`) jump, `m` toggles the marks, `x` the deletions. The toggles are remembered
 * per viewer.
 */
(() => {
  const root = document.documentElement;
  const data = document.body.dataset;
  const store = {
    get(key, fallback) {
      try { const v = localStorage.getItem(key); return v === null ? fallback : v === "1"; } catch { return fallback; }
    },
    set(key, on) {
      try { localStorage.setItem(key, on ? "1" : "0"); } catch { /* no storage: not remembered */ }
    },
  };
  let marks = store.get("nx-diff-marks", true);
  let deleted = store.get("nx-diff-deleted", true);

  const el = (tag, props = {}, ...kids) => {
    const node = Object.assign(document.createElement(tag), props);
    node.append(...kids);
    return node;
  };
  const changes = () => [...document.querySelectorAll("[data-nx-change]")]
    .filter((node) => deleted || node.dataset.nxChange !== "deleted");

  const added = +data.nxDiffAdded || 0, changed = +data.nxDiffChanged || 0, removed = +data.nxDiffDeleted || 0;
  const total = added + changed + removed;
  const isNew = data.nxDiffNewPage === "true";
  const ref = data.nxDiffRef || "HEAD";

  const bar = el("div", { className: "nx-diff-bar", role: "toolbar", ariaLabel: `Changes since ${ref}` });
  const count = el("span", { className: "nx-diff-count" });
  if (isNew) {
    count.append("New page ", el("small", {}, `since ${ref}`));
  } else {
    count.append(`${total} change${total === 1 ? "" : "s"} `, el("small", {}, `since ${ref}`));
  }
  bar.append(count);
  if (!isNew && total > 0) {
    const kinds = el("span", { className: "nx-diff-kinds" });
    for (const [n, kind] of [[added, "added"], [changed, "changed"], [removed, "deleted"]]) {
      if (n) kinds.append(el("span", { className: `k-${kind}`, title: kind }, String(n)));
    }
    bar.append(kinds);
  }

  const button = (label, title, onClick) => {
    const b = el("button", { type: "button", title }, label);
    b.addEventListener("click", onClick);
    return b;
  };
  const prev = button("↑", "Previous change (p)", () => jump(-1));
  const next = button("↓", "Next change (n)", () => jump(1));
  const marksBtn = button("Marks", "Show the changes (m)", () => { marks = !marks; store.set("nx-diff-marks", marks); apply(); });
  const delBtn = button("Deleted", "Show what was removed (x)", () => { deleted = !deleted; store.set("nx-diff-deleted", deleted); apply(); });
  if (!isNew && total > 0) bar.append(el("span", { className: "nx-diff-sep" }), prev, next);
  bar.append(el("span", { className: "nx-diff-sep" }), marksBtn);
  if (removed > 0 || document.querySelector("del.nx-del")) bar.append(delBtn);

  const pages = (data.nxDiffPages || "").split(" ").filter(Boolean);
  if (pages.length) {
    const here = location.pathname.split("/").pop().replace(/\.html$/, "") || "index";
    const list = el("ul");
    for (const name of pages) {
      const link = el("a", { href: `${name}.html` }, name === "index" ? "Whole book" : name);
      if (name === here) link.setAttribute("aria-current", "page");
      list.append(el("li", {}, link));
    }
    bar.append(el("details", {}, el("summary", { title: "The pages that changed" }, `Pages (${pages.length})`), list));
  }

  // The next or previous change: from the one jumped to last while it is in view (also when the
  // end of the page keeps it from the top), else from the top of the window, where
  // `scroll-margin` puts a change after a jump
  const line = () => parseFloat(getComputedStyle(root).fontSize) * 5;
  let current = null;
  const jump = (dir) => {
    const list = changes();
    if (!marks || !list.length) return;
    const i = current ? list.indexOf(current) : -1;
    const y = line();
    let target;
    const box = i >= 0 ? current.getBoundingClientRect() : null;
    if (box && box.bottom > 0 && box.top < innerHeight) {
      target = list[(i + dir + list.length) % list.length];
    } else if (dir > 0) {
      target = list.find((node) => node.getBoundingClientRect().top > y + 4) || list[0];
    } else {
      target = list.findLast((node) => node.getBoundingClientRect().top < y - 4) || list[list.length - 1];
    }
    current?.classList.remove("nx-diff-current");
    current = target;
    current.classList.add("nx-diff-current");
    current.scrollIntoView({ block: "start", behavior: "smooth" });
  };

  const apply = () => {
    root.classList.toggle("nx-diff-off", !marks);
    root.classList.toggle("nx-diff-no-deleted", !deleted);
    marksBtn.setAttribute("aria-pressed", String(marks));
    delBtn.setAttribute("aria-pressed", String(deleted));
    delBtn.disabled = !marks;
    prev.disabled = next.disabled = !marks || !changes().length;
    if (!marks) current?.classList.remove("nx-diff-current");
  };

  addEventListener("keydown", (event) => {
    if (event.ctrlKey || event.metaKey || event.altKey) return;
    if (event.target.closest?.("input, textarea, select, [contenteditable]")) return;
    const key = { n: () => jump(1), "]": () => jump(1), p: () => jump(-1), "[": () => jump(-1),
      m: () => marksBtn.click(), x: () => delBtn.click() }[event.key];
    if (key) { event.preventDefault(); key(); }
  });

  apply();
  document.body.append(bar);
})();
