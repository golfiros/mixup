const ids = {};
const rand_id = () => {
  const chars = "0123456789abcdef";
  var id = "";
  do
    for (var i = 0; i < 8; i++)
      id += `${chars[Math.floor(Math.random() * chars.length)]}`;
  while (ids[id] !== undefined);
  ids[id] = 0;
  return id;
}

var data_str = localStorage.getItem("dummy_storage");
if (data_str === null) {
  const data = { inputs: [], mixers: [] };
  for (var node = 0; node < 6; node++)
    for (var port = 0; port < 2; port++) {
      data.inputs.push({
        id: rand_id(),
        name: "microphone #",
        port: [`audio:node_${node}:capture_${port}`, ""],
        gain: 15.0 * Math.random() - 7.5,
        balance: 160.0 * Math.random() - 80.0,
        eq: [50, 200, 1000, 5000, 15000].map((f) => {
          return {
            gain: 12.0 * Math.random() - 6.0,
            quality: Math.exp(3 * Math.random() - 0.5 * Math.log(2)),
            freq: Math.exp(0.05 * Math.random() + Math.log(f)),
          };
        }),
      });
    }
  for (var node = 12; node < 16; node++)
    data.inputs.push({
      id: rand_id(),
      name: `device ${(node - 12 + 10).toString(36).toUpperCase()}`,
      port: [`audio:node_${node}:capture_0`, `audio:node_${node}:capture_1`],
      gain: 15.0 * Math.random() - 7.5,
      balance: 160.0 * Math.random() - 80.0,
      eq: [50, 200, 1000, 5000, 15000].map((f) => {
        return {
          gain: 12.0 * Math.random() - 6.0,
          quality: Math.exp(1 * Math.random() - 0.5 * Math.log(2)),
          freq: Math.exp(0.05 * Math.random() + Math.log(f)),
        };
      }),
    });
  for (var node = 0; node < 6; node++)
    data.mixers.push({
      id: rand_id(),
      name: "musician #",
      port: [`audio:node_${node}:playback_0`, `audio:node_${node}:playback_1`],
      master: 30.0 * Math.random() - 27.0,
      channels: data.inputs.map((input, idx) => {
        return {
          id: rand_id(),
          name: idx < 12 ? "# mic" : `dev ${(idx - 12 + 10).toString(36).toUpperCase()}`,
          src: input.id,
          gain: 30.0 * Math.random() - 27.0,
          balance: 160.0 * Math.random() - 80.0,
          mute: Math.random() > 0.90,
        }
      }),
    });
  localStorage.setItem("dummy_storage", data_str = JSON.stringify(data));
}

const data = JSON.parse(data_str);

const rpc = window.rpc = {
  input_new: () => new Promise((resolve) => {
    const input = {
      id: rand_id(),
      name: "# input",
      port: ["", ""],
      gain: 0.0,
      balance: 0.0,
      eq: [50, 200, 1000, 5000, 15000].map((f) => {
        return {
          gain: 0.,
          quality: 1 / Math.sqrt(2),
          freq: f,
        };
      }),
    };
    data.inputs.push(input);
    localStorage.setItem("dummy_storage", JSON.stringify(data));
    resolve(input);
  }),
  input_delete: (id) => {
    data.inputs = data.inputs.filter((input) => input.id !== id);
    localStorage.setItem("dummy_storage", JSON.stringify(data));
  },
  input_set_index: (id, idx) => {
    data.inputs.splice(idx, 0, data.inputs.splice(data.inputs.findIndex((input) => input.id === id), 1)[0]);
    localStorage.setItem("dummy_storage", JSON.stringify(data));
  },
  input_set_name: (id, name) => {
    data.inputs.find((input) => input.id === id).name = name;
    localStorage.setItem("dummy_storage", JSON.stringify(data));
  },
  input_set_port: (id, idx, path) => {
    data.inputs.find((input) => input.id === id).port[idx] = path;
    localStorage.setItem("dummy_storage", JSON.stringify(data));
  },
  input_set_gain: (id, gain) => {
    data.inputs.find((input) => input.id === id).gain = gain;
    localStorage.setItem("dummy_storage", JSON.stringify(data));
    return new Promise((resolve) => resolve(gain));
  },
  input_set_balance: (id, balance) => {
    data.inputs.find((input) => input.id === id).balance = balance
    localStorage.setItem("dummy_storage", JSON.stringify(data));
    return new Promise((resolve) => resolve(balance));
  },
  input_set_eq_freq: (id, stage, freq) => {
    data.inputs.find((input) => input.id === id).eq[stage].freq = freq
    localStorage.setItem("dummy_storage", JSON.stringify(data));
    return new Promise((resolve) => resolve(freq));
  },
  input_set_eq_quality: (id, stage, quality) => {
    data.inputs.find((input) => input.id === id).eq[stage].quality = quality
    localStorage.setItem("dummy_storage", JSON.stringify(data));
    return new Promise((resolve) => resolve(quality));
  },
  input_set_eq_gain: (id, stage, gain) => {
    data.inputs.find((input) => input.id === id).eq[stage].gain = gain
    localStorage.setItem("dummy_storage", JSON.stringify(data));
    return new Promise((resolve) => resolve(gain));
  },

  mixer_new: () => new Promise((resolve) => {
    const mixer = {
      id: rand_id(),
      name: "# mixer",
      port: ["", ""],
      master: 0.0,
      channels: [],
    };
    data.mixers.push(mixer);
    localStorage.setItem("dummy_storage", JSON.stringify(data));
    resolve(mixer);
  }),
  mixer_delete: (id) => {
    data.mixers = data.mixers.filter((mixer) => mixer.id !== id);
    localStorage.setItem("dummy_storage", JSON.stringify(data));
  },
  mixer_set_index: (id, idx) => {
    data.mixers.splice(idx, 0, data.mixers.splice(data.mixers.findIndex((mixer) => mixer.id === id), 1)[0]);
    localStorage.setItem("dummy_storage", JSON.stringify(data));
  },
  mixer_set_name: (id, name) => {
    data.mixers.find((mixer) => mixer.id === id).name = name;
    localStorage.setItem("dummy_storage", JSON.stringify(data));
  },
  mixer_set_port: (id, idx, path) => {
    data.mixers.find((mixer) => mixer.id === id).port[idx] = path;
    localStorage.setItem("dummy_storage", JSON.stringify(data));
  },
  mixer_set_master: (id, master) => {
    data.mixers.find((mixer) => mixer.id === id).master = master;
    localStorage.setItem("dummy_storage", JSON.stringify(data));
    return new Promise((resolve) => resolve(master));
  },

  channel_new: (mix_id) => new Promise((resolve) => {
    const channel = {
      id: rand_id(),
      name: "# channel",
      src: "00000000-0000-0000-0000-000000000000",
      gain: 0.0,
      balance: 0.0,
      mute: true,
    };
    const mixer = data.mixers.find((mixer) => mixer.id === mix_id);
    mixer.channels.push(channel);
    resolve(channel)
  }),
  channel_delete: (id) => {
    data.mixers.forEach((mixer) => mixer.channels.filter((ch) => ch.id !== id));
    localStorage.setItem("dummy_storage", JSON.stringify(data));
  },
  channel_set_index: (id, idx) => {
    data.mixers.forEach((mixer) => {
      const idx_old = mixer.channels.findIndex((ch) => ch.id === id);
      if (idx_old < 0)
        return;
      mixer.channels.splice(idx, 0, mixer.channels.splice(idx_old, 1)[0]);
    });
    localStorage.setItem("dummy_storage", JSON.stringify(data));
  },
  channel_set_name: (id, name) => {
    data.mixers.forEach((mixer) => {
      const channel = mixer.channels.find((ch) => ch.id === id);
      if (channel !== undefined)
        channel.name = name;
    });
    localStorage.setItem("dummy_storage", JSON.stringify(data));
  },
  channel_set_src: (id, src) => {
    data.mixers.forEach((mixer) => {
      const channel = mixer.channels.find((ch) => ch.id === id);
      if (channel !== undefined)
        channel.src = src;
    });
    localStorage.setItem("dummy_storage", JSON.stringify(data));
  },
  channel_set_gain: (id, gain) => {
    data.mixers.forEach((mixer) => {
      const channel = mixer.channels.find((ch) => ch.id === id);
      if (channel !== undefined)
        channel.gain = gain;
    });
    localStorage.setItem("dummy_storage", JSON.stringify(data));
    return new Promise((resolve) => resolve(gain));
  },
  channel_set_balance: (id, balance) => {
    data.mixers.forEach((mixer) => {
      const channel = mixer.channels.find((ch) => ch.id === id);
      if (channel !== undefined)
        channel.balance = balance;
    });
    localStorage.setItem("dummy_storage", JSON.stringify(data));
    return new Promise((resolve) => resolve(balance));
  },

  funcs: {},
  register: (key, val) => rpc.funcs[key] = val,

  init: () => new Promise((resolve) => {
    for (var node = 0; node < 6; node++)
      for (var port = 0; port < 2; port++) {
        rpc.funcs.port_new({
          path: `audio:node_${node}:capture_${port}`,
          node: `Interface ${node + 1}`,
          input: true,
        });
        rpc.funcs.port_new({
          path: `audio:node_${node}:playback_${port}`,
          node: `Interface ${node + 1}`,
          input: false,
        });
      }
    for (var node = 12; node < 16; node++)
      for (var port = 0; port < 2; port++)
        rpc.funcs.port_new({
          path: `audio:node_${node}:capture_${port}`,
          node: `DAC ${node - 12 + 1}`,
          input: true,
        });
    data.inputs.forEach(rpc.funcs.input_new);
    data.mixers.forEach(rpc.funcs.mixer_new);
    resolve(null);
    rpc.funcs.done();
  }),
}
