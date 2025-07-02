const GAIN_MIN = -24, GAIN_MAX = 24;
const EQ_REF_FREQ = 48000;
const EQ_MIN_FREQ = 20, EQ_MAX_FREQ = 20000;
const EQ_MIN_Q = 0.05, EQ_MAX_Q = 10;
const EQ_MIN_GAIN = -18, EQ_MAX_GAIN = 18;

const update_gain = (gain) => {
  const fraction =
    (parseFloat(gain.value) - parseFloat(gain.min)) /
    (parseFloat(gain.max) - parseFloat(gain.min));
  const start = 100 * Math.min(0.5, fraction);
  const end = 100 * Math.max(0.5, fraction);
  gain.style.background = `linear-gradient(to top,
    var(--color-bg) 0%,
    var(--color-bg) ${start}%,
    #aaa119 ${start}%,
    #aaa119 ${end}%,
    var(--color-bg) ${end}%,
    var(--color-bg) 100%)`;
  document.getElementById(`${gain.id}_val`).innerHTML = `${parseFloat(gain.value).toFixed(1)}dB`;
  document.getElementById(`${gain.id}_indicator`).style.bottom = `calc(100% * ${fraction} - 1.7vh)`;
}

const update_balance = (balance) => {
  const fraction =
    (parseFloat(balance.value) - parseFloat(balance.min)) /
    (parseFloat(balance.max) - parseFloat(balance.min));
  const start = 100 * Math.min(0.5, fraction);
  const end = 100 * Math.max(0.5, fraction);
  balance.style.background = `linear-gradient(to right,
    var(--color-bg) 0%,
    var(--color-bg) ${start}%,
    #46a3b9 ${start}%,
    #46a3b9 ${end}%,
    var(--color-bg) ${end}%,
    var(--color-bg) 100%)`;
  document.getElementById(`${balance.id}_val`).innerHTML =
    `${balance.value}${balance.value > 0 ? "R" : balance.value < 0 ? "L" : "C"}`;
}

const spline_interp = (x, y) => {
  const n = x.length - 1;

  const dx = [...Array(n)].map((_, i) => x[i + 1] - x[i]);
  const dy = [...Array(n)].map((_, i) => y[i + 1] - y[i]);

  const a = [...Array(n)].map((_, i) => 1 / dx[i - 1]);
  const b = [...Array(n)].map((_, i) => 2 * (1 / dx[i - 1] + 1 / dx[i]));
  const c = [...Array(n)].map((_, i) => 1 / dx[i]);
  const d = [...Array(n)].map((_, i) => 3 * (dy[i - 1] / (dx[i - 1] * dx[i - 1]) + dy[i] / (dx[i] * dx[i])));

  b[0] = 2 / dx[0], c[0] = 1 / dx[0], d[0] = 3 * dy[0] / (dx[0] * dx[0]);
  a[n] = 1 / dx[n - 1], b[n] = 2 / dx[n - 1], d[n] = 3 * dy[n - 1] / (dx[n - 1] * dx[n - 1]);

  const e = [c[0] / b[0]];
  d[0] = d[0] / b[0];

  for (let i = 1; i < n + 1; i++) {
    if (i < n)
      e[i] = c[i] / (b[i] - a[i] * e[i - 1])
    d[i] = (d[i] - a[i] * d[i - 1]) / (b[i] - a[i] * e[i - 1]);
  }
  for (let i = n - 1; i >= 0; i--)
    d[i] -= e[i] * d[i + 1];

  return {
    a: [...Array(n)].map((_, i) => y[i] + d[i] * dx[i] / 3),
    b: [...Array(n)].map((_, i) => y[i + 1] - d[i + 1] * dx[i] / 3),
  };
}

const update_eq = (eq) => {
  const all_freqs = [EQ_MIN_FREQ, EQ_MAX_FREQ];
  const funcs = [];
  [...eq.childNodes]
    .filter((node) => node instanceof HTMLDivElement)
    .sort((a, b) => a.id > b.id)
    .forEach((dot, idx) => {
      // from the Audio EQ cookbook https://www.w3.org/TR/audio-eq-cookbook/
      const f0 = parseFloat(dot.getAttribute("freq"));
      dot.style.left = `calc(${100 * Math.log(f0 / EQ_MIN_FREQ) / Math.log(EQ_MAX_FREQ / EQ_MIN_FREQ)}% - var(--input-eq-dot-radius))`;
      const w0 = 2 * Math.PI * f0 / 48000;
      const Q = parseFloat(dot.getAttribute("quality"));
      const g = parseFloat(dot.getAttribute("gain"));
      const A = Math.pow(10, g / 40);
      if (idx === 0 || idx === eq.childNodes.length - 2)
        dot.style.top = `calc(${100 * Math.log(EQ_MAX_Q / Q) / Math.log(EQ_MAX_Q / EQ_MIN_Q)}% - var(--input-eq-dot-radius))`;
      else
        dot.style.top = `calc(${100 * (EQ_MAX_GAIN - g) / (EQ_MAX_GAIN - EQ_MIN_GAIN)}% - var(--input-eq-dot-radius))`;
      const alpha = Math.sin(w0) / (2 * Q), cc = Math.cos(w0);

      const delta = Math.asinh(1 / (2 * Q)) * Math.sin(w0) * Math.log(10) / (w0 * Math.log(2));
      all_freqs.push(f0 * Math.pow(2, -delta));
      all_freqs.push(f0 * Math.pow(2, -0.5 * delta));
      all_freqs.push(f0);
      all_freqs.push(f0 * Math.pow(2, 0.5 * delta));
      all_freqs.push(f0 * Math.pow(2, delta));

      const a = [1 + alpha / A, -2 * cc, 1 - alpha / A];
      const b =
        idx === 0 ?
          [0.5 * (1 + cc), -(1 + cc), 0.5 * (1 + cc)]
          : idx === eq.childNodes.length - 2 ?
            [0.5 * (1 - cc), (1 - cc), 0.5 * (1 - cc)]
            : [1 + alpha * A, -2 * cc, 1 - alpha * A];
      funcs.push((x) => {
        const w = 2 * Math.PI * x / 48000;
        return 10 * (Math.log(b[0] * b[0] + b[1] * b[1] + b[2] * b[2] +
          2 * b[0] * b[2] * Math.cos(2 * w) +
          2 * (b[0] * b[1] + b[1] * b[2]) * Math.cos(w)) -
          Math.log(a[0] * a[0] + a[1] * a[1] + a[2] * a[2] +
            2 * a[0] * a[2] * Math.cos(2 * w) +
            2 * (a[0] * a[1] + a[1] * a[2]) * Math.cos(w))) / Math.log(10);
      });
    });

  const freqs = [];
  all_freqs.filter((f) => f >= EQ_MIN_FREQ && f <= EQ_MAX_FREQ).sort((a, b) => a > b).forEach((f) => {
    if (freqs.length === 0 || f / freqs[freqs.length - 1] - 1 >= 0.0001)
      freqs.push(f);
  });

  const x = freqs.map((f) => Math.log(f / EQ_MIN_FREQ) / Math.log(EQ_MAX_FREQ / EQ_MIN_FREQ));

  const gains = funcs.map((f) => freqs.map((x) => f(x))).reduce((t, v) => t.map((y, i) => y + v[i]));
  const y = gains.map((g) => (EQ_MAX_GAIN - g) / (EQ_MAX_GAIN - EQ_MIN_GAIN));

  const cx = { a: x.map((_, i) => (2 * x[i] + x[i + 1]) / 3), b: x.map((_, i) => (x[i] + 2 * x[i + 1]) / 3) };
  const cy = spline_interp(x, y);

  document.getElementById(`${eq.id}_plot`).setAttributeNS(null, "d",
    `M ${x[0]} ${y[0]} ` + x.slice(1).reduce((s, _, i) =>
      s + ` C ${cx.a[i]} ${cy.a[i]} ${cx.b[i]} ${cy.b[i]} ${x[i + 1]} ${y[i + 1]}`, ""));
}

const impl_input_set_port = (id, idx, path) => {
  const node_div = document.getElementById(`${id}_port_${idx}_node`);
  const path_div = document.getElementById(`${id}_port_${idx}_path`);
  if (path === "") {
    node_div.innerHTML = "no source";
    path_div.innerHTML = `select input ${idx ? "R" : "L"}`;
  } else {
    const port = document.getElementById(path);
    if (port === null) {
      node_div.innerHTML = "unknown source";
      node_div.innerHTML = "unknown input";
    } else {
      node_div.innerHTML = port.parentNode.title;
      path_div.innerHTML = port.innerHTML;
    }
  }
  [...document.getElementById(`${id}_port_${idx}`).childNodes]
    .filter((node) => node instanceof HTMLButtonElement)
    .forEach((node) => node.classList.remove("active"));
  document.getElementById(`${id}_port_${idx}_${path}`).classList.add("active");
}

const input_ports = document.getElementById("input_ports");
const update_port = (id, idx) => {
  const port = document.getElementById(`${id}_port_${idx}`);
  port.innerHTML = "";

  const button = document.createElement("button");
  port.appendChild(button);
  button.id = `${id}_port_${idx}_`;
  button.innerHTML = "no source";
  button.onclick = () => {
    rpc.input_set_port(id, idx, "");
    impl_input_set_port(id, idx, "");
    history.back();
  }

  [...input_ports.childNodes].forEach((group) => {
    const node = document.createElement("div");
    port.appendChild(node);
    node.innerHTML = group.title;

    [...group.childNodes].forEach((elem, i) => {
      const path = elem.id, button = document.createElement("button");
      port.appendChild(button);
      button.id = `${id}_port_${idx}_${path}`;
      button.innerHTML = `input ${i + 1}`;
      button.onclick = () => {
        rpc.input_set_port(id, idx, path);
        impl_input_set_port(id, idx, path);
        history.back();
      };
    })
  });
}

const input_list = document.getElementById("input_list");
const impl_input_new = (props) => {
  const list_elem = document.createElement("li");
  list_elem.id = `${props.id}_elem`;
  list_elem.addEventListener("itemdrop", (ev) => {
    rpc.input_set_index(props.id, ev.detail.idx)
    list_elem.scrollIntoView();
  });
  list_elem.name = props.id;

  const list_label = document.createElement("button");
  list_elem.appendChild(list_label);
  list_label.classList.add("vertical-menu-item");
  list_label.onclick = () => {
    localStorage.setItem("input", props.id);
    refresh();
  };

  const delete_button = document.createElement("button");
  list_elem.appendChild(delete_button);
  delete_button.classList.add("vertical-menu-delete");
  delete_button.innerHTML = "delete";
  delete_button.onclick = () => {
    rpc.input_delete(props.id);
    list_elem.remove();
    input.remove();
    refresh();
  };

  const input = document.createElement("div");
  document.getElementById("content").appendChild(input);
  input.hidden = true;
  input.id = props.id;
  input.classList.add("content");

  const name = document.createElement("input");
  name.id = `${props.id}_name`;
  name.type = "text";
  name.value = props.name;
  name.classList.add("content-title");

  const label = document.createElement("div");
  input.appendChild(label);
  label.id = `${props.id}_label`;
  label.classList.add("content-title");
  label.onclick = () => {
    label.replaceWith(name)
    name.focus();
  };

  name.onchange = name.onblur = () => {
    name.replaceWith(label);
    const idx = [...input_list.childNodes].indexOf(list_elem);
    list_label.innerHTML = label.innerHTML = idx >= 0 ? name.value.replace("#", idx + 1) : name.value;
    rpc.input_set_name(props.id, name.value);
  }

  const label_observer = new MutationObserver(() => {
    if (!document.contains(input))
      label_observer.disconnect();
    else if (list_elem.style.position === "fixed")
      list_label.innerHTML = label.innerHTML = name.value;
    else
      list_label.innerHTML = label.innerHTML = name.value.replace("#", [...input_list.childNodes].indexOf(list_elem) + 1);
  });
  label_observer.observe(input_list, { childList: true });

  input_list.appendChild(list_elem);

  const menu = document.createElement("div");
  input.appendChild(menu);
  menu.classList.add("content-inner");
  menu.classList.add("input");

  const ports = document.createElement("div");
  menu.appendChild(ports);
  ports.classList.add("ports");

  for (let i = 0; i < 2; i++) {
    const idx = i;

    const port_button = document.createElement("button")
    ports.appendChild(port_button);
    port_button.classList.add("port-select");
    port_button.innerHTML =
      `<div id="${props.id}_port_${idx}_node"></div>` +
      `<div id="${props.id}_port_${idx}_path"></div>`;

    const port_floater = floater_new(`${props.id}_port_${idx}`);
    ports.appendChild(port_floater.frame);
    port_floater.content.classList.add("port-list");
    port_button.onclick = port_floater.show;
    port_floater.title = `select input ${idx ? "R" : "L"}`;
    update_port(props.id, idx);
    impl_input_set_port(props.id, idx, props.port[idx]);

    const port_observer = new MutationObserver(() => {
      if (!document.contains(input))
        port_observer.disconnect();
      else
        update_port(props.id, idx);
    });
    port_observer.observe(input_ports, { childList: true });
  }

  const menu_div = document.createElement("div");
  menu.appendChild(menu_div);
  menu_div.classList.add("input-controls");

  const menu_left = document.createElement("div");
  menu_div.appendChild(menu_left);
  menu_left.classList.add("input-left");

  const gain_div = document.createElement("div");
  menu_div.appendChild(gain_div);
  gain_div.classList.add("input-gain");

  const gain_ruler = document.createElement("div");
  gain_div.appendChild(gain_ruler);
  gain_ruler.classList.add("input-gain-ruler");
  for (let i = 0; i < 9; i++)
    gain_ruler.appendChild(document.createElement("div"));

  const gain_indicator = document.createElement("div");
  gain_div.appendChild(gain_indicator);
  gain_indicator.id = `${props.id}_gain_indicator`;
  gain_indicator.classList.add("input-gain-indicator");

  const gain_labels = document.createElement("div");
  gain_div.appendChild(gain_labels);
  gain_labels.classList.add("input-gain-labels");
  gain_labels.innerHTML = "<div>gain</div>";

  const gain_val = document.createElement("div");
  gain_labels.insertBefore(gain_val, gain_labels.firstChild);
  gain_val.id = `${props.id}_gain_val`;

  const gain = document.createElement("input");
  gain_div.appendChild(gain);
  gain.classList.add("input-gain-slider");

  gain.id = `${props.id}_gain`;
  gain.type = "range";
  gain.step = 0.1;
  gain.min = GAIN_MIN;
  gain.max = GAIN_MAX;
  gain.value = props.gain;
  update_gain(gain);
  gain.oninput = gain.onchange = () => {
    rpc.input_set_gain(props.id, parseFloat(gain.value))
    update_gain(gain);
  }

  const eq = document.createElement("div");
  menu_left.appendChild(eq);
  eq.id = `${props.id}_eq`;
  eq.classList.add("input-eq");

  const eq_svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  eq.appendChild(eq_svg);
  eq_svg.setAttributeNS(null, "height", "100%");
  eq_svg.setAttributeNS(null, "width", "100%");
  eq_svg.setAttributeNS(null, "viewBox", "0 0 1 1");
  eq_svg.setAttributeNS(null, "preserveAspectRatio", "none");
  eq_svg.style.pointerEvents = "none";

  for (let i = 0; i < 7; i++) {
    const y = i / 6;
    const dash = document.createElementNS("http://www.w3.org/2000/svg", "line")
    eq_svg.appendChild(dash);
    dash.setAttributeNS(null, "x1", "0");
    dash.setAttributeNS(null, "y1", y);
    dash.setAttributeNS(null, "x2", "1");
    dash.setAttributeNS(null, "y2", y);
    dash.setAttributeNS(null, "stroke", "var(--color-hl2)");
    dash.setAttributeNS(null, "stroke-width", "1");
    dash.setAttributeNS(null, "stroke-dasharray", "10 5");
    dash.setAttributeNS(null, "vector-effect", "non-scaling-stroke");
  }

  for (let t = 0; t < 32; t++) {
    const i = Math.floor(t / 10), j = t - 10 * i;
    const x = Math.log((j + 1) / 2) / Math.log(1000) + i / 3;
    const tick = document.createElementNS("http://www.w3.org/2000/svg", "line");
    eq_svg.appendChild(tick);
    tick.setAttributeNS(null, "x1", x);
    tick.setAttributeNS(null, "y1", "0");
    tick.setAttributeNS(null, "x2", x);
    tick.setAttributeNS(null, "y2", "1");
    tick.setAttributeNS(null, "stroke", "var(--color-hl2)");
    tick.setAttributeNS(null, "stroke-width", "1");
    tick.setAttributeNS(null, "vector-effect", "non-scaling-stroke");
  }

  const eq_plot = document.createElementNS("http://www.w3.org/2000/svg", "path");
  eq_svg.appendChild(eq_plot);
  eq_plot.id = `${props.id}_eq_plot`;
  eq_plot.setAttributeNS(null, "fill", "none");
  eq_plot.setAttributeNS(null, "stroke", "white");
  eq_plot.setAttributeNS(null, "stroke-width", "2");
  eq_plot.setAttributeNS(null, "vector-effect", "non-scaling-stroke");

  const eq_dots = props.eq.map((params, stage) => {
    const dot = document.createElement("div");
    eq.appendChild(dot);
    dot.id = `${props.id}_eq_${stage}`;
    dot.classList.add("input-eq-dot");
    dot.classList.add(`band-${stage}`);
    dot.setAttribute("freq", params.freq);
    dot.setAttribute("quality", params.quality);
    dot.setAttribute("gain", params.gain);
    return dot;
  });
  update_eq(eq);

  var x0, y0, y1, id = [null, null], dot = -1;
  eq.ontouchstart = (ev) => {
    if (id[0] === null) {
      id[0] = ev.changedTouches[0].identifier;
      const x = ev.changedTouches[0].clientX, y = ev.changedTouches[0].clientY;
      const dist = eq_dots.map((d) => {
        const coords = d.getBoundingClientRect();
        const xc = coords.x + coords.width / 2, yc = coords.y + coords.height / 2;
        return ((xc - x) * (xc - x) + (yc - y) * (yc - y)) / (coords.width * coords.width);
      });
      var min = Math.min(...dist);
      if (min <= 1.5 * 1.5) {
        dot = dist.indexOf(min);
        eq_dots[dot].classList.add("active");
        const coords = eq_dots[dot].getBoundingClientRect();
        const xc = coords.x + coords.width / 2, yc = coords.y + coords.height / 2;
        x0 = eq.getBoundingClientRect().x + x - xc;
        y0 = eq.getBoundingClientRect().y + y - yc;
      }
    }
    if (id[1] === null) {
      var touch = null;
      if (id[0] === ev.changedTouches[0].identifier) {
        if (ev.changedTouches.length > 1)
          id[1] = (touch = ev.changedTouches[1]).identifier;
      } else
        id[1] = (touch = ev.changedTouches[0]).identifier;
      if (touch === null)
        return;
      if (dot !== -1) {
        const quality = eq_dots[dot].getAttribute("quality");
        y1 = touch.clientY - eq.getBoundingClientRect().height * Math.log(EQ_MAX_Q / quality) / Math.log(EQ_MAX_Q / EQ_MIN_Q);
      }
    }
  }
  eq.ontouchmove = (ev) => {
    const touches = id.map((i) => [...ev.changedTouches].find((t) => t.identifier === i));
    if (touches[0] !== undefined && dot !== -1) {
      const dx = Math.max(0, Math.min(1, (touches[0].clientX - x0) / eq.getBoundingClientRect().width));
      const dy = Math.max(0, Math.min(1, (touches[0].clientY - y0) / eq.getBoundingClientRect().height));
      if ([0, eq_dots.length - 1].includes(dot)) {
        const quality = Math.exp(Math.log(EQ_MAX_Q) - Math.log(EQ_MAX_Q / EQ_MIN_Q) * dy);
        eq_dots[dot].setAttribute("quality", quality);
        rpc.input_set_eq_quality(props.id, dot, quality);
      }
      else {
        const gain = EQ_MAX_GAIN + (EQ_MIN_GAIN - EQ_MAX_GAIN) * dy;
        eq_dots[dot].setAttribute("gain", gain);
        rpc.input_set_eq_gain(props.id, dot, gain);
      }
      const freq = Math.exp(Math.log(EQ_MIN_FREQ) + Math.log(EQ_MAX_FREQ / EQ_MIN_FREQ) * dx);
      eq_dots[dot].setAttribute("freq", freq);
      rpc.input_set_eq_freq(props.id, dot, freq);
    }
    if (touches[1] !== undefined && dot !== -1 && ![0, eq_dots.length - 1].includes(dot)) {
      const dy = Math.max(0, Math.min(1, (touches[1].clientY - y1) / eq.getBoundingClientRect().height));
      const quality = Math.exp(Math.log(EQ_MAX_Q) - Math.log(EQ_MAX_Q / EQ_MIN_Q) * dy);
      eq_dots[dot].setAttribute("quality", quality);
      rpc.input_set_eq_quality(props.id, dot, quality);
    }
    update_eq(eq);
  }
  eq.ontouchend = (ev) => {
    const touches = id.map((i) => [...ev.changedTouches].find((t) => t.identifier === i));
    if (touches[0] !== undefined) {
      id[0] = null;
      if (dot >= 0) {
        eq_dots[dot].classList.remove("active");
        dot = -1;
      }
    }
    if (touches[1] !== undefined)
      id[1] = null;
  }

  const balance_div = document.createElement("div");
  menu_left.appendChild(balance_div);
  balance_div.classList.add("input-balance");

  const balance_ruler = document.createElement("div");
  balance_div.appendChild(balance_ruler);
  balance_ruler.classList.add("input-balance-ruler");

  for (let i = 0; i < 9; i++)
    balance_ruler.appendChild(document.createElement("div"));

  const balance_labels = document.createElement("div");
  balance_div.appendChild(balance_labels);
  balance_labels.classList.add("input-balance-labels");
  balance_labels.innerHTML = "<div>balance</div>";

  const balance_val = document.createElement("div");
  balance_labels.appendChild(balance_val);
  balance_val.id = `${props.id}_balance_val`;

  const balance = document.createElement("input");
  balance_div.appendChild(balance);
  balance.classList.add("input-balance-slider");

  balance.id = `${props.id}_balance`;
  balance.type = "range";
  balance.min = -100;
  balance.max = 100;
  balance.value = props.balance;
  update_balance(balance);
  balance.oninput = balance.onchange = () => {
    if (Math.abs(balance.value) < 5)
      balance.value = 0;
    rpc.input_set_balance(props.id, parseFloat(balance.value))
    update_balance(balance);
  }

  return list_label;
};

const mixer_new = document.getElementById("input_new");
mixer_new.onclick = () => rpc.input_new().then(impl_input_new).then((label) => {
  label.scrollIntoView();
  label.dispatchEvent(new Event("click"));
});

rpc.register("input_new", impl_input_new);
rpc.register("input_delete", (id) => {
  const elem = document.getElementById(`${id}_elem`);
  elem.remove();
  const input = document.getElementById(id);
  input.remove();
  refresh();
});
rpc.register("input_set_index", (id, idx) => {
  const elem = document.getElementById(`${id}_elem`);
  input_list.appendChild(elem);
  input_list.insertBefore(elem, input_list.childNodes[idx]);
});
rpc.register("input_set_name", (id, name) => {
  const label = document.getElementById(`${id}_label`);
  label.dispatchEvent(new Event("click"));
  const elem = document.getElementById(`${id}_name`);
  elem.value = name;
  elem.dispatchEvent(new Event("blur"));
});
rpc.register("input_set_port", impl_input_set_port);
rpc.register("input_set_gain", (id, val) => {
  const gain = document.getElementById(`${id}_gain`);
  gain.value = val;
  update_gain(gain);
});
rpc.register("input_set_balance", (id, val) => {
  const balance = document.getElementById(`${id}_balance`);
  balance.value = val;
  update_balance(balance);
});
["freq", "quality", "gain"].forEach((param) =>
  rpc.register(`input_set_eq_${param}`, (id, stage, val) => {
    const dot = document.getElementById(`${id}_eq_${stage}`);
    dot.setAttribute(param, val);
    update_eq(document.getElementById(`${id}_eq`));
  })
);
