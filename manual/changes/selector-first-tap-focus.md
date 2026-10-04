---
title: Select a menu item on the first touch
category: fix
release: 0.2.0
targets:
- type: system
  id: native-menu-input
  effect: added
credit:
- OpenTS contributors
---

The product selector becomes interactive when its persistent background finishes playing. A touch retains its origin through native button-up until the input queue consumes the click, so a later browser focus loss can be repaired before the initial background wait or selector hit testing. TS, Firestorm, and Exit use the existing native results.
