---
title: Find iOS LAN games with Second Sun Bonjour discovery
category: feature
release: 0.2.0
targets:
- type: system
  id: local-lan-l0
  effect: added
credit:
- Jamesadsy
---

On iOS, Network multiplayer can find same-Wi-Fi hosts with the specific `_secondsun._udp` Bonjour service, or connect to a manually entered private IPv4 address. Both routes feed the existing OpenTS UDP direct-peer transport at port 1234; network messages and the existing multiplayer screens remain unchanged. The iOS browse starts after the player opens Network, and a host advertises after creating a game.
