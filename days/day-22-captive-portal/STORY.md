---
title: Captive Portal
---

Twenty days in, the series hits the wall every public firmware repo hits: the next ten lessons need Wi-Fi, and Wi-Fi needs a password, and the repository is public.
Hardcoding credentials is out.
A config header in `.gitignore` is a tutorial-killer — "edit secrets.h before building" loses half the audience at the sentence.

The answer is the pattern your espresso machine uses: the device *is* the setup page.

## The DNS server that lies

The magic moment of a captive portal — join the network, page appears unbidden — is not magic and not HTTP.
It's DNS.

Every OS probes a known URL after joining a network to ask "is this real internet?"
The portal answers that DNS lookup — and every other lookup — with its own address, so the probe's HTTP request lands on the board, returns the wrong content, and the OS concludes: captive portal, better show it to the human.
The board exploits the OS's own connectivity paranoia as a UI channel.

The DNS responder that does this is barely a parser: read the query's header, flip the response bit, append one answer record pointing home.
Thirty lines, no library, and the header comment calls it what it is — a liar's DNS.

## AP+STA is the underrated detail

The portal's dropdown lists your actual networks, live-scanned as the page loads.
That requires the radio to host an access point and scan as a station *simultaneously* — `WIFI_MODE_APSTA` — and it's the difference between "select your network" and "type your SSID from memory, get one character wrong, hold BOOT, try again."

Which is also why the BOOT-hold escape hatch is non-negotiable: station mode retries a bad password forever, and *forever* needs a door.

## Secrets have exactly one home

The design rule this day establishes for the rest of the month: credentials live in NVS, written by the portal, read by `portal_load_creds()`, erased by a held button.
No build flags, no environment variables baked into binaries, no gitignored headers.
The published `.bin` on the site is identical for every reader precisely because it contains nothing personal — the *device* gets personalized, not the firmware.

`portal.c` ships as a deliberately liftable module.
The next nine days import the pattern; the form grows API-key fields as lessons need them.

## What we learned

- Captive portals are DNS tricks, not web tricks. Answer everything with yourself and the OS does the rest.
- AP+STA earns its keystrokes. A live network dropdown is the whole UX gap between setup and support ticket.
- Retry-forever needs a door. The BOOT-hold reset is one GPIO read and the difference between resilient and bricked-by-typo.
- Personalize the device, not the firmware. One public binary, secrets only ever in NVS — the repo stays clean by construction.
