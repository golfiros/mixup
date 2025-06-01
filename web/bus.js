export const bus = (map, buses, rpc) => {
  const bus_new = (bus) => {
    const id = bus.id;
    bus.id = () => id;
    map[id] = bus;

    bus.delete = () => {
      buses.splice(buses.indexOf(bus), 1);
      rpc.bus_delete(id);
    }
    bus.set_port = (idx, path) => {
      bus.port[idx] = path;
      rpc.bus_set_port(id, idx, path);
    }
    bus.set_gain = (gain) => {
      bus.gain = gain;
      rpc.bus_set_gain(id, gain)
        .then((res) => {
          if (res !== null)
            bus.gain = res;
        });
    }
    bus.set_balance = (balance) => {
      bus.balance = balance;
      rpc.bus_set_balance(id, balance)
        .then((res) => {
          if (res !== null)
            bus.balance = res;
        });
    }
    buses.push(bus);
  };

  rpc.register("bus_new", bus_new);
  rpc.register("bus_delete", (id) => {
    buses.splice(buses.indexOf(map[id]), 1);
    delete map[id];
  });
  rpc.register("bus_set_port", (id, idx, path) => map[id].port[idx] = path);
  rpc.register("bus_set_gain", (id, gain) => map[id].gain = gain);
  rpc.register("bus_set_balance", (id, balance) => map[id].balance = balance);

  return () => rpc.bus_new().then(bus_new);
}
