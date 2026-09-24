---
title: Keep manual overwrites, autosaves and sidebar feedback aligned
category: fix
release: 0.2.0
targets:
- type: format
  id: save-games
  effect: changed
- type: system
  id: sidebar
  effect: changed
credit: [OpenTS contributors]
---

Saving over a listed game updates the selected save file by its stored filename, even when another save has the same description. Tiberian Sun and Firestorm each keep independent five-slot campaign and skirmish autosave rings, and every later save rotates over the next slot. The Repair and Sell sidebar buttons now follow the active native mode, including controller cycling and cancellation. The iOS app uses the approved OpenTS CABAL/Tiberium artwork for its home-screen icon.
