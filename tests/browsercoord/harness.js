(function () {
  "use strict";

  var output = document.getElementById("proof");
  var callbackCount = 0;
  var stepCalls = 0;
  var returnedCalls = 0;
  var heartbeatCount = 0;
  var totalStepMs = 0;
  var maxStepMs = 0;
  var startTime = 0;
  var timer = 0;
  var heartbeatTimer = 0;
  var finished = false;

  function publish(result) {
    window.__VV_TS1A_PROOF__ = result;
    output.textContent = JSON.stringify(result, null, 2);
    document.body.dataset.proof = result.classification === "PASS" ? "pass" : "fail";
    document.title = "OpenTS 124A coordinator proof: " + result.classification;
    document.dispatchEvent(new CustomEvent("vv-ts1a-proof", { detail: result }));
  }

  function fail(reason) {
    if (finished) return;
    finished = true;
    if (timer) clearTimeout(timer);
    if (heartbeatTimer) clearInterval(heartbeatTimer);
    publish({
      classification: "FAIL",
      reason: String(reason),
      callbackCount: callbackCount,
      stepCalls: stepCalls,
      returnedCalls: returnedCalls,
      heartbeatCount: heartbeatCount,
      elapsedMs: startTime ? performance.now() - startTime : 0
    });
  }

  function complete(stepResult) {
    var beforeResponsiveWait = heartbeatCount;
    var heartbeatCountAtCompletion = heartbeatCount;
    var nativeStatus = Module._OpenTS_Browser_Status();
    var finishResult = Module._OpenTS_Browser_Finish();
    var elapsedMs = performance.now() - startTime;

    setTimeout(function () {
      var responsive = heartbeatCount > beforeResponsiveWait;
      var passed = callbackCount >= 300
        && callbackCount === stepCalls
        && stepCalls === returnedCalls
        && stepResult === 2
        && nativeStatus === 2
        && finishResult === 1
        && heartbeatCount > 0
        && elapsedMs > 0
        && responsive;
      finished = true;
      if (heartbeatTimer) clearInterval(heartbeatTimer);
      if (timer) clearTimeout(timer);
      publish({
        classification: passed ? "PASS" : "FAIL",
        callbackCount: callbackCount,
        stepCalls: stepCalls,
        returnedCalls: returnedCalls,
        elapsedMs: Number(elapsedMs.toFixed(3)),
        heartbeatCount: heartbeatCountAtCompletion,
        heartbeatCountAfterCompletion: heartbeatCount,
        finalStepResult: stepResult,
        nativeFinalStatus: Module._OpenTS_Browser_Status(),
        finishResult: finishResult,
        maxStepDurationMs: Number(maxStepMs.toFixed(6)),
        averageStepDurationMs: Number((totalStepMs / Math.max(stepCalls, 1)).toFixed(6)),
        browserResponsive: responsive,
        unhandledException: false
      });
    }, 40);
  }

  function onFrame() {
    if (finished) return;
    callbackCount += 1;
    stepCalls += 1;
    var stepStart = performance.now();
    var stepResult;
    try {
      stepResult = Module._OpenTS_Browser_Step();
      returnedCalls += 1;
    } catch (error) {
      fail(error && error.stack ? error.stack : error);
      return;
    }
    var duration = performance.now() - stepStart;
    totalStepMs += duration;
    maxStepMs = Math.max(maxStepMs, duration);
    var nativeStatus = Module._OpenTS_Browser_Status();

    if (stepResult < 0 || stepResult > 2) {
      fail("native Step returned an unrecognized bounded result: " + stepResult);
      return;
    }
    if (nativeStatus < 0 || nativeStatus > 3) {
      fail("native status returned an unrecognized bounded phase: " + nativeStatus);
      return;
    }
    if (callbackCount < 300 && stepResult === 2) {
      fail("native control probe completed before 300 browser callbacks");
      return;
    }
    if (callbackCount >= 300) {
      if (stepResult !== 2 || nativeStatus !== 2) {
        fail("native control probe did not complete on the 300th callback");
        return;
      }
      complete(stepResult);
      return;
    }

    requestAnimationFrame(onFrame);
  }

  function beginProof() {
    try {
      var beginResult = Module._OpenTS_Browser_Begin();
      if (beginResult !== 1) {
        fail("OpenTS_Browser_Begin was rejected: " + beginResult);
        return;
      }
      startTime = performance.now();
      heartbeatTimer = setInterval(function () { heartbeatCount += 1; }, 4);
      timer = setTimeout(function () { fail("browser callback proof timed out"); }, 30000);
      requestAnimationFrame(onFrame);
    } catch (error) {
      fail(error && error.stack ? error.stack : error);
    }
  }

  window.addEventListener("error", function (event) {
    fail(event.message || "unhandled browser error");
  });
  window.addEventListener("unhandledrejection", function (event) {
    fail(event.reason || "unhandled promise rejection");
  });

  window.Module = {
    onRuntimeInitialized: beginProof,
    onAbort: function (reason) { fail("Emscripten abort: " + reason); },
    printErr: function (message) { console.error(message); }
  };
}());
