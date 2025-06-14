const input_ports = document.getElementById("input_ports");
const output_ports = document.getElementById("output_ports");

rpc.register("port_new", (props) => {
  const list = props.input ? input_ports : output_ports;
  var group = list.querySelector(`optgroup[label="${props.node}"]`);
  if (group === null) {
    group = document.createElement("optgroup");
    group.label = props.node;
    list.appendChild(group);
    [...list.childNodes]
      .filter((e) => e instanceof HTMLOptGroupElement)
      .sort((a, b) => a.label > b.label ? 1 : -1)
      .forEach((node) => list.appendChild(node));

  }
  const port = document.createElement("option");
  port.value = port.id = props.path;
  group.appendChild(port);
  [...group.childNodes]
    .sort((a, b) => a.value > b.value ? 1 : -1)
    .forEach((node, idx) => {
      const name = props.input ? `Input ${idx + 1}` : `Output ${idx + 1}`;
      node.setAttribute("open_name", name);
      node.innerHTML = `${name} - ${group.label}`;
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
