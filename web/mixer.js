const src_list = document.getElementById("src_list");

const impl_channel_new = (id, props) => {
  console.log(props);
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
    else
      src.innerHTML = src_list.innerHTML;
  });
  src_observer.observe(src_list, { childList: true, subtree: true });
  src.value = props.src;

  src.onchange = () =>
    rpc.channel_set_src(props.id, src.value);

  const gain = document.createElement("input");
  channel.appendChild(gain);

  gain.id = `${props.id}_gain`;
  gain.type = "range";
  gain.min = -90;
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

const mixer_list = document.getElementById("mixer_list");
const input_new = document.getElementById("mixer_new");

const output_ports = document.getElementById("output_ports");

const impl_mixer_new = (props) => {
  const mixer = document.createElement("div");
  mixer.id = props.id;
  mixer.name = "# mixer";

  const label = document.createElement("div");
  mixer.appendChild(label);

  const label_observer = new MutationObserver(() => {
    if (!document.contains(mixer))
      label_observer.disconnect();
    else {
      const idx = Array.from(mixer_list.childNodes).indexOf(mixer);
      label.innerHTML = mixer.name.replace("#", idx + 1);
    }
  });
  label_observer.observe(mixer_list, { childList: true });

  mixer_list.appendChild(mixer);

  const delete_button = document.createElement("button");
  mixer.appendChild(delete_button);

  delete_button.innerHTML = "delete";
  delete_button.onclick = () => {
    if (!confirm("confirm delete"))
      return;
    rpc.mixer_delete(props.id);
    mixer.remove();
  };

  const ports = document.createElement("div");
  mixer.appendChild(ports);

  for (var i = 0; i < 2; i++) {
    const port = document.createElement("select");
    ports.appendChild(port);
    port.id = `${props.id}_port_${i}`;
    port.innerHTML = output_ports.innerHTML;
    const port_observer = new MutationObserver(() => {
      if (!document.contains(mixer))
        port_observer.disconnect();
      else
        port.innerHTML = output_ports.innerHTML;
    });
    port_observer.observe(output_ports, { childList: true, subtree: true });

    const idx = i;
    port.value = props.port[idx];
    port.onchange = () =>
      rpc.mixer_set_port(props.id, idx, port.value);
  }

  const channels = document.createElement("div");
  mixer.appendChild(channels);

  channels.id = `${props.id}_channels`;

  props.channels.forEach((ch) => impl_channel_new(props.id, ch));

  const channel_new = document.createElement("button");
  mixer.appendChild(channel_new);

  channel_new.innerHTML = "new channel";
  channel_new.onclick = () => rpc.channel_new(props.id).then((ch) => impl_channel_new(props.id, ch));

  const master = document.createElement("input");
  mixer.appendChild(master);

  master.id = `${props.id}_master`;
  master.type = "range";
  master.min = -90;
  master.max = 6;
  master.value = props.master;
  master.oninput = master.onchange = () =>
    rpc.mixer_set_master(props.id, Number(master.value))
      .then((res) => {
        if (res !== null)
          master.value = res;
      });
};

input_new.onclick = () => rpc.mixer_new().then(impl_mixer_new);

rpc.register("mixer_new", impl_mixer_new);
rpc.register("mixer_delete", (id) => {
  const mixer = document.getElementById(id);
  mixer.remove();
});
rpc.register("mixer_set_port", (id, idx, path) => {
  const port = document.getElementById(`${id}_port_${idx}`);
  port.value = path;
});
rpc.register("mixer_set_master", (id, val) => {
  const master = document.getElementById(`${id}_master`);
  master.value = val;
});

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
