---
title: Pipes
---
## The prompt-first pass

The original build below records how this idea first worked.
The durable lesson now targets the M5Stack Stopwatch and lives in the prompt, behavior contract, generalized tests, and hand-review checklist.
Generated firmware is evidence for those sources, not the source itself.


The question that started this day sounded simple: *"post-auth, can the device show my connections?"*
The answer took a Worker, three wrong assumptions, and one humbling grep through production code.

## The trust ladder

The design fell out of one constraint: the WorkOS API key must never touch the device.
So the layers arranged themselves into a ladder — the board holds only the *user's* token and can prove who it acts for; a Cloudflare Worker holds the *app's* key and does everything on the board's behalf; and with Pipes Relay, the *provider* tokens (GitHub's, YouTube's) never leave WorkOS at all.
Each layer can only leak what it holds, and the device holds the least.

The firmware went thin on purpose after the first draft: one authenticated `GET /screen`, render the answer.
Connect URLs arrive pre-minted; detail lines arrive pre-fetched; new providers are a Worker redeploy, not a reflash.
The device is a terminal; the edge is the app.

## Three bugs, one lesson each

**The 512-byte ambush.** The board could reach WorkOS but not its own gateway — connection reset, `http=-1`, looked like TLS.
The unfiltered log told the truth in one line: *"Buffer length is small to fit all the headers."*
esp_http_client's default header buffer is 512 bytes; an AuthKit JWT bearer is ~1 KB.
Every WorkOS call had worked because none of them carried the token.

**The wrong endpoint.** The docs' `/data-integrations` lists the *catalog*; the user's actual connections live at `/user_management/users/{id}/data_providers`, org-scoped.
The correct call wasn't found in documentation — it was found in the production source of the desk's own prior project, already battle-tested.
When you own working code, grep it before you trust your reading of the docs.

**The one-word filter.** With everything else fixed, six live connections still read as zero, because the state string is `connected` and the filter accepted `active`, `valid`, `authorized` — every plausible synonym except the real one.
Guessing enum values is writing fiction.
Log the actual object once and the fiction ends.

There was also a fourth, self-inflicted classic: a directory rename left the ESP-IDF build directory configured for the old path, so three rounds of "fixes" flashed the same stale binary while the source sat corrected.
Deterministic mystery = check what you actually shipped.

## The moment it worked

`connected=1` six times, on hardware, from the same account the user was browsing on the web at that moment — connections made in a browser appearing on a microcontroller, because they belong to the *person*, not the app.
That symmetry was the user's expectation from the first question, and the expectation was correct the entire time.
Every bug was in the plumbing that doubted it.

## What we learned

- Arrange trust as a ladder. Device < Worker < WorkOS — each layer leaks at most what it holds, and the device holds the least.
- Thin clients age best. One request, one render; capability lives where deploys are instant.
- Production code outranks documentation. The right endpoint was already proven in a sibling repo.
- Never guess enum strings. One logged object beats four plausible synonyms.
