const src_list = document.getElementById("input_grp");

const input_list = document.getElementById("input_list");
const input_new = document.getElementById("input_new");

const input_ports = document.getElementById("input_ports");

const impl_input_new = (props) => {
  const input = document.createElement("div");
  input.id = props.id;
  input.name = "# input";

  const label = document.createElement("div");
  input.appendChild(label);

  const label_observer = new MutationObserver(() => {
    if (!document.contains(input))
      label_observer.disconnect();
    else {
      const idx = Array.from(input_list.childNodes).indexOf(input);
      const name = input.name.replace("#", idx + 1);
      label.innerHTML = name;
    }
  });
  label_observer.observe(input_list, { childList: true });

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

  input_list.appendChild(input);

  const delete_button = document.createElement("button");
  input.appendChild(delete_button);

  delete_button.innerHTML = "delete";
  delete_button.onclick = () => {
    if (!confirm("confirm delete"))
      return;
    rpc.input_delete(props.id);
    input.remove();
  };

  const ports = document.createElement("div");
  input.appendChild(ports);

  for (var i = 0; i < 2; i++) {
    const port = document.createElement("select");
    ports.appendChild(port);
    port.id = `${props.id}_port_${i}`;
    port.innerHTML = input_ports.innerHTML;
    const observer = new MutationObserver(() => {
      if (!document.contains(input))
        observer.disconnect();
      else
        port.innerHTML = input_ports.innerHTML;
    });
    observer.observe(input_ports, { childList: true, subtree: true });

    const idx = i;
    port.value = props.port[idx];
    port.onchange = () =>
      rpc.input_set_port(props.id, idx, port.value);
  }

  const gain = document.createElement("input");
  input.appendChild(gain);

  gain.id = `${props.id}_gain`;
  gain.type = "range";
  gain.min = -24;
  gain.max = 24;
  gain.value = props.gain;
  gain.oninput = gain.onchange = () =>
    rpc.input_set_gain(props.id, Number(gain.value))
      .then((res) => {
        if (res !== null)
          gain.value = res;
      });

  const balance = document.createElement("input");
  input.appendChild(balance);

  balance.id = `${props.id}_balance`;
  balance.type = "range";
  balance.min = -100;
  balance.max = 100;
  balance.value = props.gain;
  balance.oninput = balance.onchange = () =>
    rpc.input_set_gain(props.id, Number(balance.value))
      .then((res) => {
        if (res !== null)
          balance.value = res;
      });
};

input_new.onclick = () => rpc.input_new().then(impl_input_new);

rpc.register("input_new", impl_input_new);
rpc.register("input_delete", (id) => {
  const input = document.getElementById(id);
  input.remove();
});
rpc.register("input_set_port", (id, idx, path) => {
  const port = document.getElementById(`${id}_port_${idx}`);
  port.value = path;
});
rpc.register("input_set_gain", (id, val) => {
  const gain = document.getElementById(`${id}_gain`);
  gain.value = val;
});
rpc.register("input_set_balance", (id, val) => {
  const balance = document.getElementById(`${id}_balance`);
  balance.value = val;
});
