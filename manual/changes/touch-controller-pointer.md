---
title: Keep the game pointer available for touch and controllers
category: feature
release: 0.2.0
targets: []
credit: [OpenTS contributors]
---

Touch now positions the game's contextual pointer and keeps it visible across the front end, dialogs, setup screens, sidebar and tactical play. Completing a touch gesture leaves the pointer at the interaction point instead of visibly parking it at the window centre.

An SDL3 gamepad can move the same free pointer with its left stick, hold the primary or secondary mouse button, send Escape from Start, hold Control or Alt, navigate with the D-pad, and pan the tactical camera independently with the right stick. R2 boosts pointer speed; unsupported Square, Triangle, stick-click and Back actions remain inert.
