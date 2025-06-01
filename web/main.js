import { jsonrpc } from "/rpc.js";
import { bus } from "/bus.js";
import { mixer } from "/mixer.js";

const ws_url =
  window.location.protocol.replace("http", "ws") +
  "//" + window.location.host + "/rpc";
const rpc = jsonrpc(ws_url);
window.rpc = rpc;

const ports = {};
rpc.register("port_new", (path, props) => ports[path] = props);
rpc.register("port_delete", (path) => delete ports[path]);
window.ports = ports;

window.obj_map = {};

window.buses = [];
window.bus_new = bus(window.obj_map, window.buses, rpc);

window.mixers = [];
window.mixer_new = mixer(window.obj_map, window.mixers, rpc);

rpc.init();
