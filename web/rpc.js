// taken from the mongoose JSON-RPC tutorial

// Copyright (c) 2004-2013 Sergey Lyubka
// Copyright (c) 2013-2025 Cesanta Software Limited
// Copyright (c) 2025-2025 Gabriel Golfetti
// All rights reserved
//
// This software is dual-licensed: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation. For the terms of this
// license, see <http://www.gnu.org/licenses/>.
//
// You are free to use this software under the terms of the GNU General
// Public License, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// Alternatively, you can license this software under a commercial
// license, as set out in <https://mongoose.ws/licensing/>.

// JSON-RPC over Websocket implementation

const JSONRPC_TIMEOUT_MS = 1000;

export const jsonrpc = (url) => {
  var rpcid = 0, pending = {}, methods = {};

  const ws = new WebSocket(url);
  if (!ws)
    return null;

  const call = (method, params) => {
    const id = rpcid++, request = { id, method, params };
    ws.send(JSON.stringify(request));
    return new Promise((resolve, reject) => {
      setTimeout(() => {
        if (pending[id] === undefined) return;
        delete pending[id];
        reject(0, "Message timeout");
      }, JSONRPC_TIMEOUT_MS);
      pending[id] = { resolve, reject };
    });
  };

  ws.onmessage = (ev) => {
    const frame = JSON.parse(ev.data);
    console.log('rcvd', frame, 'pending:', pending);
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
  const rpc = {
    init: () => new Promise((resolve, reject) => {
      setTimeout(() => {
        if (!ready)
          reject()
      }, JSONRPC_TIMEOUT_MS);
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

  return rpc;
};
