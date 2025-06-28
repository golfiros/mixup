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
      var state = history.state;

      if (state === null)
        state = { floater_stack: [] };
      if (state.floater_stack === undefined)
        state.floater_stack = [];

      frame.style.zIndex = 1000 + 10 * state.floater_stack.length;
      frame.classList.add("active");

      state.floater_new = `${id}_frame`;
      history.replaceState(state, "");

      state.floater_stack.push(state.floater_new);
      delete state.floater_new;
      history.pushState(state, "");
    }
  });
  return floater;
}
const floater_init = window.floater_init = (state) => state?.floater_stack?.forEach((id, idx) => {
  const frame = document.getElementById(id);
  frame.style.zIndex = 1000 + 10 * idx;
  frame.classList.add("active");
});
window.addEventListener("popstate", (ev) => {
  floater_init(ev.state);
  if (ev.state !== null && ev.state.floater_new !== undefined)
    document.getElementById(ev.state.floater_new).classList.remove("active");
})
