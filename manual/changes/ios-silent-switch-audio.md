---
title: Respect the iPhone Ring/Silent switch for game audio
category: fix
release: 0.2.0
targets:
- type: system
  id: sound-effects
  effect: changed
- type: system
  id: music
  effect: changed
- type: system
  id: eva-speech
  effect: changed
- type: system
  id: multiplayer-movies
  effect: changed
credit:
- Jamesadsy
---

Game audio continued while an iPhone was in Silent mode. On iPhone, music, sound effects, speech and movie audio now follow the Ring/Silent switch, while iOS continues to select the available speaker or connected headphone route.
