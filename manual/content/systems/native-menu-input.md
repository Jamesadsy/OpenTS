---
title: Native menu input
summary: "Routes input through the game's existing menu hit tests and actions."
category: interface-controls
keys: []
---

Graphic menus test the pointer against each enabled item's active rectangle. Mouse clicks and Return or shortcut input use the existing GraphicMenu item path. A finished persistent background remains registered while the menu becomes interactive; registration alone does not mean it is still playing.

On the browser touch route, a tap waits for the pointer to settle and retains its touch origin after finger-up and native button-up, until the native input queue consumes or clears the click. Native dispatch restores activation and set-focus before routing that click. If a later browser focus-loss message is consumed afterward, GraphicMenu restores focus while the click remains pending, during the initial background wait or before interactive hit testing. Tiberian Sun, Firestorm, and Exit continue through their existing NewMenu results.

In the browser runtime, Campaign/New Game, Options, and local Skirmish enter their existing native screen continuations. Each screen yields across browser frames and returns to the selected product menu when backed out; accepting Campaign or local Skirmish continues through native scenario preparation.
