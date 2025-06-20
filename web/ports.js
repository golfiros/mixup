const input_ports = document.getElementById("input_ports");
const output_ports = document.getElementById("output_ports");

rpc.register("port_new", (props) => {
  const list = props.input ? input_ports : output_ports;
  var group = list.querySelector(`div[title="${props.node}"]`);
  if (group === null) {
    group = document.createElement("div");
    group.title = props.node;
    list.appendChild(group);
    [...list.childNodes]
      .sort((a, b) => a.title > b.title ? 1 : -1)
      .forEach((node) => list.appendChild(node));
  }
  const port = document.createElement("div");
  port.id = props.path;
  group.appendChild(port);
  [...group.childNodes]
    .sort((a, b) => a.id > b.id ? 1 : -1)
    .forEach((node, idx) => {
      node.innerHTML = props.input ? `input ${idx + 1}` : `output ${idx + 1}`;
      group.appendChild(node);
    });
});
rpc.register("port_delete", (path) => {
  const port = document.getElementById(path);
  const group = port.parentNode;
  port.remove();
  if (!group.childNodes.length)
    group.remove();
});
