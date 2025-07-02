const types = ["input", "mixer"];

const refresh = window.refresh = () => {
  [...document.getElementById("content").childNodes].forEach((node) => {
    node.hidden = true;
    document.getElementById(`${node.id}_elem`)?.childNodes[0].classList.remove("active");
  });

  var type = localStorage.getItem("selected");
  if (type === null || !types.includes(type))
    localStorage.setItem("selected", type = "input");

  types.forEach((type) => {
    document.getElementById(`${type}_nav`).classList.remove("active")
    document.getElementById(`${type}_menu`).hidden = true;
  });
  document.getElementById(`${type}_nav`).classList.add("active");
  document.getElementById(`${type}_menu`).hidden = false;

  const id = localStorage.getItem(type);
  var selected;
  if (id && (selected = document.getElementById(id))) {
    selected.hidden = false;
    document.getElementById(`${id}_elem`).childNodes[0].classList.add("active");
  }
  else {
    document.getElementById("content_empty").hidden = false;
    localStorage.setItem(type, "");
  }
}
types.forEach((type) => {
  document.getElementById(`${type}_nav`).onclick = () => {
    localStorage.setItem("selected", type);
    refresh();
  };
  const list = document.getElementById(`${type}_list`);
  const modes = Object.freeze({ SCROLL: 0, SWIPE: 1, FLOAT: 2 });
  var x0, y0, y, id = null, mode, item, under, idx, coords;
  list.ontouchstart = (ev) => {
    if (id !== null)
      return;

    const touch = ev.changedTouches[0];
    id = touch.identifier;
    x0 = touch.clientX, y = y0 = touch.clientY;

    if (!ev.target.classList.contains("vertical-menu-item"))
      return;

    mode = null;
    item = ev.target;

    setTimeout(() => {
      if (id === null || mode !== null || list.childNodes.length <= 1)
        return;

      mode = modes.FLOAT;
      item = item.parentNode;

      coords = item.getBoundingClientRect();
      item.style.position = "fixed";
      item.style.height = `${coords.height}px`;
      item.style.width = `${coords.width}px`;
      item.style.left = `${coords.x}px`;
      item.style.top = `${coords.y}px`;
      item.style.zIndex = 500;
      item.style.opacity = "70%";

      idx = [...list.childNodes].indexOf(item);
      list.appendChild(item);

      under = list.childNodes[idx < list.childNodes.length - 1 ? idx : idx - 1];
      if (idx < list.childNodes.length - 1)
        under.style.borderTop = "2px solid white";
      else
        under.style.borderBottom = "2px solid white";

      const y0 = list.getBoundingClientRect().y, h = list.getBoundingClientRect().height, f = 2 * coords.height / (3 * h);
      const list_scroll = () => {
        const r = Math.min(Math.max((y - y0) / h, 0), 1);
        if (r < f)
          list.scrollBy(0, -(1 + 10 * (f - r) / f), { behavior: "smooth" });
        if (r > 1 - f)
          list.scrollBy(0, 1 + 10 * (r - (1 - f)) / f, { behavior: "smooth" });
        if (mode === modes.FLOAT)
          setTimeout(list_scroll, 10)
      }
      list_scroll();
    }, 600);
  }
  list.addEventListener("touchmove", (ev) => {
    const touch = [...ev.changedTouches].find((t) => t.identifier === id);
    if (touch === undefined || item === null)
      return;

    const dx = touch.clientX - x0, dy = (y = touch.clientY) - y0;

    if (mode === null) {
      if (Math.abs(dy) > Math.abs(dx))
        mode = modes.SCROLL;
      else if (Math.abs(dx) > 5)
        mode = modes.SWIPE;
    }

    if (mode !== modes.SCROLL)
      ev.preventDefault();

    if (mode === modes.SWIPE)
      item.style.transform = `translateX(${dx > 0 ? Math.min(dx, 0.4 * list.offsetWidth) : 0}px)`;

    if (mode === modes.FLOAT) {
      item.style.transform = `translate(${dx}px, ${dy}px)`;

      const under_new = document.elementsFromPoint(touch.clientX, touch.clientY).find((node) => !item.contains(node));
      if (under_new.classList.contains("vertical-menu-item")) {
        under.style = "";
        under = under_new;
        idx = [...list.childNodes].indexOf(under.parentNode);
        const coords = under.getBoundingClientRect();
        if (touch.clientY - coords.y > coords.height / 2) {
          under.style.borderBottom = "2px solid white";
          idx++;
        }
        else
          under.style.borderTop = "2px solid white";
      }
    }
  }, { passive: false });
  list.ontouchend = (ev) => {
    const touch = [...ev.changedTouches].find((t) => t.identifier === id);
    if (touch === undefined)
      return;

    id = null;
    if (item === null)
      return;

    if (mode === modes.SWIPE) {
      if (touch.clientX - x0 > 0.2 * list.offsetWidth)
        item.style.transform = `translateX(${0.4 * list.offsetWidth}px)`;
      else
        item.style.transform = `translateX(0px)`;
    } else if (mode === modes.FLOAT) {
      list.insertBefore(item, list.childNodes[idx]);
      under.style = "";
      item.style = "";
      item.dispatchEvent(new CustomEvent("itemdrop", { detail: { idx: idx } }));
    }

    mode = null;
    item = null;
    id = null;
  }
});

rpc.register("done", () => {
  refresh();
  try {
    floater_init(history.state);
  } catch {
    history.replaceState(null, "");
  }
});

rpc.init();
