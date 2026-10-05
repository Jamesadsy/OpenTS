---
title: Drain movie audio before stream reuse
category: fix
release: 0.2.0
targets:
- type: format
  id: vqa
  effect: changed
credit:
- OpenTS contributors
---

Movie teardown now waits for the renderer to advance through the device's queued output and one in-flight period before reusing its stream slot. The existing bounded wait remains in force; a slot stays reserved if output does not progress in time. This fixes stale movie audio leaking into later game or menu sounds without changing movie skip or completion behavior.
