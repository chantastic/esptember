---
title: Connected
---

Day 31 ended on a trophy screen: AUTHORIZED, your email, done.
Then someone asked the board "are you connected?" and the honest answer was *historically* — the sign-in had happened, the session existed server-side, and the firmware had kept nothing at all.
The tokens lived in a function scope and died with it.

A trophy is not a relationship.

## Residency is one string

The fix is almost embarrassingly small: the authenticate response carries a refresh token, and persisting it makes the identity survive everything — reboots, reflashes, weeks in a drawer.
The boot path becomes: try the refresh grant; only if there's nothing to refresh, hold the QR ceremony.
One scan, ever.

The operational subtlety is rotation: WorkOS retires each refresh token as it's used and issues a successor.
Miss saving the newest one and the residency dies silently on the next boot.
The save therefore lives inside the token-absorbing function, unskippable — the same "durability promises must be structural" lesson day 13 taught about workout logs, now guarding a session.

The verification had a nice accident in it: the silent resume was proven **across a firmware swap** — day 33 signed in, day 32 flashed over it and resumed the same session from NVS, no QR.
The identity belongs to the device now, not to any one build.

## Read your own token

The second idea costs zero network: an access token is a JWT, a JWT is three base64url blobs, and the middle blob is claims — org, session id, role, expiry — sitting in memory, readable by forty lines of decode.
The session card is just the token explaining itself.

Most integrations treat tokens as opaque ticket stubs and ask a server who they are.
The board asks the token.

## Five minutes, animated

AuthKit access tokens live about five minutes, which sounds like an inconvenience and is actually the day's best demo: the card shows a countdown, you watch it drain, and near the bottom it snaps back with `renewed silently`.
The OAuth refresh cycle — the thing every web app does invisibly, the thing tutorials wave at with a diagram — is running as a visible heartbeat on a desk ornament.
Instruments again: day 15 made loudness visible, day 20 made confidence visible, day 32 makes *session liveness* visible.

## What we learned

- Sign-in without persistence is a trophy. Residency costs one saved string and a boot-time refresh.
- Rotation makes saving structural. The newest refresh token must be kept by construction, not by discipline.
- Tokens are documents. The claims you need are already on the device — decode, don't ask.
- Animate the invisible. A five-minute token with a countdown teaches OAuth better than any sequence diagram.
