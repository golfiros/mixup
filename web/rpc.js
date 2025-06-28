const url =
  window.location.protocol.replace("http", "ws") +
  "//" + window.location.host + "/rpc";

var rpcid = 0, pending = {}, methods = {};

const ws = new WebSocket(url);

const call = (method, params) => {
  const id = rpcid++, request = { id, method, params };
  ws.send(JSON.stringify(request));
  return new Promise((resolve, reject) => {
    setTimeout(() => {
      if (pending[id] === undefined) return;
      delete pending[id];
      reject(0, "Message timeout");
    }, 1000);
    pending[id] = { resolve, reject };
  });
};

ws.onmessage = (ev) => {
  const frame = JSON.parse(ev.data);
  if (frame.id !== undefined) {
    if (frame.result !== undefined)
      pending[frame.id].resolve(frame.result);
    else
      pending[frame.id].reject(frame.error.code, frame.error.message);
    delete (pending[frame.id]);
  } else if (frame.method !== undefined && frame.params !== undefined)
    methods[frame.method](...frame.params);
  else
    throw new Error("Unexpected message");
};

var ready = false;
var init = () => { };
const rpc = window.rpc = {
  init: () => new Promise((resolve, reject) => {
    setTimeout(() => {
      if (!ready)
        reject()
    }, 1000);
    init = () => call("init").then(resolve, reject);
    if (ready)
      init();
  }),
  close: () => ws.close(),
  register: (name, method) => methods[name] = method,
};

ws.onopen = () => call("rpc.list", [])
  .then((res) => {
    res.filter((elem) => elem !== "rpc.list" && elem !== "init")
      .forEach((method) => {
        rpc[method] = (...args) => call(method, args)
      })
    ready = true;
    if (init !== null)
      init();
  });
