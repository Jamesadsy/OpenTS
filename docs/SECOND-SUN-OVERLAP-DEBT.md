# Second Sun upstream-overlap debt

## PR #153 — portable monotonic timing and system-timer persistence

- **Why required at PIN B:** frozen PIN-SS-OPENTS-B retains Windows
  `timeGetTime` common callers and serializes generic timer `Started` values.
  A process-relative monotonic epoch cannot cross a save, exit, relaunch, and
  load without rebasing.
- **Local commit:** `b78182a702505cb2fb88a4803f71c37b490b4b35`.
- **Removal checkpoint:** a Director-authorized post-G2 upstream checkpoint,
  after PR #153 has landed and its accepted source can be compared against the
  local behavior and tests. Remove a redundant local delta only in that
  reconciliation work.
- **Equivalence:** one portable monotonic elapsed-time service; unchanged
  system timer scale; process-origin timer rebase; frame-origin timer
  preservation; and no Windows clock dependency in common code.
- **Difference:** this implementation consolidates the existing multiplayer
  load countdown onto the common service, keeps `timeBeginPeriod` and
  `timeEndPeriod` as Windows host scheduler policy, and adds a controllable
  test-clock seam. It does not import PR #153's logging changes or copy its
  patch.
