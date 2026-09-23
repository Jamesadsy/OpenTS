---
title: Keep the game pointer available for touch and controllers
category: feature
release: 0.2.0
targets:
- type: key
  id: ScrollRate
  effect: changed
credit: [OpenTS contributors]
---

Touch now positions the game's contextual pointer and keeps it visible across the front end, dialogs, setup screens, sidebar and tactical play. Completing a touch gesture leaves the pointer at the interaction point instead of visibly parking it at the window centre.

Over the tactical map, the cursor follows the action under the pointer. It marks selectable friendly objects, valid movement, attacks, deploys, repairs and sales only when the game resolves those actions; Repair and Sell cursors update as the pointer moves between valid and invalid targets.

An SDL3 gamepad can move the same free pointer with its left stick, hold the primary or secondary mouse button, send Escape from Start, hold Control or Alt, navigate with the D-pad, and pan the tactical camera with the right stick. The Game Controls dialog offers eight `ScrollRate` positions, and right-stick pan scales from 1.0 at 0 to 0.5 at 7; touch pan keeps its existing speed. R2 boosts only pointer speed, and unsupported Square, Triangle, stick-click and Back actions remain inert.
