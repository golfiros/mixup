import { jsonrpc } from "/rpc.js";

const ws_url =
  window.location.protocol.replace("http", "ws") +
  "//" + window.location.host + "/rpc";
const rpc = jsonrpc(ws_url);
window.rpc = rpc;

const ports = {};
rpc.register("port_new", (path, props) => ports[path] = props);
rpc.register("port_delete", (path) => delete ports[path]);
window.ports = ports;

const buses = [];
window.buses = buses;

const bus_data = {};
const rpc_bus_new = (data) => {
  bus_data[data.id] = data;
  const bus = {
    delete: () => {
      buses.splice(buses.indexOf(bus), 1);
      rpc.bus_delete(data.id);
    },
    id: () => data.id,
    port: (idx) => data.port[idx],
    set_port: (idx, path) => {
      data.port[idx] = path;
      rpc.bus_set_port(data.id, idx, path);
    },
    gain: () => data.gain,
    set_gain: (gain) => {
      data.gain = gain;
      rpc.bus_set_gain(data.id, gain)
        .then((res) => {
          if (res !== null)
            data.gain = res;
        });
    },
    balance: () => data.balance,
    set_balance: (balance) => {
      data.balance = balance;
      rpc.bus_set_balance(data.id, balance)
        .then((res) => {
          if (res !== null)
            data.balance = res;
        });
    },
  };
  buses.push(bus);
};

rpc.register("bus_new", rpc_bus_new);
const bus_new = () => rpc.bus_new().then(rpc_bus_new);
window.bus_new = bus_new;

rpc.register("bus_delete", (ptr) => {
  const bus = buses.find((bus) => bus.id() == ptr);
  buses.splice(buses.indexOf(bus), 1);
  delete bus_data[ptr];
});

rpc.register("bus_set_port", (ptr, idx, path) => bus_data[ptr].port[idx] = path);
rpc.register("bus_set_gain", (ptr, gain) => bus_data[ptr].gain = gain);
rpc.register("bus_set_balance", (ptr, balance) => bus_data[ptr].balance = balance);

rpc.init();
