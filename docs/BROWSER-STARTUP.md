# Cooperative browser startup

`OpenTS_Browser::Step` owns the process lifetime. Phase 3 is `MainGame`.
Its first Step calls `OpenTS_Browser_Startup_Begin_MainGame`, which creates a
`MainGameSession` and its `InitGameSession` without entering a movie lifetime.
Later callbacks increment `MainGameSteps` and step that retained initialization.

`InitGameSession` runs preparation once, then the applicable EVA, Westwood logo,
and title movies in their original order. It owns one `TitleMovieSession` at a
time. Only after all three movie slots finish does it run the remaining
initialization and let `MainGameSession` enter `SelectGameSession`.

## Movie lifetime

`TitleMovieSession` owns the filename, `MovieSkip::Playback`, movie handle,
mouse hiding, and cleanup. Its Step services input and skip, then calls
`Movie_Advance_Cooperative_Frame` once. The fullscreen decoder uses
`VQAClass::Advance_Cooperative_Playback`, with per-instance focus/pause state.
It calls `VQA_Play(..., VQAMODE_WALK, ...)` at most once per slice. The existing
decoder controls timing, buffer availability, sound synchronization, and end
of playback. A timing wait is not completion.

Each pass uses the same stop-frame preparation as legacy `Play_VQA`. Loop
options and iterations remain in the movie owner, and each completed pass
resets the decoder before the next pass. Skip ends the retained movie through
the existing `MovieSkip` policy. Cleanup is idempotent and clears `CurrentVQ`
before destroying the handle. Native `Play_Movie` / `Movie_Play` / `Play_VQA`
remain the blocking adapter for desktop callers.

## Campaign continuation

The browser supplies the optional preparation callbacks on `MainGameSession`.
After selection, they own a `ScenarioStartSession`: side intro, scenario load,
intro movie, briefing movie or briefing text, optional dropship selection,
action movie, and scenario activation. The same movie owner handles these
movies. `RestateMissionSession` retains text pages, animations and input state;
`DropshipSession` retains the original loadout screen state and advances one
iteration of its presentation per Step. Native adapters continue to wait for
these screens. Shared helpers preserve random initialization, difficulty,
scenario finalization, initial presentation and gameplay setup order.

The existing `MainLoopSession` starts only after preparation completes. These
continuations do not run gameplay simulation while a movie or modal child is
active, and do not add a scheduler or event loop.

## Bounded diagnostics

The existing native-operation callback still feeds the host freeze recorder.
It retains the existing run/build identity, last Step and capped operation
trace. The startup/movie markers are:

| Marker | Operation |
| --- | --- |
| 180 | Cooperative movie session begins |
| 181 | Playback slice enters |
| 182 | Slice returns without a new decoded frame |
| 183 | Slice advances a decoded frame |
| 184 | Decoder completes naturally |
| 185 | Existing skip policy accepts dismissal |
| 186 | Once-only movie cleanup |
| 187 | Startup resumes after its movies |
| 188 | Selected scenario preparation begins |
| 189 | Scenario continuation enters |
| 190 | Initial presentation and gameplay setup |
| 192–202 | Scenario subphases, below |

Scenario markers are 192 prepare/side intro, 193 read scenario, 194 intro,
195 briefing movie, 196 choose briefing-text fallback, 197 briefing-text
slice, 198 begin dropship, 199 dropship slice, 200 action movie, 201 movie
slice, and 202 finalization. Movie markers provide the more specific boundary
within a movie slice. Existing legacy markers, including 134, remain intact.

`MainGameSessionContract` links the real process, MainGame and initialization
coordinators with a synthetic decoder clock and movie operations. It checks
bounded Steps, retained identity, order, skip, unavailable media, focus,
timing waits, abort, cleanup, continuation and the preparation/gameplay handoff.
`VqaCooperativeState` is the same slice/focus policy used by the real decoder.
Owner-media playback, visible presentation and touch/audio acceptance still
require a device run; these synthetic tests do not assert those outcomes.
