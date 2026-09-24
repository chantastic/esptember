---
title: WorkOS AuthKit
---

Every lesson this month asked "what can a small screen and two inputs do?"
The finale asks the inverted question: what does the modern internet *demand* of a device, and can this one comply?

The demand is identity, and identity assumes a keyboard and a browser.
The board has neither.

## The flow built for keyboardless things

RFC 8628 exists precisely for this hardware shape.
The device asks the identity provider for a *pair* of codes: one short and human, one long and secret.
The human carries the short one to a real browser — here, by scanning a QR the board renders on its own face — and does the actual authenticating on pages built for it, password managers and passkeys and all.
The device just polls with its secret until the provider says "your human approved."

The division of labor is the elegance: the board never sees a password, never renders a login form, never touches a credential it could leak.
It learns exactly one thing — the verdict — and shows it with an email address.

## slow_down is part of the contract

Polling protocols usually degenerate into hammering.
RFC 8628 writes the etiquette into the grant: the server hands you an interval, and if you poll faster it answers `slow_down`, and the *specified* response is to add five seconds and feel shame.
The firmware implements the shame.
The same citizenship lesson as day 23's ten-minute weather refresh, now with the server enforcing it.

The three-way poll response — pending, slow down, done — plus expiry-and-refresh is the whole state machine, and every arm got a screen state.
Devices that show their state by name are devices whose bug reports are readable.

## Standing on the month

The build was an assembly, which after thirty days is the point.
Wi-Fi arrives through day 22's `portal.c`, unmodified.
The client id follows the secrets-live-in-NVS rule, entered over day 23's serial grammar.
The HTTPS-with-cert-bundle pattern came from day 25.
The endpoints themselves were proven on this hardware family by an earlier project of this desk — prior art is a renewable resource.

The only genuinely new part was the QR widget and the flow itself, which is how a finale should feel: the month did the work, the last day just introduced it to an identity provider.

## What we learned

- Device flow inverts authentication. The gadget delegates the hard part to hardware that's good at it and keeps only the verdict.
- Public client ids are public. The device grant is designed to survive shipping its only identifier in a downloadable binary.
- Rate limits belong in protocols. `slow_down` makes politeness a grammatical feature instead of a hope.
- A month compounds. The finale wrote one new state machine; everything else was already on the shelf.
