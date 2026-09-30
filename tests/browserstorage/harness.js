(function () {
  "use strict";

  var output = document.getElementById("proof");
  var runtimeStatus = document.getElementById("runtime-status");
  var runtimeHeading = document.getElementById("runtime-heading");
  var runtimeCustody = document.getElementById("runtime-custody");
  var runtimeDiagnostics = document.getElementById("runtime-diagnostics");
  var storage = window.OpenTS124HStorage;
  var stateKey = "opents-124h-proof-state-v1";
  var pageUrl = new URL(location.href);
  var ownerMode = pageUrl.searchParams.get("profile") === "ts-import";
  var stage = pageUrl.searchParams.get("stage") || "write";
  var callbackCount = 0;
  var returnedCalls = 0;
  var heartbeatCount = 0;
  var phaseProgression = [];
  var pathSamples = [];
  var runtimeErrors = [];
  var consoleErrors = [];
  var heartbeatTimer = 0;
  var timeoutTimer = 0;
  var finished = false;
  var startTime = 0;
  var routeStartedAt = performance.now();
  var maxStepMs = 0;
  var importedOwnerDataSummary = null;
  var ownerTimeline = [];
  var ownerInputEvents = [];
  var ownerLifecycleEvents = [];
  var ownerPhaseKey = "";
  var ownerLastSnapshot = null;
  var ownerStableMenuFrames = 0;
  var ownerFrameSamples = [];
  var ownerBootstrapTransitions = [];
  var ownerLastRafAt = 0;
  var ownerLastBootstrapStage = null;
  var ownerLastMenuState = null;
  var ownerMenuHitBaseline = 0;
  var ownerMenuMouseDownBaseline = 0;
  var ownerAudioGestures = [];
  var ownerAudioAtSelectorEntry = null;
  var ownerFinished = false;
  var nativeFinishCalled = false;
  var state = readState();
  if (ownerMode) {
    state = {};
    sessionStorage.removeItem(stateKey);
  } else if (stage === "write") {
    state = {};
    sessionStorage.removeItem(stateKey);
  }

  function ownerDiagnostics() {
    if (!ownerMode) return null;
    var resourceTimings = performance.getEntriesByType("resource")
      .filter(function (entry) { return /OpenTS\.(?:js|wasm)(?:$|\?)/i.test(entry.name); })
      .map(function (entry) {
        return { resource: entry.name.split("/").pop(), startMs: Number(entry.startTime.toFixed(1)),
          durationMs: Number(entry.duration.toFixed(1)), transferBytes: entry.transferSize || null };
      });
    return {
      runtime: "Tiberian Sun development runtime",
      ownerDataCustody: "installed OPFS generation is read-only; /userdata remains local",
      elapsedMs: Number((performance.now() - routeStartedAt).toFixed(1)),
      routeStartedAt: new Date(performance.timeOrigin).toISOString(),
      timeline: ownerTimeline,
      latestNativeState: ownerLastSnapshot,
      framePump: {
        scheduledPolicy: "one native OpenTS_Browser_Step per requestAnimationFrame callback",
        callbackCount: callbackCount,
        returnedStepCount: returnedCalls,
        latestSamples: ownerFrameSamples.slice(-120),
        bootstrapTransitions: ownerBootstrapTransitions.slice(-64),
        nativeStepEntryMs: nativeValue("OpenTS_Browser_Step_Entry_Ms"),
        nativeStepExitMs: nativeValue("OpenTS_Browser_Step_Exit_Ms"),
        nativeStepResult: nativeValue("OpenTS_Browser_Last_Step_Result"),
        processPhase: nativeValue("OpenTS_Browser_Status"),
        mainGameSteps: nativeValue("OpenTS_Browser_MainGame_Steps"),
        hostSleepCallsInsideBrowserStep: nativeTelemetryCounter(16)
      },
      inputPath: ownerInputTelemetry(),
      audio: ownerAudioTelemetry(),
      audioAtSelectorEntry: ownerAudioAtSelectorEntry,
      trustedAudioGestures: ownerAudioGestures.slice(-16),
      input: ownerInputEvents.slice(-24),
      lifecycle: ownerLifecycleEvents.slice(-16),
      canvas: window.OpenTSRuntimeInput && window.Module && window.Module.canvas
        ? window.OpenTSRuntimeInput.measure(window.Module.canvas, document.getElementById("runtime-stage")) : null,
      runtimeResources: resourceTimings,
      importedGeneration: importedOwnerDataSummary ? {
        generation: importedOwnerDataSummary.generation,
        fileCount: importedOwnerDataSummary.fileCount,
        totalBytes: importedOwnerDataSummary.totalBytes,
        copiedOwnerData: importedOwnerDataSummary.copiedOwnerData
      } : null,
      runtimeErrors: runtimeErrors.slice(),
      consoleErrors: consoleErrors.slice()
    };
  }

  function refreshOwnerDiagnostics() {
    if (!ownerMode || !runtimeDiagnostics.open) return;
    output.textContent = JSON.stringify(ownerDiagnostics(), null, 2);
  }

  function setOwnerPhase(key, label, detail) {
    if (!ownerMode) return;
    if (runtimeStatus) runtimeStatus.textContent = label;
    if (document.body) document.body.dataset.runtime = key === "failed" ? "failed" : "running";
    if (key !== ownerPhaseKey) {
      ownerPhaseKey = key;
      ownerTimeline.push({ phase: key, status: label,
        elapsedMs: Number((performance.now() - routeStartedAt).toFixed(1)), detail: detail || null });
      if (window.parent !== window) {
        try {
          window.parent.postMessage({ type: "vv-ts-runtime-phase", phase: key, status: label,
            elapsedMs: Number((performance.now() - routeStartedAt).toFixed(1)) }, location.origin);
        } catch (_) { /* The visible in-frame status does not depend on parent messaging. */ }
      }
    }
    refreshOwnerDiagnostics();
  }

  if (ownerMode) {
    document.body.classList.add("owner-runtime");
    document.title = "Tiberian Sun development runtime";
    runtimeHeading.textContent = "Tiberian Sun development runtime";
    runtimeCustody.textContent = "The installed TS generation stays in this browser’s local OPFS. OpenTS mounts it read-only and keeps runtime user data local.";
    setOwnerPhase("module-loading", "Loading OpenTS WebAssembly module…");
    var ownerCanvas = document.getElementById("canvas");
    var stagePresentation = document.documentElement.dataset.presentation === "stage";
    var inputBridge = window.OpenTSRuntimeInput.install(ownerCanvas, function (entry) {
      ownerInputEvents.push({ elapsedMs: Number((performance.now() - routeStartedAt).toFixed(1)), event: entry });
      if (ownerInputEvents.length > 64) ownerInputEvents.shift();
      refreshOwnerDiagnostics();
    }, {
      fitToStage: stagePresentation,
      stage: document.getElementById("runtime-stage"),
      onTrustedGesture: resumeExistingAudioFromGesture
    });
    window.OpenTSRuntimeDiagnostics = function () {
      return ownerDiagnostics();
    };
    function recordLifecycle(name, detail) {
      ownerLifecycleEvents.push({
        type: name,
        elapsedMs: Number((performance.now() - routeStartedAt).toFixed(1)),
        visibility: document.visibilityState,
        persisted: detail && typeof detail.persisted === "boolean" ? detail.persisted : null,
        contextLost: name.indexOf("webglcontext") === 0 && detail && detail.target
          ? Boolean(detail.target.isContextLost && detail.target.isContextLost()) : null
      });
      if (ownerLifecycleEvents.length > 32) ownerLifecycleEvents.shift();
      refreshOwnerDiagnostics();
    }
    function restoreOwnerDisplay() {
      inputBridge.fit();
      try { ownerCanvas.focus({ preventScroll: true }); }
      catch (_) { try { ownerCanvas.focus(); } catch (_) { /* iOS may require a user gesture. */ } }
      ownerStableMenuFrames = 0;
      ownerLastSnapshot = null;
      setOwnerPhase("display-resuming", "Restoring the OpenTS display after returning…");
    }
    document.addEventListener("visibilitychange", function () {
      recordLifecycle("visibilitychange", null);
      if (document.visibilityState === "visible") restoreOwnerDisplay();
    });
    window.addEventListener("pageshow", function (event) {
      recordLifecycle("pageshow", event);
      if (event.persisted) restoreOwnerDisplay();
    });
    window.addEventListener("pagehide", function (event) { recordLifecycle("pagehide", event); });
    ownerCanvas.addEventListener("webglcontextlost", function (event) {
      event.preventDefault();
      recordLifecycle("webglcontextlost", event);
      setOwnerPhase("display-context-lost", "The OpenTS display context was lost; waiting for the browser to restore it…");
    });
    ownerCanvas.addEventListener("webglcontextrestored", function (event) {
      recordLifecycle("webglcontextrestored", event);
      restoreOwnerDisplay();
    });
    runtimeDiagnostics.addEventListener("toggle", refreshOwnerDiagnostics);
    window.addEventListener("message", function (event) {
      var message = event.data;
      if (event.origin !== location.origin || event.source !== window.parent || !message
        || message.type !== "vv-ts-native-key" || typeof message.down !== "boolean") return;
      window.OpenTSRuntimeInput.dispatchKey(ownerCanvas, message.key, message.down);
    });
  } else {
    document.body.classList.add("proof-mode");
    document.title = "OpenTS 124H local storage proof";
    runtimeHeading.textContent = "OpenTS 124H local storage proof";
    runtimeCustody.textContent = "Synthetic browser storage fixture. No owner game data is used.";
    runtimeStatus.textContent = "Preparing synthetic local storage…";
    runtimeDiagnostics.open = true;
  }

  function textOf(value) {
    try { return typeof value === "string" ? value : JSON.stringify(value); }
    catch (_) { return String(value); }
  }

  function errorText(error) {
    if (error && typeof error.stack === "string" && error.stack.length > 0) return error.stack;
    if (error && typeof error.message === "string" && error.message.length > 0) return error.message;
    return textOf(error);
  }

  function readState() {
    try { return JSON.parse(sessionStorage.getItem(stateKey) || "{}"); }
    catch (_) { return {}; }
  }

  function saveState() {
    sessionStorage.setItem(stateKey, JSON.stringify(state));
  }

  function publish(result) {
    window.__VV_TS1A_124H_PROOF__ = result;
    output.textContent = JSON.stringify(result, null, 2);
    document.body.dataset.proof = result.classification === "PASS" ? "pass" : "fail";
    if (ownerMode) {
      setOwnerPhase(result.classification === "PASS" ? "complete" : "failed",
        result.classification === "PASS" ? "OpenTS runtime completed." : "OpenTS runtime stopped with an error.",
        result.reason || null);
      document.body.dataset.runtime = result.classification === "PASS" ? "ready" : "failed";
      document.title = "Tiberian Sun development runtime";
      return;
    }
    document.title = "OpenTS 124H local storage proof: " + result.classification;
    document.dispatchEvent(new CustomEvent(ownerMode ? "vv-ts1b-126-owner-runtime-proof" : "vv-ts1a-124h-proof", { detail: result }));
  }

  function check(condition, message) {
    if (!condition) throw new Error(message);
  }

  function nativeValue(name) {
    var fn = Module["_" + name];
    return typeof fn === "function" ? fn() : null;
  }

  function nativeText(name) {
    var pointer = nativeValue(name);
    return pointer && Module.UTF8ToString ? Module.UTF8ToString(pointer) : "";
  }

  function nativeTelemetryCounter(id) {
    var fn = Module["_OpenTS_Browser_Telemetry_Counter"];
    return typeof fn === "function" ? fn(id) : null;
  }

  function nativeTelemetryValue(id) {
    var fn = Module["_OpenTS_Browser_Telemetry_Value"];
    return typeof fn === "function" ? fn(id) : null;
  }

  function audioContextSnapshot() {
    var devices = window.miniaudio && Array.isArray(window.miniaudio.devices)
      ? window.miniaudio.devices : [];
    return devices.map(function (device, index) {
      if (!device) return null;
      return {
        index: index,
        deviceState: device.state,
        contextState: device.webaudio ? device.webaudio.state : "unavailable",
        sampleRate: device.webaudio ? device.webaudio.sampleRate : null
      };
    }).filter(Boolean);
  }

  function ownerAudioTelemetry() {
    return {
      themeRequested: nativeText("OpenTS_Browser_Theme_Name"),
      themeRequestObserved: nativeTelemetryCounter(17) > 0,
      audioEngineAvailable: nativeValue("OpenTS_Browser_Audio_Available"),
      audioDeviceAvailable: nativeValue("OpenTS_Browser_Audio_Device_Available"),
      audioDeviceOpen: nativeValue("OpenTS_Browser_Audio_Device_Open"),
      audioDeviceRunning: nativeValue("OpenTS_Browser_Audio_Device_Running"),
      feederThreadRunning: nativeValue("OpenTS_Browser_Audio_Feeder_Running"),
      cooperativeFeederPasses: nativeValue("OpenTS_Browser_Audio_Feeder_Passes"),
      childAudioContexts: audioContextSnapshot(),
      sdlResume: "not applicable: this browser OpenTS build uses the existing miniaudio device path"
    };
  }

  function ownerInputTelemetry() {
    var counterNames = ["sdlFingerDown", "sdlFingerMotion", "sdlFingerUp", "sdlFingerCanceled",
      "recognizerAccepted", "recognizerRejected", "touchServiceRuns", "activateAppPosted",
      "setFocusPosted", "activateAppConsumed", "setFocusConsumed", "wmMouseMove",
      "wmLButtonDown", "wmLButtonUp", "menuCandidateChecks", "menuActiveRectHits", "hostSleep",
      "themeRequests", "productChoicesApplied"];
    var values = [];
    for (var i = 0; i < counterNames.length; i++) values.push(nativeTelemetryCounter(i));
    return {
      counters: counterNames.reduce(function (result, name, index) {
        result[name] = values[index];
        return result;
      }, {}),
      nativeInputOrdering: (function () {
        var activate = nativeTelemetryValue(17) || 0;
        var setFocus = nativeTelemetryValue(18) || 0;
        var down = nativeTelemetryValue(19) || 0;
        var route = nativeTelemetryValue(21) || 0;
        var hit = nativeTelemetryValue(22) || 0;
        var choice = nativeTelemetryValue(23) || 0;
        return {
          gameInFocusAtClickConsumption: nativeTelemetryValue(10),
          activationAndSetFocusBeforeClick: activate > 0 && setFocus > 0 && down > 0
            && activate < down && setFocus < down && nativeTelemetryValue(10) === 1,
          clickReachedGameFrameRoute: down > 0 && route >= down,
          gameFrameRouteReachedGraphicMenuActiveRect: route > 0 && hit > route,
          graphicMenuSelectionReachedNewMenuResult: hit > 0 && choice > hit,
          sequences: {
            activateAppConsumed: activate,
            setFocusConsumed: setFocus,
            lButtonDownConsumed: down,
            gameFrameRoute: route,
            graphicMenuActiveRectHit: hit,
            newMenuProductChoiceApplied: choice
          }
        };
      })(),
      last: {
        sdlEventType: nativeTelemetryValue(0),
        touchDeviceType: nativeTelemetryValue(1),
        sdlLogicalWindow: { width: nativeTelemetryValue(2), height: nativeTelemetryValue(3) },
        sdlDrawable: { width: nativeTelemetryValue(4), height: nativeTelemetryValue(5) },
        pixelDensity: (nativeTelemetryValue(6) || 0) / 1000,
        recognizerPhase: nativeTelemetryValue(7),
        touchWindowPoint: { x: (nativeTelemetryValue(8) || 0) / 1000, y: (nativeTelemetryValue(9) || 0) / 1000 },
        gameInFocusAtMouseMessage: nativeTelemetryValue(10),
        lastMouseMessage: nativeTelemetryValue(11),
        gameFramePoint: { x: nativeTelemetryValue(12), y: nativeTelemetryValue(13) },
        graphicMenuCandidateId: nativeTelemetryValue(14),
        graphicMenuActiveRectHit: nativeTelemetryValue(15),
        newMenuProductChoiceItem: nativeTelemetryValue(16),
        activationConsumedSequence: nativeTelemetryValue(17),
        setFocusConsumedSequence: nativeTelemetryValue(18),
        mouseDownConsumedSequence: nativeTelemetryValue(19),
        mouseUpConsumedSequence: nativeTelemetryValue(20),
        mouseRouteGameFrameSequence: nativeTelemetryValue(21),
        graphicMenuHitSequence: nativeTelemetryValue(22),
        newMenuProductChoiceSequence: nativeTelemetryValue(23)
      }
    };
  }

  function resumeExistingAudioFromGesture(event) {
    var before = audioContextSnapshot();
    var needsResume = before.some(function (device) { return device.contextState !== "running"; });
    var record = {
      eventType: event.type,
      isTrusted: event.isTrusted === true,
      elapsedMs: Number((performance.now() - routeStartedAt).toFixed(1)),
      stateBefore: before,
      action: needsResume && window.miniaudio && typeof window.miniaudio.unlock === "function"
        ? "requested existing miniaudio device contexts to resume" : "no suspended existing context to resume",
      unlockResult: "not-requested",
      sdlResume: "not applicable: SDL audio is not the browser audio backend"
    };
    if (needsResume && window.miniaudio && typeof window.miniaudio.unlock === "function") {
      try {
        var result = window.miniaudio.unlock();
        record.unlockResult = result === undefined ? "called; API returns no result" : textOf(result);
      } catch (error) {
        record.unlockResult = "threw: " + errorText(error);
      }
    }
    ownerAudioGestures.push(record);
    if (ownerAudioGestures.length > 16) ownerAudioGestures.shift();
    setTimeout(function () {
      record.stateAfter = audioContextSnapshot();
      record.elapsedAfterMs = Number((performance.now() - routeStartedAt).toFixed(1));
      refreshOwnerDiagnostics();
    }, 250);
    refreshOwnerDiagnostics();
  }

  function recordPhase(value) {
    if (phaseProgression.length === 0 || phaseProgression[phaseProgression.length - 1] !== value) {
      phaseProgression.push(value);
    }
  }

  function recordDataPath() {
    var value = nativeText("OpenTS_124H_Data_Root");
    if (value && pathSamples.indexOf(value) === -1) pathSamples.push(value);
  }

  function decodeHex(value) {
    var bytes = new Uint8Array(value.length / 2);
    for (var index = 0; index < bytes.length; index += 1) {
      bytes[index] = parseInt(value.slice(index * 2, index * 2 + 2), 16);
    }
    return bytes;
  }

  async function hashHex(value) {
    return storage.sha256(decodeHex(value));
  }

  function ownerRequests() {
    return storage.remoteDataRequests();
  }

  function finishProcess() {
    if (finished) return;
    finished = true;
    if (timeoutTimer) clearTimeout(timeoutTimer);
    var finishResult = nativeValue("OpenTS_Browser_Finish");
    var elapsedMs = performance.now() - startTime;
    state.runtimeErrors = (state.runtimeErrors || []).concat(runtimeErrors);
    state.consoleErrors = (state.consoleErrors || []).concat(consoleErrors);
    state.processStages = state.processStages || [];

    var current = {
      stage: stage,
      emscriptenMainReturned: nativeValue("OpenTS_Browser_Main_Returned") === 1,
      mainEntryReturned: nativeValue("OpenTS_Browser_Main_Returned") === 1,
      beginCount: nativeValue("OpenTS_Browser_Begin_Count"),
      beginResult: nativeValue("OpenTS_Browser_Begin_Result"),
      browserCallbacks: callbackCount,
      returnedCallbacks: returnedCalls,
      nativeProcessSteps: nativeValue("OpenTS_Browser_Step_Count"),
      phaseProgression: phaseProgression,
      waitFocusYields: nativeValue("OpenTS_Browser_WaitFocus_Yields"),
      mainGameBegan: nativeValue("OpenTS_Browser_MainGame_Began") === 1,
      mainGameSteps: nativeValue("OpenTS_Browser_MainGame_Steps"),
      firstNoOwnerDataFrontier: "code/init.cpp::Bootstrap before Init_Bootstrap_Mixfiles",
      boundaryProbePassed: nativeValue("OpenTS_124H_Boundary_Probe_Passed") === 1,
      boundaryProbeCount: nativeValue("OpenTS_124H_Boundary_Probe_Count"),
      sentinelSize: nativeValue("OpenTS_124H_Boundary_Sentinel_Size"),
      storageReadyAtBoundary: nativeValue("OpenTS_124H_Storage_Is_Ready") === 1,
      finishResult: finishResult,
      finishCount: nativeValue("OpenTS_Browser_Finish_Count"),
      finalProcessStatus: nativeValue("OpenTS_Browser_Status"),
      rendererInitialized: nativeValue("OpenTS_Browser_Renderer_Initialized") === 1,
      rendererName: nativeText("OpenTS_Browser_Renderer_Name"),
      canvasExists: document.getElementById("canvas") instanceof HTMLCanvasElement,
      webglContextAvailable: !!(document.getElementById("canvas").getContext("webgl2")
        || document.getElementById("canvas").getContext("webgl")),
      dataRoot: nativeText("OpenTS_124H_Data_Root"),
      dataPathSamples: pathSamples,
      dataPathStable: nativeValue("OpenTS_124H_Data_Root_Stable") === 1,
      dataProbePassed: nativeValue("OpenTS_124H_Data_Probe_Passed") === 1,
      rootEntries: nativeValue("OpenTS_124H_Root_Entry_Count"),
      nestedEntries: nativeValue("OpenTS_124H_Nested_Entry_Count"),
      heartbeatTicks: heartbeatCount,
      maxNativeStepMs: Number(maxStepMs.toFixed(3)),
      elapsedMs: Number(elapsedMs.toFixed(3)),
      runtimeErrors: runtimeErrors,
      consoleErrors: consoleErrors
    };
    state.processStages.push(current);
    saveState();

    window.setTimeout(async function () {
      if (stage === "write") {
        try {
          check(current.finishResult === 1 && current.finishCount === 1, "native Finish was not called exactly once");
          var writeResult = nativeValue("OpenTS_124H_User_Write");
          check(writeResult === 1, "native C++ synthetic User write failed");
          var localHex = nativeText("OpenTS_124H_User_Readback_Hex");
          var localSize = nativeValue("OpenTS_124H_User_Readback_Size");
          var expectedBytes = storage.expectedUserBytes();
          var expectedHex = storage.bytesToHex(expectedBytes);
          check(localSize === expectedBytes.length && localHex === expectedHex,
            "native User write did not read back the exact deterministic bytes");
          var expectedHash = await storage.sha256(expectedBytes);
          check(await hashHex(localHex) === expectedHash, "native User write hash mismatch");
          await storage.flushUser(Module);
          state.userWrite = {
            nativeWriteResult: writeResult,
            size: localSize,
            sha256: expectedHash,
            bytesMatch: true,
            explicitFlush: "PASS"
          };
          state.dataGeneration = state.dataGeneration || window.__VV124H_GENERATION__;
          state.remoteDataRequests = ownerRequests();
          state.reloadFrom = location.href;
          saveState();
          location.replace("?stage=readback");
        } catch (error) { fail(error); }
        return;
      }

      if (stage === "readback") {
        try {
          check(current.finishResult === 1 && current.finishCount === 1, "readback process Finish was not called exactly once");
          var readHex = nativeText("OpenTS_124H_User_Readback_Hex");
          var readSize = nativeValue("OpenTS_124H_User_Readback_Size");
          var expected = state.userWrite;
          check(!!expected && readSize === expected.size, "reloaded native User size mismatch");
          var readHash = await hashHex(readHex);
          check(readHash === expected.sha256, "reloaded native User hash mismatch");
          check(readHex === storage.bytesToHex(storage.expectedUserBytes()), "reloaded native User bytes mismatch");
          check(nativeValue("OpenTS_124H_User_Remove") === 1, "native synthetic User cleanup failed");
          await storage.flushUser(Module);
          state.userReload = {
            fullPageRuntimeReload: true,
            hydratedBeforeMain: true,
            nativeReadSize: readSize,
            nativeReadSha256: readHash,
            exactBytesMatch: true,
            cleanupAndFlush: "PASS"
          };
          state.remoteDataRequests = (state.remoteDataRequests || []).concat(ownerRequests());
          saveState();
          location.replace("?stage=failure");
        } catch (error) { fail(error); }
        return;
      }

      fail(new Error("unexpected successful process stage: " + stage));
    }, 0);
  }

  function ownerNativeSnapshot() {
    return {
      status: nativeValue("OpenTS_Browser_Status"),
      faultCode: nativeValue("OpenTS_Browser_Fault_Code"),
      bootstrapStage: nativeValue("OpenTS_Browser_Bootstrap_Stage"),
      rendererInitialized: nativeValue("OpenTS_Browser_Renderer_Initialized") === 1,
      rendererName: nativeText("OpenTS_Browser_Renderer_Name"),
      bootstrapMixfilesOpened: nativeValue("OpenTS_Browser_Bootstrap_Mixfiles_Opened") === 1,
      secondaryMixfilesOpened: nativeValue("OpenTS_Browser_Secondary_Mixfiles_Opened") === 1,
      frontendReady: nativeValue("OpenTS_Browser_Frontend_Ready") === 1,
      menuState: nativeValue("OpenTS_Browser_Menu_State"),
      selectedProduct: nativeValue("OpenTS_Browser_Selected_Product"),
      titleMediaAvailable: nativeValue("OpenTS_Browser_Title_Media_Available") === 1,
      titleMediaNatural: nativeValue("OpenTS_Browser_Title_Media_Natural") === 1,
      titleMediaSkipped: nativeValue("OpenTS_Browser_Title_Media_Skipped") === 1,
      beginCount: nativeValue("OpenTS_Browser_Begin_Count"),
      beginResult: nativeValue("OpenTS_Browser_Begin_Result"),
      mainGameSteps: nativeValue("OpenTS_Browser_MainGame_Steps"),
      finishCount: nativeValue("OpenTS_Browser_Finish_Count"),
      dataProbePassed: nativeValue("OpenTS_126_Data_Probe_Passed") === 1,
      ownerDataBoundaryReached: nativeValue("OpenTS_Browser_OwnerData_Boundary") === 1,
      dataRoot: nativeText("OpenTS_124H_Data_Root")
    };
  }

  function recordOwnerMilestone(key, label, detail) {
    if (ownerTimeline.some(function (item) { return item.milestone === key; })) return;
    ownerTimeline.push({ milestone: key, status: label,
      elapsedMs: Number((performance.now() - routeStartedAt).toFixed(1)), detail: detail || null });
    refreshOwnerDiagnostics();
  }

  function updateOwnerProgress(snapshot) {
    var previous = ownerLastSnapshot;
    ownerLastSnapshot = snapshot;
    recordPhase(snapshot.status);
    if (snapshot.status > 0) recordOwnerMilestone("native-start", "OpenTS native startup began.", snapshot.status);
    if (snapshot.rendererInitialized) {
      recordOwnerMilestone("renderer-ready", "OpenTS renderer initialized.", snapshot.rendererName);
    }
    if (snapshot.bootstrapMixfilesOpened) recordOwnerMilestone("bootstrap-mix", "Initial TS MIX bootstrap completed.");
    if (snapshot.secondaryMixfilesOpened) recordOwnerMilestone("secondary-mix", "Secondary TS MIX bootstrap completed.");
    if (snapshot.frontendReady) recordOwnerMilestone("newmenu-ready", "Native TS/Firestorm NewMenu became ready.");
    if (snapshot.titleMediaNatural) recordOwnerMilestone("title-natural", "Native title media completed naturally.");
    if (snapshot.titleMediaSkipped) recordOwnerMilestone("title-skipped", "Native title-media skip completed.");

    if (snapshot.menuState > 0 && ownerAudioAtSelectorEntry === null) {
      ownerAudioAtSelectorEntry = ownerAudioTelemetry();
      recordOwnerMilestone("selector-audio-state",
        "Audio engine, output device, existing child audio contexts, and requested theme sampled at selector entry.",
        ownerAudioAtSelectorEntry);
    }

    if (!previous || previous.menuState !== snapshot.menuState) {
      ownerLastMenuState = snapshot.menuState;
      ownerMenuHitBaseline = nativeTelemetryCounter(15) || 0;
      ownerMenuMouseDownBaseline = nativeTelemetryCounter(12) || 0;
    }

    if (!previous || previous.selectedProduct !== snapshot.selectedProduct) {
      ownerBootstrapTransitions.push({
        type: "selectedProduct-transition",
        elapsedMs: Number((performance.now() - routeStartedAt).toFixed(1)),
        from: previous ? previous.selectedProduct : null,
        to: snapshot.selectedProduct,
        nativeProductChoiceItem: nativeTelemetryValue(16),
        nativeProductChoiceCount: nativeTelemetryCounter(18),
        menuState: snapshot.menuState,
        processPhase: snapshot.status
      });
      if (ownerBootstrapTransitions.length > 64) ownerBootstrapTransitions.shift();
    }

    if (snapshot.menuState > 0 && previous && snapshot.menuState === previous.menuState) ownerStableMenuFrames += 1;
    else ownerStableMenuFrames = snapshot.menuState > 0 ? 1 : 0;

    if (snapshot.status === 7) {
      throw new Error("OpenTS native runtime fault " + snapshot.faultCode);
    }
    if (snapshot.status === 6) {
      finishOwnerRuntime(snapshot);
      return;
    }

    if (snapshot.status === 1) {
      var stage = snapshot.bootstrapStage || 0;
      setOwnerPhase("native-bootstrap", stage >= 12
        ? "Starting the OpenTS renderer and Tiberian Sun runtime…"
        : "Initializing OpenTS runtime services…", { bootstrapStage: stage });
    } else if (snapshot.status === 2) {
      setOwnerPhase("waiting-for-focus", "Waiting for the OpenTS display to become active…");
    } else if (snapshot.status === 3 && snapshot.menuState === 1) {
      var selectorFocused = nativeTelemetryValue(10) === 1;
      var selectorHit = selectorFocused
        && (nativeTelemetryCounter(12) || 0) > ownerMenuMouseDownBaseline
        && (nativeTelemetryCounter(15) || 0) > ownerMenuHitBaseline;
      var selectorArmed = selectorFocused
        && (nativeTelemetryCounter(9) || 0) > 0
        && (nativeTelemetryCounter(10) || 0) > 0;
      var selectorPhase = selectorHit ? "selector-interactive"
        : selectorArmed ? "input-path-armed" : "selector-visible";
      var selectorStatus = selectorHit
        ? "Native focus and a mouse activation reached a GraphicMenu ActiveRect."
        : selectorArmed
          ? "Selector visible; native focus and touch routing are armed. Tap a product."
          : "Selector visible; waiting for native input-path evidence.";
      setOwnerPhase(selectorPhase, selectorStatus, {
        menuState: snapshot.menuState,
        stableFrames: ownerStableMenuFrames,
        nativeHitCount: nativeTelemetryCounter(15),
        mouseDownCount: nativeTelemetryCounter(12),
        gameInFocus: nativeTelemetryValue(10)
      });
      if (ownerStableMenuFrames >= 8) recordOwnerMilestone("first-stable-menu",
        "TS / Firestorm selector rendered across eight native Step samples; hit-path interactivity remains unproven.");
    } else if (snapshot.status === 3 && (snapshot.menuState === 2 || snapshot.menuState === 3)) {
      var productName = snapshot.menuState === 2 ? "Tiberian Sun" : "Firestorm";
      var productMenuHit = nativeTelemetryValue(10) === 1
        && (nativeTelemetryCounter(12) || 0) > ownerMenuMouseDownBaseline
        && (nativeTelemetryCounter(15) || 0) > ownerMenuHitBaseline;
      setOwnerPhase(productMenuHit ? "product-menu-interactive" : "product-menu-visible",
        productMenuHit
          ? productName + " menu received a focused native ActiveRect hit."
          : productName + " menu is visible; waiting for a focused native hit.",
        { menuState: snapshot.menuState, stableFrames: ownerStableMenuFrames,
          nativeHitCount: nativeTelemetryCounter(15), mouseDownCount: nativeTelemetryCounter(12),
          gameInFocus: nativeTelemetryValue(10) });
      if (productMenuHit) recordOwnerMilestone("selected-product-menu",
        productName + " menu received a native ActiveRect hit.");
    } else if (snapshot.status === 3 && snapshot.selectedProduct >= 0) {
      var selectedName = snapshot.selectedProduct === 0 ? "Tiberian Sun" : "Firestorm";
      setOwnerPhase("title-and-menu-loading", selectedName + " selected; native title and menu startup is in progress…",
        { selectedProduct: snapshot.selectedProduct, titleMediaAvailable: snapshot.titleMediaAvailable });
      if (!previous || previous.selectedProduct !== snapshot.selectedProduct) {
        recordOwnerMilestone("product-selected", selectedName + " product selection accepted by native NewMenu.");
        recordOwnerMilestone("title-media-stage", "Native title-media stage began.", selectedName);
      }
    } else if (snapshot.status === 3 && snapshot.frontendReady) {
      setOwnerPhase("newmenu-starting", "Preparing the native TS / Firestorm selector…");
    } else if (snapshot.status === 3 && snapshot.secondaryMixfilesOpened) {
      setOwnerPhase("menu-assets", "Tiberian Sun menu data loaded; initializing the native selector…");
    } else if (snapshot.status === 3 && snapshot.bootstrapMixfilesOpened) {
      setOwnerPhase("mix-bootstrap", "Loading Tiberian Sun menu and title data…");
    } else {
      setOwnerPhase("engine-starting", "Starting the native Tiberian Sun frontend…", { status: snapshot.status });
    }
    refreshOwnerDiagnostics();
  }

  function finishOwnerRuntime(snapshot) {
    if (ownerFinished) return;
    ownerFinished = true;
    finished = true;
    var finishCount = nativeValue("OpenTS_Browser_Finish_Count");
    var finishResult = null;
    if (finishCount === 0 && !nativeFinishCalled) {
      nativeFinishCalled = true;
      finishResult = nativeValue("OpenTS_Browser_Finish");
    }
    setOwnerPhase("runtime-exited", "The OpenTS runtime has exited.", {
      finishResult: finishResult, status: snapshot.status, faultCode: snapshot.faultCode
    });
    document.body.dataset.runtime = "ready";
    refreshOwnerDiagnostics();
  }

  function fail(error) {
    if (finished) return;
    finished = true;
    if (timeoutTimer) clearTimeout(timeoutTimer);
    runtimeErrors.push(textOf(error && error.stack ? error.stack : error));
    state.runtimeErrors = (state.runtimeErrors || []).concat(runtimeErrors);
    state.consoleErrors = (state.consoleErrors || []).concat(consoleErrors);
    state.failure = errorText(error);
    if (ownerMode) {
      var beginCount = nativeValue("OpenTS_Browser_Begin_Count");
      var finishCount = nativeValue("OpenTS_Browser_Finish_Count");
      if (beginCount === 1 && finishCount === 0 && !nativeFinishCalled) {
        nativeFinishCalled = true;
        nativeValue("OpenTS_Browser_Finish");
      }
      publish({
        classification: "FAIL",
        reason: errorText(error),
        dataSource: "the installed Vanilla Vault TS OPFS generation",
        copiedOwnerData: false,
        importedFileCount: importedOwnerDataSummary ? importedOwnerDataSummary.fileCount : null,
        importedBytes: importedOwnerDataSummary ? importedOwnerDataSummary.totalBytes : null,
        nativeState: ownerLastSnapshot || ownerNativeSnapshot(),
        browserCallbacks: callbackCount,
        returnedCallbacks: returnedCalls,
        nativeProcessSteps: nativeValue("OpenTS_Browser_Step_Count"),
        framePump: ownerDiagnostics().framePump,
        inputPath: ownerDiagnostics().inputPath,
        audio: ownerDiagnostics().audio,
        trustedAudioGestures: ownerAudioGestures.slice(-16),
        ownerDataRequests: ownerRequests(),
        runtimeErrors: runtimeErrors.slice(),
        consoleErrors: consoleErrors.slice(),
        ownerTimeline: ownerTimeline
      });
    } else {
      saveState();
      publish({ classification: "FAIL", stage: stage, reason: errorText(error), state: state });
    }
  }

  function onFrame() {
    if (ownerMode) {
      if (finished) return;
      callbackCount += 1;
      var ownerCallbackEntry = performance.now();
      var ownerInterRafDelta = ownerLastRafAt > 0 ? ownerCallbackEntry - ownerLastRafAt : null;
      ownerLastRafAt = ownerCallbackEntry;
      var ownerPhaseBefore = nativeValue("OpenTS_Browser_Status");
      var ownerStageBefore = nativeValue("OpenTS_Browser_Bootstrap_Stage");
      var ownerStepStarted = performance.now();
      var ownerStepResult = null;
      try {
        ownerStepResult = nativeValue("OpenTS_Browser_Step");
        returnedCalls += 1;
        var ownerCallbackExit = performance.now();
        var ownerStepDuration = ownerCallbackExit - ownerStepStarted;
        maxStepMs = Math.max(maxStepMs, ownerStepDuration);
        var ownerPhase = nativeValue("OpenTS_Browser_Status");
        var ownerStageAfter = nativeValue("OpenTS_Browser_Bootstrap_Stage");
        var nativeEntryMs = nativeValue("OpenTS_Browser_Step_Entry_Ms");
        var nativeExitMs = nativeValue("OpenTS_Browser_Step_Exit_Ms");
        var stepSample = {
          callback: callbackCount,
          callbackElapsedMs: Number((ownerCallbackEntry - routeStartedAt).toFixed(1)),
          interRafDeltaMs: ownerInterRafDelta === null ? null : Number(ownerInterRafDelta.toFixed(1)),
          jsStepEntryMs: Number(ownerStepStarted.toFixed(1)),
          jsStepExitMs: Number(ownerCallbackExit.toFixed(1)),
          jsStepDurationMs: Number(ownerStepDuration.toFixed(2)),
          nativeStepEntryMs: Number(nativeEntryMs),
          nativeStepExitMs: Number(nativeExitMs),
          nativeStepDurationMs: Number((nativeExitMs - nativeEntryMs).toFixed(2)),
          cooperativeResult: ownerStepResult,
          nativeCooperativeResult: nativeValue("OpenTS_Browser_Last_Step_Result"),
          processPhaseBefore: ownerPhaseBefore,
          processPhaseAfter: ownerPhase,
          bootstrapStageBefore: ownerStageBefore,
          bootstrapStageAfter: ownerStageAfter,
          stepCount: nativeValue("OpenTS_Browser_Step_Count"),
          mainGameStepCount: nativeValue("OpenTS_Browser_MainGame_Steps"),
          hostSleepCallsDuringBrowserSteps: nativeTelemetryCounter(16)
        };
        ownerFrameSamples.push(stepSample);
        if (ownerFrameSamples.length > 120) ownerFrameSamples.shift();
        if (ownerStageBefore !== ownerStageAfter || ownerLastBootstrapStage === null) {
          ownerBootstrapTransitions.push({
            type: "bootstrap-stage-transition",
            elapsedMs: Number((performance.now() - routeStartedAt).toFixed(1)),
            from: ownerLastBootstrapStage,
            stageBefore: ownerStageBefore,
            stageAfter: ownerStageAfter,
            processPhase: ownerPhase,
            stepCount: nativeValue("OpenTS_Browser_Step_Count"),
            mainGameSteps: nativeValue("OpenTS_Browser_MainGame_Steps")
          });
          if (ownerBootstrapTransitions.length > 64) ownerBootstrapTransitions.shift();
          ownerLastBootstrapStage = ownerStageAfter;
        }
        recordPhase(ownerPhase);
        if (ownerStepResult === 3 || ownerPhase === 7) {
          throw new Error("OpenTS native runtime fault " + nativeValue("OpenTS_Browser_Fault_Code"));
        }
        if ((ownerStepResult !== 1 && ownerStepResult !== 2) || ownerPhase < 0 || ownerPhase > 7) {
          throw new Error("Unexpected OpenTS runtime Step result/state: " + ownerStepResult + "/" + ownerPhase);
        }
        updateOwnerProgress(ownerNativeSnapshot());
      } catch (error) {
        fail(error);
        return;
      }
      if (!finished) requestAnimationFrame(onFrame);
      return;
    }
    if (finished) return;
    callbackCount += 1;
    var started = performance.now();
    try {
      recordPhase(nativeValue("OpenTS_Browser_Status"));
      var result = nativeValue("OpenTS_Browser_Step");
      returnedCalls += 1;
      maxStepMs = Math.max(maxStepMs, performance.now() - started);
      var phase = nativeValue("OpenTS_Browser_Status");
      recordPhase(phase);
      recordDataPath();
      if (result === 3 || phase === 7) throw new Error("native process entered Fault (" + nativeValue("OpenTS_Browser_Fault_Code") + ")");
      if ((result !== 1 && result !== 2) || phase < 0 || phase > 7) {
        throw new Error("unexpected bounded native Step result/state: " + result + "/" + phase);
      }
      if (phase === 6) {
        check(returnedCalls === callbackCount, "browser callback did not return after each native Step");
        check(nativeValue("OpenTS_Browser_Step_Count") === returnedCalls, "native Step and browser callback counts differ");
        check(nativeValue("OpenTS_124H_Boundary_Probe_Passed") === 1, "guard did not see the synthetic bootstrap sentinel");
        finishProcess();
        return;
      }
    } catch (error) {
      fail(error);
      return;
    }
    requestAnimationFrame(onFrame);
  }

  async function startFailureStage(Module) {
    var data = await storage.prepareData(Module);
    storage.mountUserRoot(Module);
    var hydrationFailure = "";
    try { await storage.hydrateUser(Module, true); }
    catch (error) { hydrationFailure = errorText(error); }
    check(hydrationFailure.indexOf("Injected synthetic initial IDBFS hydration failure") !== -1,
      "initial User hydration failure was not injected and observed");
    check(nativeValue("OpenTS_124H_Storage_Is_Ready") === 0,
      "storage became authoritative after failed User hydration");
    check(nativeValue("OpenTS_Browser_Begin_Count") === 0,
      "browser process began despite failed User hydration");
    check(nativeValue("OpenTS_Browser_Main_Returned") === 0,
      "Emscripten main ran despite failed User hydration");
    var faultFinishProof = nativeValue("OpenTS_124H_Browser_Fault_Finish_Proof");
    check(faultFinishProof === 15,
      "native BrowserProcessSession Fault and idempotent Finish proof did not pass");
    var callbackTotal = 0;
    await new Promise(function (resolve) {
      function tick() {
        callbackTotal += 1;
        if (callbackTotal < 60) requestAnimationFrame(tick);
        else resolve();
      }
      requestAnimationFrame(tick);
    });
    var failureStage = {
      storagePreparationPhase: "after Emscripten runtime initialization, before process main",
      dataPublication: data.generation,
      userHydration: "INJECTED FAILURE",
      hydrationError: hydrationFailure,
      nativeProcessFaultAndFinish: {
        resultMask: faultFinishProof,
        faultResultObserved: (faultFinishProof & 1) !== 0,
        faultPhaseObserved: (faultFinishProof & 2) !== 0,
        expectedFaultCodeObserved: (faultFinishProof & 4) !== 0,
        FinishCalledExactlyOnce: (faultFinishProof & 8) !== 0
      },
      faultFinishProofPassed: faultFinishProof === 15,
      failClosed: nativeValue("OpenTS_Browser_Begin_Count") === 0
        && nativeValue("OpenTS_Browser_Main_Returned") === 0
        && nativeValue("OpenTS_124H_Storage_Is_Ready") === 0,
      browserCallbacks: callbackTotal,
      heartbeatTicks: heartbeatCount,
      ownerDataAccessed: false
    };
    state.hydrationFailure = failureStage;
    state.runtimeErrors = (state.runtimeErrors || []).concat(runtimeErrors);
    state.consoleErrors = (state.consoleErrors || []).concat(consoleErrors);
    state.remoteDataRequests = (state.remoteDataRequests || []).concat(ownerRequests());
    saveState();
    var success = state.processStages && state.processStages.length === 2
      && state.userWrite && state.userReload && failureStage.failClosed
      && failureStage.faultFinishProofPassed
      && state.processStages.every(function (record) {
        return record.emscriptenMainReturned && record.mainEntryReturned
          && record.beginCount === 1 && record.beginResult === 1
          && record.browserCallbacks > 0 && record.browserCallbacks === record.returnedCallbacks
          && record.returnedCallbacks === record.nativeProcessSteps
          && [0, 1, 2, 3, 4, 5, 6].every(function (phase) { return record.phaseProgression.indexOf(phase) !== -1; })
          && record.boundaryProbePassed && record.boundaryProbeCount === 1
          && record.storageReadyAtBoundary && record.finishResult === 1 && record.finishCount === 1
          && record.finalProcessStatus === 6 && record.rendererInitialized && !!record.rendererName
          && record.webglContextAvailable
          && record.dataRoot === "/data/" && record.dataPathSamples.length === 1
          && record.dataPathSamples[0] === "/data/" && record.dataPathStable
          && record.dataProbePassed && record.rootEntries === 3 && record.nestedEntries === 1
          && record.heartbeatTicks > 0 && record.runtimeErrors.length === 0 && record.consoleErrors.length === 0;
      })
      && state.userWrite.explicitFlush === "PASS"
      && state.userReload.fullPageRuntimeReload && state.userReload.hydratedBeforeMain
      && state.userReload.exactBytesMatch && state.userReload.nativeReadSize === 257
      && state.userWrite.size === 257 && state.userReload.nativeReadSha256 === state.userWrite.sha256
      && state.runtimeErrors.length === 0 && state.consoleErrors.length === 0
      && state.remoteDataRequests.length === 0;
    finished = true;
    if (heartbeatTimer) clearInterval(heartbeatTimer);
    if (timeoutTimer) clearTimeout(timeoutTimer);
    publish({
      classification: success ? "PASS" : "FAIL",
      browser: navigator.userAgent,
      storagePreparationPhase: "OPFS generation + /data mount + /userdata IDBFS hydration before callMain",
      dataGeneration: state.dataGeneration,
      dataRoot: "/data",
      nativeDataProbe: state.processStages.map(function (record) {
        return { passed: record.dataRoot === "/data/" && record.dataPathStable,
          rootEntries: record.rootEntries, nestedEntries: record.nestedEntries };
      }),
      processStages: state.processStages,
      userWrite: state.userWrite,
      userReload: state.userReload,
      failClosedHydration: failureStage,
      nativeProcessFaultAndFinish: failureStage.nativeProcessFaultAndFinish,
      heartbeatAfterFullRun: heartbeatCount,
      ownerDataRequestsAfterLocalInstall: state.remoteDataRequests,
      ownerDataAccessed: false,
      movieNetworkPersistenceOrGameplayExpansion: false,
      asyncify: false,
      jspi: false,
      runtimeErrors: state.runtimeErrors,
      consoleErrors: state.consoleErrors
    });
    sessionStorage.removeItem(stateKey);
  }

  async function startOwnerRuntime(Module) {
    var mountStartedAt = performance.now();
    setOwnerPhase("opfs-mount", "Opening the accepted TS generation from this browser’s local OPFS…");
    var prepared = await storage.prepareImportedOwnerData(Module);
    importedOwnerDataSummary = prepared.generation;
    recordOwnerMilestone("opfs-mounted", "Read-only TS OPFS mount is ready.", {
      fileCount: prepared.generation.fileCount,
      totalBytes: prepared.generation.totalBytes,
      durationMs: Number((performance.now() - mountStartedAt).toFixed(1))
    });

    setOwnerPhase("userdata-mount", "Mounting local /userdata for OpenTS…");
    storage.mountUserRoot(Module);
    var hydrationStartedAt = performance.now();
    setOwnerPhase("userdata-hydration", "Restoring local OpenTS settings and saves…");
    await storage.hydrateUser(Module, false);
    recordOwnerMilestone("userdata-hydrated", "Local /userdata hydration completed.", {
      durationMs: Number((performance.now() - hydrationStartedAt).toFixed(1))
    });

    setOwnerPhase("profile-check", "Checking the installed TS profile before native startup…");
    check(Module._OpenTS_126_Storage_Mark_Ready() === 1, "native imported TS storage readiness check failed");
    check(Module._OpenTS_126_Storage_Is_Ready() === 1, "imported TS storage is not ready before startup main");
    check(Module._OpenTS_126_Data_Probe() === 1, "native imported TS Data probe failed");
    recordOwnerMilestone("profile-ready", "Native TS data profile check passed.", {
      fileCount: prepared.generation.fileCount,
      dataRoot: "/data/",
      copiedOwnerData: false
    });

    startTime = performance.now();
    check(nativeValue("OpenTS_Browser_Status") === 0, "process was not Idle before Emscripten main");
    setOwnerPhase("native-bootstrap", "Starting the OpenTS renderer and Tiberian Sun runtime…");
    await new Promise(function (resolve) { requestAnimationFrame(function () { resolve(); }); });
    var nativeStart = performance.now();
    Module.callMain(["-DATADIR=/data", "-USERDIR=/userdata"]);
    var nativeDuration = performance.now() - nativeStart;
    recordOwnerMilestone("module-main-returned", "OpenTS native entry point returned to the browser scheduler.", {
      durationMs: Number(nativeDuration.toFixed(1)),
      beginCount: nativeValue("OpenTS_Browser_Begin_Count"),
      beginResult: nativeValue("OpenTS_Browser_Begin_Result"),
      rendererInitialized: nativeValue("OpenTS_Browser_Renderer_Initialized") === 1,
      rendererName: nativeText("OpenTS_Browser_Renderer_Name"),
      bootstrapStage: nativeValue("OpenTS_Browser_Bootstrap_Stage"),
      bootstrapMixfilesOpened: nativeValue("OpenTS_Browser_Bootstrap_Mixfiles_Opened") === 1,
      secondaryMixfilesOpened: nativeValue("OpenTS_Browser_Secondary_Mixfiles_Opened") === 1
    });
    check(nativeValue("OpenTS_Browser_Main_Returned") === 1, "Emscripten main did not return after Begin");
    check(nativeValue("OpenTS_Browser_Begin_Count") === 1
      && nativeValue("OpenTS_Browser_Begin_Result") === 1, "native OpenTS Begin did not succeed exactly once");
    requestAnimationFrame(onFrame);
  }

  async function startProcessStage(Module) {
    var prepared = ownerMode
      ? await storage.prepareImportedOwnerData(Module)
      : await storage.prepareData(Module);
    storage.mountUserRoot(Module);
    await storage.hydrateUser(Module, false);
    if (ownerMode) {
      check(Module._OpenTS_126_Storage_Mark_Ready() === 1, "native imported TS storage readiness check failed");
      check(Module._OpenTS_126_Storage_Is_Ready() === 1, "imported TS storage is not ready before startup main");
      check(Module._OpenTS_126_Data_Probe() === 1, "native imported TS Data probe failed");
      importedOwnerDataSummary = prepared.generation;
    } else {
    check(Module._OpenTS_124H_Storage_Mark_Ready() === 1, "native storage readiness check failed");
    check(Module._OpenTS_124H_Storage_Is_Ready() === 1, "storage is not ready before startup main");
    var dataProbeResult = Module._OpenTS_124H_Data_Probe();
    check(dataProbeResult === 1, "native Data enumeration/stat/open/read/seek proof failed (diagnostic mask "
      + Math.abs(dataProbeResult) + ")");
    state.storagePreparationPhase = "OPFS Data prepared and /userdata hydrated before main";
    state.dataGeneration = prepared.generation;
    window.__VV124H_GENERATION__ = prepared.generation;
    saveState();
    }
    startTime = performance.now();
    check(nativeValue("OpenTS_Browser_Status") === 0, "process was not Idle before Emscripten main");
    recordPhase(nativeValue("OpenTS_Browser_Status"));
    Module.callMain(["-DATADIR=/data", "-USERDIR=/userdata"]);
    check(nativeValue("OpenTS_Browser_Main_Returned") === 1,
      "Emscripten main did not return after Begin");
    check(nativeValue("OpenTS_Browser_Begin_Count") === 1
      && nativeValue("OpenTS_Browser_Begin_Result") === 1, "process Begin did not succeed exactly once");
    timeoutTimer = setTimeout(function () { fail(new Error("browser process proof timed out")); }, ownerMode ? 300000 : 30000);
    requestAnimationFrame(onFrame);
  }

  var originalConsoleError = console.error.bind(console);
  console.error = function () {
    var message = Array.prototype.map.call(arguments, textOf).join(" ");
    consoleErrors.push(message);
    originalConsoleError.apply(console, arguments);
  };
  window.addEventListener("error", function (event) {
    runtimeErrors.push(event.message || "unhandled browser error");
    fail(new Error(event.message || "unhandled browser error"));
  });
  window.addEventListener("unhandledrejection", function (event) {
    runtimeErrors.push(textOf(event.reason));
    fail(new Error("Unhandled rejection: " + textOf(event.reason)));
  });

  window.Module = {
    canvas: document.getElementById("canvas"),
    noInitialRun: true,
    noExitRuntime: true,
    onRuntimeInitialized: function () {
      startTime = performance.now();
      if (!ownerMode) {
        heartbeatTimer = setInterval(function () { heartbeatCount += 1; }, 4);
        timeoutTimer = setTimeout(function () {
          fail(new Error("bounded browser storage/process proof timed out"));
        }, 60000);
      } else {
        var moduleResource = performance.getEntriesByType("resource")
          .filter(function (entry) { return /OpenTS\.js(?:$|\?)/i.test(entry.name); }).pop();
        recordOwnerMilestone("wasm-ready", "OpenTS WebAssembly module initialized.", {
          durationMs: Number((performance.now() - routeStartedAt).toFixed(1)),
          moduleResourceMs: moduleResource ? Number(moduleResource.duration.toFixed(1)) : null
        });
        setOwnerPhase("opfs-mount", "OpenTS module is ready; opening the accepted TS generation…");
      }
      try { window.focus(); } catch (_) { /* Canvas focus is retried on the user’s first pointerdown. */ }
      if (Module.canvas && Module.canvas.focus) Module.canvas.focus();
      var action = ownerMode
        ? startOwnerRuntime(Module)
        : (stage === "failure" ? startFailureStage(Module) : startProcessStage(Module));
      action.catch(fail);
    },
    onAbort: function (reason) {
      runtimeErrors.push("Emscripten abort: " + textOf(reason));
      fail(new Error("Emscripten abort: " + textOf(reason)));
    },
    printErr: function (message) {
      runtimeErrors.push(textOf(message));
      originalConsoleError(message);
    }
  };
}());
