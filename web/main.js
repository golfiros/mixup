import { jsonrpc } from "/rpc.js";
import { input } from "/input.js";
import { mixer } from "/mixer.js";

const data = window.data = {};
const ws_url =
  window.location.protocol.replace("http", "ws") +
  "//" + window.location.host + "/rpc";
data.rpc = jsonrpc(ws_url);

const ports = data.ports = {};
data.rpc.register("port_new", (path, props) => ports[path] = props);
data.rpc.register("port_delete", (path) => delete ports[path]);

data.obj_map = {};
window.input_new = input(data);
window.mixer_new = mixer(data);


data.rpc.init();
