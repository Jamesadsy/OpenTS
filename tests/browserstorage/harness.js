(function () {
  "use strict";

  var output = document.getElementById("proof");
  var storage = window.OpenTS124HStorage;
  var stateKey = "opents-124h-proof-state-v1";
  var stage = new URL(location.href).searchParams.get("stage") || "write";
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
  var maxStepMs = 0;
  var state = readState();
  if (stage === "write") {
    state = {};
    sessionStorage.removeItem(stateKey);
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
    document.title = "OpenTS 124H local storage proof: " + result.classification;
    document.dispatchEvent(new CustomEvent("vv-ts1a-124h-proof", { detail: result }));
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

  function fail(error) {
    if (finished) return;
    finished = true;
    if (timeoutTimer) clearTimeout(timeoutTimer);
    runtimeErrors.push(textOf(error && error.stack ? error.stack : error));
    state.runtimeErrors = (state.runtimeErrors || []).concat(runtimeErrors);
    state.consoleErrors = (state.consoleErrors || []).concat(consoleErrors);
    state.failure = errorText(error);
    saveState();
    publish({ classification: "FAIL", stage: stage, reason: errorText(error), state: state });
  }

  function onFrame() {
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

  async function startProcessStage(Module) {
    var prepared = await storage.prepareData(Module);
    storage.mountUserRoot(Module);
    await storage.hydrateUser(Module, false);
    check(Module._OpenTS_124H_Storage_Mark_Ready() === 1, "native storage readiness check failed");
    check(Module._OpenTS_124H_Storage_Is_Ready() === 1, "storage is not ready before startup main");
    var dataProbeResult = Module._OpenTS_124H_Data_Probe();
    check(dataProbeResult === 1, "native Data enumeration/stat/open/read/seek proof failed (diagnostic mask "
      + Math.abs(dataProbeResult) + ")");
    state.storagePreparationPhase = "OPFS Data prepared and /userdata hydrated before main";
    state.dataGeneration = prepared.generation;
    window.__VV124H_GENERATION__ = prepared.generation;
    saveState();
    startTime = performance.now();
    check(nativeValue("OpenTS_Browser_Status") === 0, "process was not Idle before Emscripten main");
    recordPhase(nativeValue("OpenTS_Browser_Status"));
    Module.callMain(["-DATADIR=/data", "-USERDIR=/userdata"]);
    check(nativeValue("OpenTS_Browser_Main_Returned") === 1,
      "Emscripten main did not return after Begin");
    check(nativeValue("OpenTS_Browser_Begin_Count") === 1
      && nativeValue("OpenTS_Browser_Begin_Result") === 1, "process Begin did not succeed exactly once");
    timeoutTimer = setTimeout(function () { fail(new Error("browser process proof timed out")); }, 30000);
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
      heartbeatTimer = setInterval(function () { heartbeatCount += 1; }, 4);
      timeoutTimer = setTimeout(function () {
        fail(new Error("bounded browser storage/process proof timed out"));
      }, 60000);
      window.focus();
      if (Module.canvas && Module.canvas.focus) Module.canvas.focus();
      var action = stage === "failure" ? startFailureStage(Module) : startProcessStage(Module);
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
