---
title: Clear mixer history when a new audio source starts
category: fix
release: 0.2.0
targets:
- type: format
  id: vqa
  effect: changed
- type: system
  id: music
  effect: changed
- type: system
  id: sound-effects
  effect: changed
credit:
- OpenTS contributors
---

When a completed movie voice was reused at the same rate and channel count, its resampler could carry cached movie samples and filter history into the next source. A new source now resets that history before rendering, so menu and game audio resume from their own PCM.
