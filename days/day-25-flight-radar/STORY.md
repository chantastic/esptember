---
title: Flight Radar
---

The best free API on the internet is powered by people's roofs.

Aircraft broadcast their position, altitude, and identity in the clear at 1090 MHz — a design decision from an era that assumed listeners would be air traffic control, not hobbyists with $20 software-defined radios.
The hobbyists came anyway, by the tens of thousands, and aggregators like adsb.lol merge their feeds into a live picture of the sky that costs nothing and asks for no key.

This lesson consumes that gift with one GET every eight seconds.

## Filter the parse or drown

Day 22's weather payload was polite — a few hundred bytes.
An ADS-B point query near a city returns dozens of aircraft, each a fat record of registration, squawk, signal metadata, and twenty other fields the scope will never draw.

ArduinoJson's filter document is the tool built for exactly this: declare the five fields you want, and the parser discards everything else *as it streams*.
The JSON work stays tiny no matter how busy the sky gets.
Day 22 taught streaming; day 25 teaches selective hearing.

## Flat-earth math is correct here

Great-circle distance is the reflex, and at 25 nautical miles it's a waste of trigonometry.
An equirectangular projection — treat degrees as a grid, shrink longitude by one cosine of latitude — is accurate to rounding error at scope range, and it turns every blip position into two multiplies.
Knowing when the cheap approximation is *actually right* is a better lesson than always paying for the exact one.

## The sweep is load-bearing decoration

A rotating radar sweep synchronized to the fetch interval adds zero information.
It was also non-negotiable from the first sketch, because the sweep is what tells a human "this instrument is alive and looking" — the same job day 14's peak-hold and day 19's nodding dot did.
Instruments need vital signs.
The blips are data; the sweep is trust.

The velocity leaders, though, are real: a scope showing dots without direction shows where planes *were*.
Fourteen pixels of line along the track turns a map into a prediction.

## What we learned

- The sky is an open API. ADS-B is broadcast in the clear, and volunteers made it a free web service.
- Filtered parsing is selective hearing. Five fields survive; the payload's obesity becomes irrelevant.
- Cheap projections are right at small scales. One cosine beats a haversine inside 25 nm.
- Instruments need vital signs. The sweep informs nothing and reassures everything.
