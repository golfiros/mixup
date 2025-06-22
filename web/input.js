const update_gain = (gain) => {
  const start = 50 * (1 + Math.min(0, gain.value / 24));
  const end = 50 * (1 + Math.max(0, gain.value / 24));
  gain.style.background = `linear-gradient(to top,
    var(--color-bg) 0%,
    var(--color-bg) ${start}%,
    #aaa119 ${start}%,
    #aaa119 ${end}%,
    var(--color-bg) ${end}%,
    var(--color-bg) 100%)`;
  document.getElementById(`${gain.id}_val`).innerHTML = `${parseFloat(gain.value).toFixed(1)}dB`;
}

const update_balance = (balance) => {
  const start = 50 * (1 + Math.min(0, balance.value / 100));
  const end = 50 * (1 + Math.max(0, balance.value / 100));
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

const impl_input_set_port = (id, idx, path) => {
  const node_div = document.getElementById(`${id}_port_${idx}_node`);
  const path_div = document.getElementById(`${id}_port_${idx}_path`);
  const port = document.getElementById(path);
  if (path === "") {
    node_div.innerHTML = "no source";
    path_div.innerHTML = `select input ${idx ? "R" : "L"}`;
  } else {
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
const src_list = document.getElementById("input_grp");
const impl_input_new = (props) => {
  const list_elem = document.createElement("li");
  list_elem.id = `${props.id}_elem`;

  const list_label = document.createElement("button");
  list_elem.appendChild(list_label);
  list_label.classList.add("vertical-menu-item");
  list_label.onclick = () => {
    localStorage.setItem("input", props.id);
    refresh();
  };

  const delete_button = document.createElement("button");
  list_elem.appendChild(delete_button);
  delete_button.classList.add("delete-btn");
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
  input.name = "# input";
  input.classList.add("content-inner");

  const label = document.createElement("div");
  input.appendChild(label);
  label.classList.add("content-title");

  const label_observer = new MutationObserver(() => {
    if (!document.contains(input))
      label_observer.disconnect();
    else {
      const idx = [...input_list.childNodes].indexOf(list_elem);
      const name = input.name.replace("#", idx + 1);
      list_label.innerHTML = label.innerHTML = name;
    }
  });
  label_observer.observe(input_list, { childList: true });

  input_list.appendChild(list_elem);

  const src = document.createElement("option");
  src_list.appendChild(src);

  src.value = props.id;
  const src_observer = new MutationObserver(() => {
    if (!document.contains(input))
      src_observer.disconnect();
    else
      src.innerHTML = label.innerHTML;
  });
  src_observer.observe(label, { childList: true, subtree: true });

  const menu = document.createElement("div");
  input.appendChild(menu);
  menu.classList.add("input");

  const menu_left = document.createElement("div");
  menu.appendChild(menu_left);
  menu_left.classList.add("input-left");

  const ports = document.createElement("div");
  menu_left.appendChild(ports);
  ports.classList.add("input-ports");

  for (var i = 0; i < 2; i++) {
    const idx = i;

    const port_button = document.createElement("button")
    ports.appendChild(port_button);
    port_button.classList.add("input-port-select");
    port_button.innerHTML =
      `<div id="${props.id}_port_${idx}_node"></div>` +
      `<div id="${props.id}_port_${idx}_path"></div>`;

    const port_floater = floater_new(`${props.id}_port_${idx}`);
    ports.appendChild(port_floater.frame);
    port_floater.content.classList.add("input-port-list");
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

  const gain_div = document.createElement("div");
  menu.appendChild(gain_div);
  gain_div.classList.add("input-gain");

  const gain_ruler = document.createElement("div");
  gain_div.appendChild(gain_ruler);
  gain_ruler.classList.add("input-gain-ruler");

  for (var i = 0; i < 9; i++)
    gain_ruler.appendChild(document.createElement("div"));

  const gain_labels = document.createElement("div");
  gain_div.appendChild(gain_labels);
  gain_labels.classList.add("input-gain-labels");
  gain_labels.innerHTML = "<div>gain</div>";

  const gain_val = document.createElement("div");
  gain_labels.appendChild(gain_val);
  gain_val.id = `${props.id}_gain_val`;

  const gain = document.createElement("input");
  gain_div.appendChild(gain);
  gain.classList.add("input-gain-slider");

  gain.id = `${props.id}_gain`;
  gain.type = "range";
  gain.step = 0.1;
  gain.min = -24;
  gain.max = 24;
  gain.value = props.gain;
  update_gain(gain);
  gain.oninput = gain.onchange = () => {
    if (Math.abs(gain.value) < 0.5)
      gain.value = 0;
    rpc.input_set_gain(props.id, Number(gain.value))
      .then((res) => {
        if (res !== null)
          gain.value = res;
      });
    update_gain(gain);
  }

  const balance_div = document.createElement("div");
  menu_left.appendChild(balance_div);
  balance_div.classList.add("input-balance");

  const balance_ruler = document.createElement("div");
  balance_div.appendChild(balance_ruler);
  balance_ruler.classList.add("input-balance-ruler");

  for (var i = 0; i < 9; i++)
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
    rpc.input_set_balance(props.id, Number(balance.value))
      .then((res) => {
        if (res !== null)
          balance.value = res;
      });
    update_balance(balance);
  }
};

const mixer_new = document.getElementById("input_new");
mixer_new.onclick = () => rpc.input_new().then(impl_input_new);

rpc.register("input_new", impl_input_new);
rpc.register("input_delete", (id) => {
  const elem = document.getElementById(`${id}_elem`);
  elem.remove();
  const input = document.getElementById(id);
  input.remove();
  refresh();
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
