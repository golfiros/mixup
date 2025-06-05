const input_ports = document.getElementById("input_ports");
const output_ports = document.getElementById("output_ports");

rpc.register("port_new", (props) => {
  const list = props.input ? input_ports : output_ports;
  var group = list.querySelector(`optgroup[label="${props.node}"]`);
  if (group === null) {
    group = document.createElement("optgroup");
    group.label = props.node;
    list.appendChild(group);
  }
  const port = document.createElement("option");
  port.innerHTML = port.value = props.path;
  group.appendChild(port);
});
rpc.register("port_delete", (path) => delete ports[path]);
