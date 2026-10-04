---
title: Browser product-menu actions open native screens
category: fix
release: 0.2.0
targets:
- type: system
  id: native-menu-input
  effect: changed
credit:
- Jamesadsy
---

Campaign/New Game, Options, and local Skirmish previously returned to the product menu before their native screens could stay open in the browser. The browser now resolves the shipped UI files through the engine's native file search, so those screens remain interactive across frames; backing out returns to the selected product page, and accepting a campaign or local Skirmish continues to scenario startup.
