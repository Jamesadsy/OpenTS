---
title: Keep the existing audio device available in single-thread browser builds
category: fix
release: 0.2.0
targets:
- type: system
  id: music
  effect: changed
- type: system
  id: sound-effects
  effect: changed
credit: [OpenTS contributors]
---

The Emscripten build keeps its miniaudio device open when it cannot start a feeder thread. The existing feeder pass runs from the game sound callback, so menu music, speech, and sound streams keep using the same device and mixer. When the browser suspends that runtime's existing Web Audio context, a trusted canvas tap asks miniaudio to resume it. No additional audio context is created.
