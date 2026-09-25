# Future voice integration (not the current Pi Pet scope)

This investigation was considered before the existing Pi-session pet was found. The recommended current phase preserves the existing host/hub/USB transport. Do not replace it with a new voice protocol merely because this reference is bundled.

The StopWatch has audio hardware, but the inspected visualization candidate disabled audio and its speech gateway was roadmap work. Revalidate source if voice is requested. A new provider route, cloud deployment, or actual speech call requires the user's task to authorize that work; a source design article is not such authorization.

Current xAI documentation inspected during the review: [Voice API reference](https://docs.x.ai/developers/rest-api-reference/inference/voice). Event names included `input_audio_buffer.speech_started`, `response.created`, `response.output_audio.delta`, and `response.done`. Recheck current provider documentation when implementing.

Useful design conclusions:

- Keep activity, connection health, and temporary expression separate.
- Speaking starts when the speaker actually plays decoded audio. Arrival of an audio chunk or `response.done` does not determine playback completion.
- Provider generation may finish while the speaker still has buffered output. Stay speaking until playback drains.
- Interruption stops/flushes playback and invalidates the old response/turn. Stale chunks or completion events must not restart speaking or trigger Done.
- Display only bounded state and optional playback-amplitude data. Keep provider secrets, raw provider payloads, full transcripts and tool arguments outside the visualization channel.
- A disconnect decays amplitude and exposes connection loss; it does not fabricate completion.

If a later transport extension is actually needed, consider version/session/sequence/turn identifiers, bounded frame size, known enums, snapshots on reconnect, and host-driven playback amplitude. The earlier idea of a new 256-byte JSON protocol, 1-second snapshots and 20 Hz amplitude was a proposal, not an existing contract or verified requirement. Do not impose it on today's working serial line protocol.

Meaningful later acceptance includes interruption during queued playback, generation finishing before playback, disconnect/reconnect, stale completion after cancel, and reduced-motion state distinction. These are design cases; no live voice test was performed in this work.
