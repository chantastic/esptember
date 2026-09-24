---
title: Walkie-Talkie, Worldwide
---

The pitch for this pair of lessons was made a month ago: build the walkie-talkie twice, changing only the transport, and let the diff teach.
The diff came in smaller than promised.

## What actually changed

On the transmitter: `esp_now_send(BROADCAST, ...)` became `ws.sendBIN(...)`, plus a connect state machine.
On the receiver: the ESP-NOW callback became a WebSocket event handler.
The frame struct, the mic pacing, the jitter buffer, the squelch, the channel dial, the screens — byte-for-byte day 24.

That's the lesson working as designed: when the *frame* is the contract, the carrier underneath is an implementation detail.
Day 21 proved it across vendors; day 25 proves it across a planet.

## The backend is sixty lines

The relay had candidates: public MQTT (higher latency, someone else's broker), Mumble (real VoIP, someone else's client), or a from-scratch WebSocket fan-out.
The from-scratch option won because it fits in a screen of JavaScript: accept a socket, remember it, forward binary messages to the other sockets in the room.

Durable Objects make the room primitive free: `idFromName("7")` conjures a tiny stateful server for channel 7, living wherever Cloudflare finds convenient, holding nothing but a `Set` of sockets.
Twenty-two channels by convention.
Infinity by implementation.
Rooms are URLs, and changing channels is rejoining a different one.

The relay was verified before any firmware touched it — two host-side WebSocket clients, four bytes in, four bytes out the other side, half a second around the world and back.
Test the new organ before transplanting it.

## Jitter is jitter

The prediction was that internet transport would need a different audio strategy than radio.
It didn't.
Internet jitter and RF jitter are the same disease — frames arrive unevenly, the speaker consumes evenly — and day 24's 60 ms prebuffer treats both without modification.
The only concession to distance is latency the buffer never sees: the relay adds real milliseconds, but a walkie-talkie conversation carries half-duplex rhythm anyway, and PTT culture absorbs delay that would kill a phone call.

## Provisioning paid out

This is the first day that *needed* day 22 and day 23 simultaneously — two boards, two provisioning styles, one rule.
The RX lifts `portal.c` verbatim; the TX reads day 23's NVS namespace so a board provisioned for weather is already provisioned for voice.
Secrets have one home, and the home has compound interest.

## What we learned

- Transports are swappable when the frame is the contract. The audio pipeline crossed from RF to internet untouched.
- Rooms are URLs. Durable Objects turn channel numbers into servers with one `idFromName`.
- Jitter is jitter. One prebuffer treats radio and internet alike.
- Verify the organ before the transplant. The relay proved itself host-to-host before any firmware depended on it.
