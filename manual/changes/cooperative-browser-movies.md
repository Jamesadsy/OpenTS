---
title: Resume browser movies between native Steps
category: fix
release: 0.2.0
targets:
- type: format
  id: vqa
  effect: changed
credit:
- OpenTS contributors
---

Browser startup and scenario movies retain playback state between native Steps, allowing browser input and presentation to continue. Movie order, decoder timing and skip handling are preserved. The desktop playback adapter continues to wait for the movie to end.
