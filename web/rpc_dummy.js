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
      });
    }
  for (var node = 12; node < 16; node++)
    data.inputs.push({
      id: rand_id(),
      name: `device ${(node - 12 + 10).toString(36).toUpperCase()}`,
      port: [`audio:node_${node}:capture_0`, `audio:node_${node}:capture_1`],
      gain: 15.0 * Math.random() - 7.5,
      balance: 160.0 * Math.random() - 80.0,
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
        }
      }),
    });
  localStorage.setItem("dummy_storage", data_str = JSON.stringify(data));
}

const data = new Proxy({ data: data_str }, {
  get: (target, prop) => JSON.parse(target.data)[prop],
  set: (target, prop, val) => {
    const data_local = JSON.parse(target.data);
    data_local[prop] = val;
    localStorage.setItem("dummy_storage", target.data = JSON.stringify(data_local));
    return true;
  },
});

const rpc = window.rpc = {
  input_new: () => new Promise((resolve) => {
    const input = {
      id: rand_id(),
      name: "# input",
      port: ["", ""],
      gain: 0.0,
      balance: 0.0,
    };
    data.inputs.push(input);
    resolve(input);
  }),
  input_delete: (id) => data.inputs = data.inputs.filter((input) => input.id !== id),
  input_set_index: (id, idx) => {
    const idx_old = data.inputs.findIndex((input) => input.id === id);
    const temp = data.inputs[idx_old];
    data.inputs[idx_old] = data_inputs[idx];
    data.inputs[idx] = temp;
  },
  input_set_name: (id, name) => data.inputs.find((input) => input.id === id).name = name,
  input_set_port: (id, idx, path) => data.inputs.find((input) => input.id === id).ports[idx] = path,
  input_set_gain: (id, gain) => data.inputs.find((input) => input.id === id).gain = gain,
  input_set_balance: (id, balance) => data.inputs.find((input) => input.id === id).balance = balance,

  mixer_new: () => new Promise((resolve) => resolve({
    id: rand_id(),
    name: "# mixer",
    port: ["", ""],
    master: 0.0,
    channels: [],
  })),
  mixer_delete: () => { },
  mixer_set_index: () => { },
  mixer_set_name: () => { },
  mixer_set_port: () => { },
  mixer_set_master: () => null,

  channel_new: () => new Promise((resolve) => resolve({
    id: rand_id(),
    name: "# channel",
    src: "00000000-0000-0000-0000-000000000000",
    gain: 0.0,
    balance: 0.0,
  })),
  channel_delete: () => { },
  channel_set_src: () => { },
  channel_set_index: () => { },
  channel_set_name: () => { },
  channel_set_gain: () => null,
  channel_set_balance: () => null,

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
