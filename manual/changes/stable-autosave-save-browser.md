---
title: Stable autosaves and save browser navigation
category: fix
release: 0.2.0
targets:
- type: format
  id: save-games
  effect: changed
credit:
- Jamesadsy
---

A successful load or save makes that exact filename the Save screen's current selection for the session; if it is no longer listed, or a new game starts, Save opens on [EMPTY SLOT]. Campaign autosaves now overwrite a stable filename derived from the scenario filename, and skirmish autosaves follow the active map filename; existing numbered autosaves remain available. Up and Down move through Save, Load and Delete rows, wrap at either end and keep the selected row visible; on Save, the description follows the highlighted row.
