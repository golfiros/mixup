const update_vol = (vol) => {
  const end = 100 * (parseFloat(vol.value) + 60) / (6 + 60);
  vol.style.background = `linear-gradient(to top,
    #12a119 0%,
    #12a119 ${end}%,
    var(--color-bg) ${end}%,
    var(--color-bg) 100%)`;
  //document.getElementById(`${vol.id}_val`).innerHTML = `${parseFloat(vol.value).toFixed(1)}dB`;
}

const impl_channel_new = (id, props) => {
  const channels = document.getElementById(`${id}_channels`);

  const channel = document.createElement("div");
  channel.id = props.id;
  channel.name = "# channel";

  const label = document.createElement("div");
  channel.appendChild(label);

  const label_observer = new MutationObserver(() => {
    if (!document.contains(channel))
      label_observer.disconnect();
    else {
      const idx = Array.from(channels.childNodes).indexOf(channel);
      label.innerHTML = channel.name.replace("#", idx + 1);
    }
  });
  label_observer.observe(channels, { childList: true });

  channels.appendChild(channel);

  const delete_button = document.createElement("button");
  channel.appendChild(delete_button);

  delete_button.innerHTML = "delete";
  delete_button.onclick = () => {
    if (!confirm("confirm delete"))
      return;
    rpc.channel_delete(props.id);
    channel.remove();
  };

  const src = document.createElement("select");
  channel.appendChild(src);

  src.id = `${props.id}_src`;
  src.innerHTML = src_list.innerHTML;
  const src_observer = new MutationObserver(() => {
    if (!document.contains(channel))
      src_observer.disconnect();
    else {
      const val = src.value;
      src.innerHTML = src_list.innerHTML;
      src.value = val;
    }
  });
  src_observer.observe(src_list, { childList: true, subtree: true });
  src.value = props.src;

  src.onchange = () =>
    rpc.channel_set_src(props.id, src.value);

  const gain = document.createElement("input");
  channel.appendChild(gain);

  gain.id = `${props.id}_gain`;
  gain.type = "range";
  gain.min = -60;
  gain.max = 6;
  gain.value = props.gain;
  gain.oninput = gain.onchange = () =>
    rpc.channel_set_gain(props.id, Number(gain.value))
      .then((res) => {
        if (res !== null)
          gain.value = res;
      });

  const balance = document.createElement("input");
  channel.appendChild(balance);

  balance.id = `${props.id}_balance`;
  balance.type = "range";
  balance.min = -100;
  balance.max = 100;
  balance.value = props.balance;
  balance.oninput = balance.onchange = () =>
    rpc.channel_set_balance(props.id, Number(balance.value))
      .then((res) => {
        if (res !== null)
          balance.value = res;
      });
};

const impl_mixer_set_port = (id, idx, path) => {
  const node_div = document.getElementById(`${id}_port_${idx}_node`);
  const path_div = document.getElementById(`${id}_port_${idx}_path`);
  const port = document.getElementById(path);
  if (path === "") {
    node_div.innerHTML = "no sink";
    path_div.innerHTML = `select output ${idx ? "R" : "L"}`;
  } else {
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

const output_ports = document.getElementById("output_ports");
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

  [...output_ports.childNodes].forEach((group) => {
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
  mixer.name = "# mixer";
  mixer.classList.add("content");

  const label = document.createElement("div");
  mixer.appendChild(label);
  label.classList.add("content-title");

  const label_observer = new MutationObserver(() => {
    if (!document.contains(mixer))
      label_observer.disconnect();
    else {
      const idx = [...mixer_list.childNodes].indexOf(list_elem);
      const name = mixer.name.replace("#", idx + 1);
      list_label.innerHTML = label.innerHTML = name;
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
    port_observer.observe(output_ports, { childList: true });
  }

  const console = document.createElement("div");
  menu.appendChild(console);
  console.classList.add("mixer-console");

  const channels = document.createElement("div");
  console.appendChild(channels);
  channels.id = `${props.id}_channels`;
  channels.classList.add("mixer-channels");

  const master_ch = document.createElement("div");
  console.appendChild(master_ch);
  master_ch.classList.add("mixer-master");

  const master_div = document.createElement("div");
  master_ch.appendChild(master_div);
  master_div.classList.add("mixer-channel-vol");

  const master_ruler = document.createElement("div");
  master_div.appendChild(master_ruler);
  master_ruler.classList.add("mixer-channel-ruler");
  for (var i = 0; i < 12; i++)
    master_ruler.appendChild(document.createElement("div"));

  const master_vol = document.createElement("input");
  master_div.appendChild(master_vol);
  master_vol.classList.add("mixer-channel-slider");

  master_vol.id = `${props.id}_master`;
  master_vol.type = "range";
  master_vol.min = -60;
  master_vol.max = 6;
  master_vol.value = props.master;
  update_vol(master_vol);
  master_vol.oninput = master_vol.onchange = () => {
    rpc.mixer_set_master(props.id, Number(master_vol.value))
      .then((res) => {
        if (res !== null)
          master_vol.value = res;
      });
    update_vol(master_vol);
  }

  const new_channel = document.createElement("button");
  master_ch.appendChild(new_channel);
  new_channel.classList.add("mixer-channel-new");
  new_channel.innerHTML = "+";

  const master_label = document.createElement("div");
  master_ch.appendChild(master_label);
  master_label.classList.add("mixer-channel-label");
  master_label.innerHTML = "master";

  /*
  const channels = document.createElement("div");
  mixer.appendChild(channels);

  channels.id = `${props.id}_channels`;

  props.channels.forEach((ch) => impl_channel_new(props.id, ch));

  const channel_new = document.createElement("button");
  mixer.appendChild(channel_new);

  channel_new.innerHTML = "new channel";
  channel_new.onclick = () => rpc.channel_new(props.id).then((ch) => impl_channel_new(props.id, ch));
  */
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
rpc.register("mixer_set_port", impl_mixer_set_port);
rpc.register("mixer_set_master", (id, val) => {
  const master = document.getElementById(`${id}_master`);
  master.value = val;
  update_vol(master);
});

/*
rpc.register("channel_new", impl_channel_new);
rpc.register("channel_delete", (id) => {
  const channel = document.getElementById(id);
  channel.remove();
});
rpc.register("channel_set_src", (id, val) => {
  const src = document.getElementById(`${id}_src`);
  src.value = val;
});
rpc.register("channel_set_gain", (id, val) => {
  const gain = document.getElementById(`${id}_gain`);
  gain.value = val;
});
rpc.register("channel_set_balance", (id, val) => {
  const balance = document.getElementById(`${id}_balance`);
  balance.value = val;
});
*/
