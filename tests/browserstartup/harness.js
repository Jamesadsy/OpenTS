(function () {
  "use strict";

  var output = document.getElementById("proof");
  var callbackCount = 0;
  var returnedCalls = 0;
  var heartbeatCount = 0;
  var heartbeatAtTerminal = 0;
  var maxStepMs = 0;
  var phaseProgression = [];
  var runtimeErrors = [];
  var consoleErrors = [];
  var heartbeatTimer = 0;
  var timeoutTimer = 0;
  var terminal = false;
  var finished = false;
  var startTime = 0;
  var live = window.__VV_TS1A_124G_LIVE__ = {
    callbackCount: 0,
    returnedCalls: 0,
    heartbeatCount: 0,
    lastPhase: null,
    lastStepResult: null,
    lastBootstrapStage: null,
    lastCallbackStartedAt: null,
    lastCallbackReturnedAt: null,
    finishStarted: false
  };

  function publish(result) {
    window.__VV_TS1A_124G_PROOF__ = result;
    output.textContent = JSON.stringify(result, null, 2);
    document.body.dataset.proof = result.classification === "OWNER_DATA_BOUNDARY" ? "pass" : "fail";
    document.title = "OpenTS 124G process proof: " + result.classification;
    document.dispatchEvent(new CustomEvent("vv-ts1a-124g-proof", { detail: result }));
  }

  function textOf(value) {
    try { return typeof value === "string" ? value : JSON.stringify(value); }
    catch (_) { return String(value); }
  }

  function errorOf(error) {
    if (error && typeof error.stack === "string" && error.stack.length > 0) return error.stack;
    if (error && typeof error.message === "string" && error.message.length > 0) return error.message;
    try { return String(error); }
    catch (_) { return textOf(error); }
  }

  function recordPhase(phase) {
    if (phaseProgression.length === 0 || phaseProgression[phaseProgression.length - 1] !== phase) {
      phaseProgression.push(phase);
    }
  }

  function nativeValue(name) {
    var fn = Module["_" + name];
    return typeof fn === "function" ? fn() : null;
  }

  function rendererName() {
    var pointer = nativeValue("OpenTS_Browser_Renderer_Name");
    return pointer && Module.UTF8ToString ? Module.UTF8ToString(pointer) : "";
  }

  function finishAndPublish(classification, reason) {
    if (finished) return;
    finished = true;
    terminal = true;
    live.finishStarted = true;
    if (timeoutTimer) clearTimeout(timeoutTimer);
    heartbeatAtTerminal = heartbeatCount;

    var canvas = document.getElementById("canvas");
    var webglContext = null;
    if (canvas) {
      try { webglContext = canvas.getContext("webgl2") || canvas.getContext("webgl"); }
      catch (error) { runtimeErrors.push("canvas WebGL query: " + textOf(error)); }
    }
    var renderer = "";
    try { renderer = rendererName(); }
    catch (error) { runtimeErrors.push("renderer name: " + textOf(error)); }
    var finishResult = 0;
    try { finishResult = nativeValue("OpenTS_Browser_Finish"); }
    catch (error) {
      runtimeErrors.push("native Finish at cleanup stage " + nativeValue("OpenTS_Browser_Finish_Stage")
        + ", Prog_End stage " + nativeValue("OpenTS_Browser_ProgEnd_Stage") + ": " + errorOf(error));
    }

    setTimeout(function () {
      var browserStatus = nativeValue("OpenTS_Browser_Status");
      var beginCount = nativeValue("OpenTS_Browser_Begin_Count");
      var beginResult = nativeValue("OpenTS_Browser_Begin_Result");
      var mainReturned = nativeValue("OpenTS_Browser_Main_Returned");
      var stepCount = nativeValue("OpenTS_Browser_Step_Count");
      var waitFocusYields = nativeValue("OpenTS_Browser_WaitFocus_Yields");
      var mainGameBegan = nativeValue("OpenTS_Browser_MainGame_Began");
      var mainGameSteps = nativeValue("OpenTS_Browser_MainGame_Steps");
      var drainPasses = nativeValue("OpenTS_Browser_Shutdown_Drain_Passes");
      var dataBoundary = nativeValue("OpenTS_Browser_OwnerData_Boundary");
      var faultCode = nativeValue("OpenTS_Browser_Fault_Code");
      var finishCount = nativeValue("OpenTS_Browser_Finish_Count");
      var rendererInitialized = nativeValue("OpenTS_Browser_Renderer_Initialized");
      var allPhasesReached = [1, 2, 3, 4, 5, 6].every(function (phase) {
        return phaseProgression.indexOf(phase) !== -1;
      });
      var passed = classification === "OWNER_DATA_BOUNDARY"
        && beginCount === 1 && beginResult === 1 && mainReturned === 1
        && callbackCount > 0 && callbackCount === returnedCalls && callbackCount === stepCount
        && browserStatus === 6 && allPhasesReached && dataBoundary === 1
        && mainGameBegan === 1 && drainPasses >= 1 && finishResult === 1 && finishCount === 1
        && rendererInitialized === 1 && !!canvas && !!webglContext && renderer.length > 0
        && heartbeatAtTerminal > 0 && maxStepMs < 5000
        && faultCode === 0 && runtimeErrors.length === 0 && consoleErrors.length === 0;

      publish({
        classification: passed ? "OWNER_DATA_BOUNDARY" : "FAIL",
        reason: reason || (passed ? "Reached the guarded proprietary MIX boundary and shut down cooperatively." : "One or more browser process proof gates failed."),
        emscriptenMainReturned: mainReturned === 1,
        beginCount: beginCount,
        beginResult: beginResult,
        browserCallbacks: callbackCount,
        nativeProcessSteps: stepCount,
        returnedCallbacks: returnedCalls,
        phaseProgression: phaseProgression,
        waitFocusYields: waitFocusYields,
        mainGameBegan: mainGameBegan === 1,
        mainGameSteps: mainGameSteps,
        firstNoOwnerDataFrontier: dataBoundary === 1 ? "code/init.cpp::Bootstrap before Init_Bootstrap_Mixfiles" : null,
        shutdownDrainPasses: drainPasses,
        nativeFinishCalls: finishCount,
        finishResult: finishResult,
        rendererInitialized: rendererInitialized === 1,
        rendererName: renderer,
        canvasExistsBeforeFinish: !!canvas,
        webglContextAvailableBeforeFinish: !!webglContext,
        browserHeartbeatTicksDuringStartup: heartbeatAtTerminal,
        browserHeartbeatTicksAfterFinish: heartbeatCount,
        maxNativeStepMs: Number(maxStepMs.toFixed(3)),
        elapsedMs: Number((performance.now() - startTime).toFixed(3)),
        finalProcessStatus: browserStatus,
        faultCode: faultCode,
        consoleErrors: consoleErrors,
        runtimeErrors: runtimeErrors,
        bootstrapStage: nativeValue("OpenTS_Browser_Bootstrap_Stage"),
        finishStage: nativeValue("OpenTS_Browser_Finish_Stage"),
        progEndStage: nativeValue("OpenTS_Browser_ProgEnd_Stage")
      });
      if (heartbeatTimer) clearInterval(heartbeatTimer);
    }, 80);
  }

  function fail(reason) {
    if (finished) return;
    var status = nativeValue("OpenTS_Browser_Status");
    if (status === 6 || status === 7) {
      finishAndPublish("FAIL", String(reason));
      return;
    }
    finished = true;
    if (timeoutTimer) clearTimeout(timeoutTimer);
    if (heartbeatTimer) clearInterval(heartbeatTimer);
    publish({
      classification: "FAIL",
      reason: String(reason),
      browserCallbacks: callbackCount,
      returnedCallbacks: returnedCalls,
      nativeProcessSteps: nativeValue("OpenTS_Browser_Step_Count"),
      bootstrapStage: nativeValue("OpenTS_Browser_Bootstrap_Stage"),
      phaseProgression: phaseProgression,
      consoleErrors: consoleErrors,
      runtimeErrors: runtimeErrors
    });
  }

  function onFrame() {
    if (finished) return;
    callbackCount += 1;
    live.callbackCount = callbackCount;
    live.lastCallbackStartedAt = performance.now();
    var began = performance.now();
    try {
      recordPhase(nativeValue("OpenTS_Browser_Status"));
      var stepResult = nativeValue("OpenTS_Browser_Step");
      returnedCalls += 1;
      maxStepMs = Math.max(maxStepMs, performance.now() - began);
      var phase = nativeValue("OpenTS_Browser_Status");
      live.returnedCalls = returnedCalls;
      live.lastStepResult = stepResult;
      live.lastPhase = phase;
      live.lastBootstrapStage = nativeValue("OpenTS_Browser_Bootstrap_Stage");
      live.lastCallbackReturnedAt = performance.now();
      recordPhase(phase);
      if (stepResult === 3 || phase === 7) {
        finishAndPublish("FAIL", "Native browser process entered Fault (code " + nativeValue("OpenTS_Browser_Fault_Code") + ").");
        return;
      }
      if (stepResult !== 1 && stepResult !== 2) {
        fail("OpenTS_Browser_Step returned unexpected result " + stepResult + ".");
        return;
      }
      if (phase === 6) {
        finishAndPublish("OWNER_DATA_BOUNDARY");
        return;
      }
    } catch (error) {
      runtimeErrors.push(textOf(error && error.stack ? error.stack : error));
      fail("Native callback threw: " + textOf(error));
      return;
    }
    requestAnimationFrame(onFrame);
  }

  var oldConsoleError = console.error;
  console.error = function () {
    consoleErrors.push(Array.prototype.map.call(arguments, textOf).join(" "));
    if (oldConsoleError) oldConsoleError.apply(console, arguments);
  };

  window.addEventListener("error", function (event) {
    runtimeErrors.push(event.message || "unhandled window error");
    fail(event.message || "unhandled window error");
  });
  window.addEventListener("unhandledrejection", function (event) {
    runtimeErrors.push(textOf(event.reason));
    fail("Unhandled rejection: " + textOf(event.reason));
  });

  window.Module = {
    canvas: document.getElementById("canvas"),
    onRuntimeInitialized: function () {
      startTime = performance.now();
      live.runtimeInitializedAt = startTime;
      heartbeatTimer = setInterval(function () {
        heartbeatCount += 1;
        live.heartbeatCount = heartbeatCount;
      }, 4);
      timeoutTimer = setTimeout(function () { fail("Browser process proof timed out before Complete."); }, 30000);
      window.focus();
      if (Module.canvas && Module.canvas.focus) Module.canvas.focus();
      recordPhase(nativeValue("OpenTS_Browser_Status"));
      live.lastPhase = nativeValue("OpenTS_Browser_Status");
      requestAnimationFrame(onFrame);
    },
    onAbort: function (reason) {
      runtimeErrors.push("Emscripten abort: " + textOf(reason));
    },
    printErr: function (message) {
      runtimeErrors.push(textOf(message));
      if (oldConsoleError) oldConsoleError.call(console, message);
    }
  };
}());
