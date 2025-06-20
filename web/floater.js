const floater_new = window.floater_new = (id) => {
  const frame = document.createElement("div");
  frame.id = `${id}_frame`;
  frame.classList.add("floater-frame");

  const border = document.createElement("div");
  frame.appendChild(border);
  border.classList.add("floater-border");

  frame.onclick = (ev) => {
    if (!border.contains(ev.target))
      history.back();
  }

  const header = document.createElement("div");
  border.appendChild(header);
  header.classList.add("floater-header");

  const title = document.createElement("div");
  header.appendChild(title);

  const close = document.createElement("button");
  header.appendChild(close);
  close.innerHTML = "X";
  close.onclick = () => { history.back() };

  const content = document.createElement("div");
  border.appendChild(content);
  content.id = id;
  content.classList.add("floater-content");

  const floater = {};
  Object.defineProperty(floater, "frame", { value: frame });
  Object.defineProperty(floater, "content", { value: content });
  Object.defineProperty(floater, "title", { set: (text) => title.innerHTML = text });
  Object.defineProperty(floater, "show", {
    value: () => {
      frame.classList.add("active");
      var state = history.state;
      if (state === null)
        state = {};
      state.floater_hide = `${id}_frame`;
      history.replaceState(state, "");
      history.pushState({ floater_show: `${id}_frame` }, "");
    }
  });
  return floater;
}
const floater_history = window.floater_history = (state) => {
  if (state.floater_hide !== undefined)
    document.getElementById(state.floater_hide).classList.remove("active");
  if (state.floater_show !== undefined)
    document.getElementById(state.floater_show).classList.add("active");
}
window.addEventListener("popstate", (ev) => {
  if (ev.state !== null)
    floater_history(ev.state)
})
