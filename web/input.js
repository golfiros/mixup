export const input = (data) => {
  const map = data.obj_map;
  const inputs = data.inputs = [];
  const rpc = data.rpc;
  const input_new = (input) => {
    const id = input.id;
    input.id = () => id;
    map[id] = input;

    input.delete = () => {
      inputs.splice(inputs.indexOf(input), 1);
      rpc.input_delete(id);
    }
    input.set_port = (idx, path) => {
      input.port[idx] = path;
      rpc.input_set_port(id, idx, path);
    }
    input.set_gain = (gain) => {
      input.gain = gain;
      rpc.input_set_gain(id, gain)
        .then((res) => {
          if (res !== null)
            input.gain = res;
        });
    }
    input.set_balance = (balance) => {
      input.balance = balance;
      rpc.input_set_balance(id, balance)
        .then((res) => {
          if (res !== null)
            input.balance = res;
        });
    }
    inputs.push(input);
  };

  rpc.register("input_new", input_new);
  rpc.register("input_delete", (id) => {
    inputs.splice(inputs.indexOf(map[id]), 1);
    delete map[id];
  });
  rpc.register("input_set_port", (id, idx, path) => map[id].port[idx] = path);
  rpc.register("input_set_gain", (id, gain) => map[id].gain = gain);
  rpc.register("input_set_balance", (id, balance) => map[id].balance = balance);

  return () => rpc.input_new().then(input_new);
}
