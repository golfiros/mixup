const fader_max = 6, fader_min = -60, fader_offset = 0.1, fader_slope = 0.15;
const fader_curve = (x) =>
  (fader_max - fader_min) *
  (Math.pow(x + fader_offset, fader_slope) - Math.pow(fader_offset, fader_slope)) /
  (Math.pow(1 + fader_offset, fader_slope) - Math.pow(fader_offset, fader_slope))
  + fader_min;
const fader_curve_inv = (y) =>
  fader_offset * (Math.pow(
    (Math.pow(1 + 1 / fader_offset, fader_slope) - 1) *
    (y - fader_min) / (fader_max - fader_min) + 1,
    1 / fader_slope
  ) - 1);

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
const update_gain = (gain) => {
  gain.style.background = `linear-gradient(to top,
    #12a119 0%,
    #12a119 ${gain.value}%,
    var(--color-bg) ${gain.value}%,
    var(--color-bg) 100%)`;
  document.getElementById(`${gain.id}_val`).innerHTML =
    `${gain.value > 0 ? fader_curve(parseFloat(gain.value) / 100).toFixed(1) : "-Ꝏ "}dB`;
  document.getElementById(`${gain.id}_indicator`).style.bottom = `calc(${gain.value}% - 1.7vh)`;
}

const impl_channel_set_src = (id, obj_id) => {
  const type_div = document.getElementById(`${id}_src_type`);
  const obj_div = document.getElementById(`${id}_src_obj`);
  const port = document.getElementById(obj_id);
  if (port === null) {
    type_div.innerHTML = "no source";
    obj_div.innerHTML = "select source";
  } else {
    type_div.innerHTML = [...port.querySelector(".content-inner").classList]
      .filter((c) => c !== "content-inner")[0];
    obj_div.innerHTML = port.querySelector(".content-title").innerHTML;
  }
  [...document.getElementById(`${id}_src`).childNodes]
    .filter((node) => node instanceof HTMLButtonElement)
    .forEach((node) => node.classList.remove("active"));
  document.getElementById(`${id}_src_${obj_id}`)?.classList.add("active");
}

const input_list = document.getElementById("input_list");
const update_src = (id) => {
  const src = document.getElementById(`${id}_src`);
  const active = src.querySelector("button.active")?.name;
  src.innerHTML = "";

  const button = document.createElement("button");
  src.appendChild(button);
  button.id = `${id}_src_00000000-0000-0000-0000-000000000000`;
  button.name = "00000000-0000-0000-0000-000000000000";
  button.innerHTML = "no source";
  button.onclick = () => {
    rpc.channel_set_src(id, "00000000-0000-0000-0000-000000000000");
    impl_channel_set_src(id, "00000000-0000-0000-0000-000000000000");
    history.back();
  }

  const inputs = document.createElement("div");
  src.appendChild(inputs);
  inputs.innerHTML = "inputs";

  [...input_list.childNodes].forEach((elem) => {
    const obj = elem.name, button = document.createElement("button");
    src.appendChild(button);
    button.id = `${id}_src_${obj}`;
    button.name = obj;
    button.innerHTML = elem.childNodes[0].innerHTML;
    button.onclick = () => {
      rpc.channel_set_src(id, obj);
      impl_channel_set_src(id, obj);
      history.back();
    };
  })
  if (active !== undefined)
    impl_channel_set_src(id, active);
}

const impl_channel_new = (id, props) => {
  const channels = document.getElementById(`${id}_channels`);

  const channel_div = document.createElement("div");
  channel_div.id = props.id;
  channel_div.classList.add("mixer-channel-div");
  channel_div.addEventListener("itemdrop", (ev) => rpc.channel_set_index(props.id, ev.detail.idx));

  const channel = document.createElement("div");
  channel_div.appendChild(channel);
  channel.classList.add("mixer-channel");

  const gain_div = document.createElement("div");
  channel.appendChild(gain_div);
  gain_div.classList.add("mixer-channel-vol");

  const gain_ruler = document.createElement("div");
  gain_div.appendChild(gain_ruler);
  gain_ruler.classList.add("mixer-channel-ruler");
  for (var i = 0; i < 12; i++) {
    const tick = document.createElement("div");
    gain_ruler.appendChild(tick);
    tick.style.bottom = `calc(${100 * fader_curve_inv((fader_max - fader_min) * (i / 11) + fader_min)}% - 2px)`;
  }

  const gain_indicator = document.createElement("div");
  gain_div.appendChild(gain_indicator);
  gain_indicator.id = `${props.id}_gain_indicator`;
  gain_indicator.classList.add("mixer-channel-indicator");

  const gain_val = document.createElement("div");
  gain_div.appendChild(gain_val);
  gain_val.id = `${props.id}_gain_val`;
  gain_val.classList.add("mixer-channel-value");

  const gain = document.createElement("input");
  gain_div.appendChild(gain);
  gain.classList.add("mixer-channel-slider");

  gain.id = `${props.id}_gain`;
  gain.type = "range";
  gain.step = 0.1;
  gain.value = 100 * fader_curve_inv(props.gain);
  gain.oninput = gain.onchange = () => {
    rpc.channel_set_gain(props.id, fader_curve(parseFloat(gain.value) / 100))
      .then((res) => {
        if (res !== null)
          gain.value = 100 * fader_curve_inv(res);
      });
    update_gain(gain);
  }

  const mute = document.createElement("button");
  channel.appendChild(mute);
  mute.id = `${props.id}_mute`;
  mute.classList.add("mixer-channel-mute");
  mute.innerHTML = "M";
  mute.onclick = () => {
    mute.classList.toggle("active");
    const mute_val = mute.classList.contains("active");
    rpc.channel_set_mute(props.id, mute_val);
  }
  if (props.mute)
    mute.classList.add("active");

  const name = document.createElement("input");
  name.id = `${props.id}_name`;
  name.type = "text";
  name.value = props.name;
  name.classList.add("content-title");

  const label = document.createElement("button");
  channel.appendChild(label);
  label.classList.add("mixer-channel-label");
  label.innerHTML = channel.name;

  const menu = floater_new(`${props.id}_menu`);
  channel_div.appendChild(menu.frame);
  menu.title.id = `${props.id}_label`;
  menu.title.onclick = () => {
    menu.title.replaceWith(name);
    name.focus();
  }

  name.onchange = name.onblur = () => {
    name.replaceWith(menu.title);
    const idx = [...channels.childNodes].indexOf(channel_div);
    menu.title = label.innerHTML = idx >= 0 ? name.value.replace("#", idx + 1) : name.value;
    rpc.channel_set_name(props.id, name.value);
  }

  label.onclick = menu.show;

  const label_observer = new MutationObserver(() => {
    if (!document.contains(channel_div))
      label_observer.disconnect();
    else {
      const idx = [...channels.childNodes].indexOf(channel_div);
      menu.title = label.innerHTML = idx >= 0 ? name.value.replace("#", idx + 1) : name.value;
    }
  });
  label_observer.observe(channels, { childList: true });
  channels.appendChild(channel_div);

  update_gain(gain);

  const delete_button = document.createElement("button");
  channel_div.appendChild(delete_button);
  delete_button.classList.add("mixer-channel-delete");
  delete_button.innerHTML = "delete";
  delete_button.onclick = () => {
    rpc.channel_delete(props.id);
    channel_div.remove();
  };

  const src_div = document.createElement("div");
  menu.content.appendChild(src_div);
  src_div.classList.add("channel-src");

  const src_button = document.createElement("button")
  src_div.appendChild(src_button);
  src_button.classList.add("channel-src-select");
  src_button.innerHTML =
    `<div id="${props.id}_src_type"></div>` +
    `<div id="${props.id}_src_obj"></div>`;

  const src = floater_new(`${props.id}_src`);
  src_div.appendChild(src.frame);
  src.content.classList.add("port-list");
  src_button.onclick = src.show;
  src.title = `select source`;
  update_src(props.id);
  impl_channel_set_src(props.id, props.src);

  const src_observer = new MutationObserver(() => {
    if (!document.contains(src.frame))
      src_observer.disconnect();
    else
      update_src(props.id);
  });
  src_observer.observe(input_list, { childList: true, subtree: true });

  const balance_div = document.createElement("div");
  menu.content.appendChild(balance_div);
  balance_div.classList.add("channel-balance");

  const balance_ruler = document.createElement("div");
  balance_div.appendChild(balance_ruler);
  balance_ruler.classList.add("channel-balance-ruler");

  for (var i = 0; i < 9; i++)
    balance_ruler.appendChild(document.createElement("div"));

  const balance_labels = document.createElement("div");
  balance_div.appendChild(balance_labels);
  balance_labels.classList.add("channel-balance-labels");
  balance_labels.innerHTML = "<div>balance</div>";

  const balance_val = document.createElement("div");
  balance_labels.appendChild(balance_val);
  balance_val.id = `${props.id}_balance_val`;

  const balance = document.createElement("input");
  balance_div.appendChild(balance);
  balance.classList.add("channel-balance-slider");

  balance.id = `${props.id}_balance`;
  balance.type = "range";
  balance.min = -100;
  balance.max = 100;
  balance.value = props.balance;
  update_balance(balance);
  balance.oninput = balance.onchange = () => {
    if (Math.abs(balance.value) < 5)
      balance.value = 0;
    rpc.channel_set_balance(props.id, parseFloat(balance.value))
      .then((res) => {
        if (res !== null)
          balance.value = res;
      });
    update_balance(balance);
  }
};

const impl_mixer_set_port = (id, idx, path) => {
  const node_div = document.getElementById(`${id}_port_${idx}_node`);
  const path_div = document.getElementById(`${id}_port_${idx}_path`);
  if (path === "") {
    node_div.innerHTML = "no sink";
    path_div.innerHTML = `select output ${idx ? "R" : "L"}`;
  } else {
    const port = document.getElementById(path);
    if (port === null) {
      node_div.innerHTML = "unknown sink";
      node_div.innerHTML = "unknown output";
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

const output_list = document.getElementById("output_ports");
const update_port = (id, idx) => {
  const port = document.getElementById(`${id}_port_${idx}`);
  port.innerHTML = "";

  const button = document.createElement("button");
  port.appendChild(button);
  button.id = `${id}_port_${idx}_`;
  button.innerHTML = "no sink";
  button.onclick = () => {
    rpc.mixer_set_port(id, idx, "");
    impl_mixer_set_port(id, idx, "");
    history.back();
  }

  [...output_list.childNodes].forEach((group) => {
    const node = document.createElement("div");
    port.appendChild(node);
    node.innerHTML = group.title;

    [...group.childNodes].forEach((elem, i) => {
      const path = elem.id, button = document.createElement("button");
      port.appendChild(button);
      button.id = `${id}_port_${idx}_${path}`;
      button.innerHTML = `output ${i + 1}`;
      button.onclick = () => {
        rpc.mixer_set_port(id, idx, path);
        impl_mixer_set_port(id, idx, path);
        history.back();
      };
    })
  });
}

const mixer_list = document.getElementById("mixer_list");
const impl_mixer_new = (props) => {
  const list_elem = document.createElement("li");
  list_elem.id = `${props.id}_elem`;
  list_elem.addEventListener("itemdrop", (ev) => rpc.mixer_set_index(props.id, ev.detail.idx));
  list_elem.name = props.id;

  const list_label = document.createElement("button");
  list_elem.appendChild(list_label);
  list_label.classList.add("vertical-menu-item");
  list_label.onclick = () => {
    localStorage.setItem("mixer", props.id);
    refresh();
  };

  const delete_button = document.createElement("button");
  list_elem.appendChild(delete_button);
  delete_button.classList.add("vertical-menu-delete");
  delete_button.innerHTML = "delete";
  delete_button.onclick = () => {
    rpc.mixer_delete(props.id);
    list_elem.remove();
    mixer.remove();
    refresh();
  };

  const mixer = document.createElement("div");
  document.getElementById("content").appendChild(mixer);
  mixer.hidden = true;
  mixer.id = props.id;
  mixer.classList.add("content");

  const name = document.createElement("input");
  name.id = `${props.id}_name`;
  name.type = "text";
  name.value = props.name;
  name.classList.add("content-title");

  const label = document.createElement("div");
  mixer.appendChild(label);
  label.id = `${props.id}_label`;
  label.classList.add("content-title");
  label.onclick = () => {
    label.replaceWith(name)
    name.focus();
  };

  name.onchange = name.onblur = () => {
    name.replaceWith(label);
    const idx = [...mixer_list.childNodes].indexOf(list_elem);
    list_label.innerHTML = label.innerHTML = idx >= 0 ? name.value.replace("#", idx + 1) : name.value;
    rpc.mixer_set_name(props.id, name.value);
  }

  const label_observer = new MutationObserver(() => {
    if (!document.contains(mixer))
      label_observer.disconnect();
    else {
      const idx = [...mixer_list.childNodes].indexOf(list_elem);
      list_label.innerHTML = label.innerHTML = idx >= 0 ? name.value.replace("#", idx + 1) : name.value;
    }
  });
  label_observer.observe(mixer_list, { childList: true });

  mixer_list.appendChild(list_elem);

  const menu = document.createElement("div");
  mixer.appendChild(menu);
  menu.classList.add("content-inner");
  menu.classList.add("mixer");

  const ports = document.createElement("div");
  menu.appendChild(ports);
  ports.classList.add("ports");

  for (var i = 0; i < 2; i++) {
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
    port_floater.title = `select output ${idx ? "R" : "L"}`;
    update_port(props.id, idx);
    impl_mixer_set_port(props.id, idx, props.port[idx]);

    const port_observer = new MutationObserver(() => {
      if (!document.contains(mixer))
        port_observer.disconnect();
      else
        update_port(props.id, idx);
    });
    port_observer.observe(output_list, { childList: true });
  }

  const mixer_console = document.createElement("div");
  menu.appendChild(mixer_console);
  mixer_console.classList.add("mixer-console");

  const channels = document.createElement("div");
  mixer_console.appendChild(channels);
  channels.id = `${props.id}_channels`;
  channels.classList.add("mixer-channels");
  var x, x0, y, y0, id = null;
  var drag = false, swipe = false, floating = false;
  var item = null;
  channels.ontouchstart = (ev) => {
    if (!ev.target.classList.contains("mixer-channel-label") &&
      !ev.target.classList.contains("mixer-channel-mute") || id !== null)
      return;

    x = x0 = ev.changedTouches[0].clientX;
    y = y0 = ev.changedTouches[0].clientY;
    id = ev.changedTouches[0].identifier;

    drag = true;
    swipe = false;
    floating = true;

    item = ev.target.parentNode;
    if (channels.childNodes.length > 1)
      setTimeout(() => {
        if (!floating)
          return;

        drag = false;
        item = item.parentNode;
        const coords = item.getBoundingClientRect();

        var idx = [...channels.childNodes].indexOf(item);
        document.body.appendChild(item);

        var under = idx < channels.childNodes.length ? channels.childNodes[idx] : channels.lastChild;
        if (idx < channels.childNodes.length)
          under.style.borderLeft = "2px solid white";
        else
          under.style.borderRight = "2px solid white";

        const list_scroll = () => {
          const coords = channels.getBoundingClientRect();
          const r = Math.min(Math.max((x - coords.x) / coords.width, 0), 1);
          if (r < 0.15)
            channels.scrollBy(-(1 + 10 * (0.15 - r) / 0.15), { behavior: "smooth" }, 0);
          if (r > 0.85)
            channels.scrollBy(1 + 10 * (r - 0.85) / 0.15, { behavior: "smooth" }, 0);
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

          const under_new = document.elementsFromPoint(x, y)
            .find((node) => node !== item && node.classList.contains("mixer-channel-div"));
          if (under_new !== undefined) {
            under.style = "";
            under = under_new;
            idx = [...channels.childNodes].indexOf(under);
            const coords = under.getBoundingClientRect();
            if (idx < channels.childNodes.length && x - coords.x > coords.width / 2) {
              under.style.borderRight = "2px solid white";
              idx++;
            }
            else
              under.style.borderLeft = "2px solid white";
          }
        }
        item.addEventListener("touchmove", onmove, { passive: false });
        item.ontouchend = (ev) => {
          const touch = [...ev.changedTouches].find((t) => t.identifier === id);
          if (touch === undefined)
            return;
          id = null;
          channels.insertBefore(item, channels.childNodes[idx]);
          item.dispatchEvent(new CustomEvent("itemdrop", { detail: { idx: idx } }));
          floating = false;
          under.style = "";
          item.style = "";
          item.removeEventListener("touchmove", onmove, { passive: false });
          item.ontouchend = null;
          item = null;
        };
      }, 600);
  };
  channels.addEventListener("touchmove", (ev) => {
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
      if (Math.abs(dy) > Math.abs(dx) && Math.abs(dy) > 5)
        swipe = true;
      else if (Math.abs(dx) > Math.abs(dy)) {
        drag = false;
        return;
      }
    }

    if (swipe) {
      ev.preventDefault();
      item.style.transform = `translateY(${dy < 0 ? Math.max(dy, -0.25 * channels.offsetHeight) : 0}px)`;
    }
  }, { passive: false });
  channels.ontouchend = (ev) => {
    if (!item)
      return;

    const touch = [...ev.changedTouches].find((t) => t.identifier === id);
    if (touch === undefined)
      return;

    floating = false;

    id = null;
    drag = false;

    const dy = y - y0;

    if (swipe && dy < -0.125 * channels.offsetHeight)
      item.style.transform = `translateY(${-0.25 * channels.offsetHeight}px)`;
    else
      item.style.transform = `translateY(0)`;

    item = null;
    swipe = false;
  };

  const master_ch = document.createElement("div");
  mixer_console.appendChild(master_ch);
  master_ch.classList.add("mixer-master");

  const master_div = document.createElement("div");
  master_ch.appendChild(master_div);
  master_div.classList.add("mixer-channel-vol");

  const master_ruler = document.createElement("div");
  master_div.appendChild(master_ruler);
  master_ruler.classList.add("mixer-channel-ruler");
  for (var i = 0; i < 12; i++) {
    const tick = document.createElement("div");
    master_ruler.appendChild(tick);
    tick.style.bottom = `calc(${100 * fader_curve_inv((fader_max - fader_min) * (i / 11) + fader_min)}% - 2px)`;
  }

  const master_indicator = document.createElement("div");
  master_div.appendChild(master_indicator);
  master_indicator.id = `${props.id}_master_indicator`;
  master_indicator.classList.add("mixer-channel-indicator");

  const master_val = document.createElement("div");
  master_div.appendChild(master_val);
  master_val.id = `${props.id}_master_val`;
  master_val.classList.add("mixer-channel-value");

  const master_vol = document.createElement("input");
  master_div.appendChild(master_vol);
  master_vol.classList.add("mixer-channel-slider");

  master_vol.id = `${props.id}_master`;
  master_vol.type = "range";
  master_vol.step = 0.1;
  master_vol.value = 100 * fader_curve_inv(props.master);
  update_gain(master_vol);
  master_vol.oninput = master_vol.onchange = () => {
    rpc.mixer_set_master(props.id, fader_curve(parseFloat(master_vol.value) / 100))
      .then((res) => {
        if (res !== null)
          master_vol.value = 100 * fader_curve_inv(res);
      });
    update_gain(master_vol);
  }

  const channel_new = document.createElement("button");
  master_ch.appendChild(channel_new);
  channel_new.classList.add("mixer-channel-new");
  channel_new.innerHTML = "+";
  channel_new.onclick = () => rpc.channel_new(props.id).then((ch) => impl_channel_new(props.id, ch));

  const master_label = document.createElement("div");
  master_ch.appendChild(master_label);
  master_label.classList.add("mixer-channel-label");
  master_label.innerHTML = "master";

  props.channels.forEach((ch) => impl_channel_new(props.id, ch));
};

const mixer_new = document.getElementById("mixer_new");
mixer_new.onclick = () => rpc.mixer_new().then(impl_mixer_new);

rpc.register("mixer_new", impl_mixer_new);
rpc.register("mixer_delete", (id) => {
  const elem = document.getElementById(`${id}_elem`);
  elem.remove();
  const mixer = document.getElementById(id);
  mixer.remove();
  refresh();
});
rpc.register("mixer_set_index", (id, idx) => {
  const elem = document.getElementById(`${id}_elem`);
  mixer_list.appendChild(elem);
  mixer_list.insertBefore(elem, mixer_list.childNodes[idx]);
});
rpc.register("mixer_set_name", (id, name) => {
  const label = document.getElementById(`${id}_label`);
  label.dispatchEvent(new Event("click"));
  const elem = document.getElementById(`${id}_name`);
  elem.value = name;
  elem.dispatchEvent(new Event("blur"));
});
rpc.register("mixer_set_port", impl_mixer_set_port);
rpc.register("mixer_set_master", (id, val) => {
  const master = document.getElementById(`${id}_master`);
  master.value = 100 * fader_curve_inv(val);
  update_gain(master);
});

rpc.register("channel_new", impl_channel_new);
rpc.register("channel_delete", (id) => {
  const channel = document.getElementById(id);
  channel.remove();
});
rpc.register("channel_set_index", (id, idx) => {
  const elem = document.getElementById(`${id}`);
  elem.parentNode.appendChild(elem);
  elem.parentNode.insertBefore(elem, elem.parentNode.childNodes[idx]);
});
rpc.register("channel_set_name", (id, name) => {
  const label = document.getElementById(`${id}_label`);
  label.dispatchEvent(new Event("click"));
  const elem = document.getElementById(`${id}_name`);
  elem.value = name;
  elem.dispatchEvent(new Event("blur"));
});
rpc.register("channel_set_src", impl_channel_set_src);
rpc.register("channel_set_gain", (id, val) => {
  const gain = document.getElementById(`${id}_gain`);
  gain.value = 100 * fader_curve_inv(val);
  update_gain(gain);
});
rpc.register("channel_set_mute", (id, val) => {
  const mute = document.getElementById(`${id}_mute`);
  mute.classList[val ? "add" : "remove"]("active");
});
rpc.register("channel_set_balance", (id, val) => {
  const balance = document.getElementById(`${id}_balance`);
  balance.value = val;
  update_balance(balance);
});
