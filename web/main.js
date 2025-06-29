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
  var x, x0, y, y0, id = null;
  var drag = false, swipe = false, floating = false;
  var item = null;
  list.ontouchstart = (ev) => {
    if (!ev.target.classList.contains("vertical-menu-item") || id !== null)
      return;

    x = x0 = ev.changedTouches[0].clientX;
    y = y0 = ev.changedTouches[0].clientY;
    id = ev.changedTouches[0].identifier;

    drag = true;
    swipe = false;
    floating = true;

    item = ev.target;
    setTimeout(() => {
      if (!floating)
        return;

      drag = false;
      item = item.parentNode;
      const coords = item.getBoundingClientRect();

      var idx = [...list.childNodes].indexOf(item);
      document.body.appendChild(item);

      var under = idx < list.childNodes.length ? list.childNodes[idx] : list.lastChild;
      if (idx < list.childNodes.length)
        under.style.borderTop = "2px solid white";
      else
        under.style.borderBottom = "2px solid white";

      const list_scroll = () => {
        const coords = list.getBoundingClientRect();
        const r = Math.min(Math.max((y - coords.y) / coords.height, 0), 1);
        if (r < 0.15)
          list.scrollBy(0, -(1 + 10 * (0.15 - r) / 0.15), { behavior: "smooth" });
        if (r > 0.85)
          list.scrollBy(0, 1 + 10 * (r - 0.85) / 0.15, { behavior: "smooth" });
        if (floating)
          setTimeout(list_scroll, 10)
      }
      list_scroll();

      item.style.position = "fixed";
      item.style.height = `${coords.height}px`;
      item.style.width = `${coords.width}px`;
      item.style.left = `${coords.x}px`;
      item.style.top = `${coords.y}px`;
      item.style.zIndex = 500;
      item.style.opacity = "70%";
      const onmove = (ev) => {
        ev.preventDefault();

        const touch = [...ev.changedTouches].find((t) => t.identifier === id);
        if (touch === undefined)
          return;
        x = touch.clientX;
        y = touch.clientY;

        const dx = x - x0, dy = y - y0;

        item.style.transform = `translate(${dx}px, ${dy}px)`;

        const under_new = document.elementsFromPoint(x, y).find((node) => !item.contains(node));
        if (under_new.classList.contains("vertical-menu-item")) {
          under.style = "";
          under = under_new;
          idx = [...list.childNodes].indexOf(under.parentNode);
          const coords = under.getBoundingClientRect();
          if (idx < list.childNodes.length && y - coords.y > coords.height / 2) {
            under.style.borderBottom = "2px solid white";
            idx++;
          }
          else
            under.style.borderTop = "2px solid white";
        }
      }
      item.addEventListener("touchmove", onmove, { passive: false });
      item.ontouchend = (ev) => {
        const touch = [...ev.changedTouches].find((t) => t.identifier === id);
        if (touch === undefined)
          return;
        id = null;
        list.insertBefore(item, list.childNodes[idx]);
        item.dispatchEvent(new CustomEvent("itemdrop", { detail: { idx: idx } }));
        floating = false;
        under.style = "";
        item.style = "";
        item.removeEventListener("touchmove", onmove, { passive: false });
        item.ontouchend = null;
        item = null;
      };
    }, 600);
  }
  list.addEventListener("touchmove", (ev) => {
    if (!drag)
      return;

    const touch = [...ev.changedTouches].find((t) => t.identifier === id);
    if (touch === undefined)
      return;

    floating = false;

    x = touch.clientX;
    y = touch.clientY;

    const dx = x - x0, dy = y - y0;

    if (!swipe) {
      if (Math.abs(dx) > Math.abs(dy) && Math.abs(dx) > 5)
        swipe = true;
      else if (Math.abs(dy) > Math.abs(dx)) {
        drag = false;
        return;
      }
    }

    if (swipe) {
      ev.preventDefault();
      item.style.transform = `translateX(${dx > 0 ? Math.min(dx, 0.4 * list.offsetWidth) : 0}px)`;
    }
  }, { passive: false });
  list.ontouchend = (ev) => {
    if (!item)
      return;

    const touch = [...ev.changedTouches].find((t) => t.identifier === id);
    if (touch === undefined)
      return;

    floating = false;

    id = null;
    drag = false;

    const dx = x - x0;

    if (swipe && dx > 0.2 * list.offsetWidth)
      item.style.transform = `translateX(${0.4 * list.offsetWidth}px)`;
    else
      item.style.transform = `translateX(0)`;

    item = null;
    swipe = false;
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
