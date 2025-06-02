export const mixer = (data) => {
  const map = data.obj_map;
  const mixers = data.mixers = [];
  const rpc = data.rpc;
  const channel_setup = (mixer, channel) => {
    const id = channel.id;
    channel.id = () => id;
    map[id] = channel;

    channel.src = map[channel.src];

    channel.delete = () => {
      const channels = mixer.channels;
      channels.splice(channels.indexOf(channel), 1);
      rpc.channel_delete(id)
    }
    channel.set_src = (obj) => {
      channel.src = obj;
      rpc.channel_set_src(id, obj.id());
    }
    channel.set_gain = (gain) => {
      channel.gain = gain;
      rpc.channel_set_gain(id, gain)
        .then((res) => {
          if (res !== null)
            channel.gain = res;
        });
    }
    channel.set_balance = (balance) => {
      channel.balance = balance;
      rpc.channel_set_balance(id, balance)
        .then((res) => {
          if (res !== null)
            channel.balance = res;
        });
    }
  }

  const channel_new = (mixer, channel) => {
    channel_setup(mixer, channel);
    mixer.channels.push(channel);
  };

  const mixer_new = (mixer) => {
    const id = mixer.id;
    mixer.id = () => id;
    map[id] = mixer;

    mixer.delete = () => {
      mixers.splice(mixers.indexOf(mixer), 1);
      rpc.mixer_delete(id);
      delete map[id];
    }
    mixer.set_port = (idx, path) => {
      mixer.port[idx] = path;
      rpc.mixer_set_port(id, idx, path);
    }
    mixer.set_master = (master) => {
      mixer.master = master;
      rpc.mixer_set_master(id, master)
        .then((res) => {
          if (res !== null)
            mixer.master = res;
        });
    }
    mixer.channel_new = () => rpc.channel_new(id).then((channel) => channel_new(mixer, channel));
    mixer.channels.forEach((channel) => channel_setup(mixer, channel));
    mixers.push(mixer);
  };

  rpc.register("mixer_new", mixer_new);
  rpc.register("mixer_delete", (id) => {
    mixers.splice(mixers.indexOf(map[id]), 1);
    delete map[id];
  });
  rpc.register("mixer_set_port", (id, idx, path) => map[id].port[idx] = path);
  rpc.register("mixer_set_master", (id, master) => map[id].master = master);

  rpc.register("channel_new", channel_new);
  rpc.register("channel_delete", (mixer_id, id) => {
    const channels = map[mixer_id].channels;
    channels.splice(channels.indexOf(map[id]), 1);
    delete map[id];
  });
  rpc.register("channel_set_src", (id, obj_id) => map[id].src = map[obj_id]);
  rpc.register("channel_set_gain", (id, gain) => map[id].gain = gain);
  rpc.register("channel_set_balance", (id, balance) => map[id] = balance);

  return () => rpc.mixer_new().then(mixer_new);
}
