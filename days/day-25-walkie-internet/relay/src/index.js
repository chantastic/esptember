// Day 25 — the walkie relay: one Durable Object per channel, fanning
// binary WebSocket frames to every listener. The entire backend of an
// internet walkie-talkie is: accept a socket, remember it, forward
// bytes to the others. ~60 lines, deployed at the edge.

export class Channel {
  constructor(state) {
    this.state = state;
    this.sockets = new Set();
  }

  async fetch(request) {
    if (request.headers.get("Upgrade") !== "websocket")
      return new Response("expected websocket", { status: 426 });

    const [client, server] = Object.values(new WebSocketPair());
    server.accept();
    this.sockets.add(server);

    server.addEventListener("message", (event) => {
      // Voice frames are binary; forward to everyone but the speaker.
      for (const socket of this.sockets)
        if (socket !== server && socket.readyState === 1)
          socket.send(event.data);
    });
    const drop = () => this.sockets.delete(server);
    server.addEventListener("close", drop);
    server.addEventListener("error", drop);

    return new Response(null, { status: 101, webSocket: client });
  }
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    // /channel/7 -> the Durable Object named "7": one room per channel,
    // 22 rooms by convention, infinity by implementation.
    const match = url.pathname.match(/^\/channel\/(\d+)$/);
    if (!match) return new Response("esptember walkie relay", { status: 200 });
    const id = env.CHANNEL.idFromName(match[1]);
    return env.CHANNEL.get(id).fetch(request);
  },
};
