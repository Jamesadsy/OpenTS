---
title: Separate Tiberian Sun and Firestorm saves
category: fix
release: 0.2.0
targets:
- type: format
  id: save-games
  effect: changed
---

Campaign and skirmish saves now use separate `Tiberian Sun` and `Firestorm` folders inside `Saved Games`. Manual, quick, and automatic save names can no longer collide across products. Existing files directly in `Saved Games` are preserved but no longer appear in either product's load list; copy a known legacy save to its product folder to load it.
