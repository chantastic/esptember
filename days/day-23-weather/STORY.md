---
title: Weather Station
---

The internet arc opens with a deliberate softball: one GET, one JSON parse, one screen.
After twenty-one days of timing grammars and DNS liars, the first networked lesson should be about the *shape* of a fetch-and-render app, not about fighting a vendor.

Open-Meteo is the whole reason this works as day one of the arc.
No key, no account, no OAuth dance — the API-key lesson is day 27's job, and weather gets to be pure.

## One URL is the API lesson

Everything the screen shows rides a single request: current conditions, twelve hourly temperatures, the day's range, all selected by query parameters.
Reading that URL *is* reading the app's data model, which is the underrated virtue of well-designed REST — the contract fits in a `snprintf`.

The parse streams.
ArduinoJson reads straight off the HTTP connection, so the board never buffers the full payload — a habit worth forming before the lessons where payloads stop being polite.

## The board without a browser

Day 22's captive portal assumed a touchscreen and a phone.
The StopWatch has pushers and a serial port, and pretending otherwise would have meant porting a web portal to a device with no way to show one.

So this board's portal is the serial line: `wifi SSID PASS`, persisted to NVS, same one-home-for-secrets rule, different door.
The rule survived the hardware change; the mechanism adapted.
That's the sign the day-21 rule was the right abstraction — *where secrets live*, not *how they arrive*.

## Rounds want radial data

The 12-hour forecast could have been a bar chart, awkwardly rectangular on a circle.
Instead it's a bezel: twelve dots sweeping 270°, radius riding the temperature.
The round face keeps winning these design moments — data that has a cycle (hours, compass points, beats per bar) maps onto the rim like it was always meant to live there.

And the ten-minute refresh is a design position too: free APIs are a commons, and a weather station that polls every ten seconds is a tragedy-of-the-commons tutorial.
**B** exists for impatience; the timer exists for citizenship.

## What we learned

- Start the arc keyless. The first network lesson should teach fetch-parse-render, not account management.
- Stream the parse. Never buffer a payload you can read in flight.
- Abstractions survive hardware; mechanisms don't. Secrets-in-NVS crossed boards; the web portal didn't need to.
- Cyclic data belongs on the rim. Round displays turn hours into bezels for free.
