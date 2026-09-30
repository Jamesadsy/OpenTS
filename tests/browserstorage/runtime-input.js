(function (global) {
  "use strict";

  function mapClientToLogical(canvas, clientX, clientY) {
    var rect = canvas.getBoundingClientRect();
    if (!rect || rect.width <= 0 || rect.height <= 0 || canvas.width <= 0 || canvas.height <= 0) {
      return null;
    }
    var x = (clientX - rect.left) * canvas.width / rect.width;
    var y = (clientY - rect.top) * canvas.height / rect.height;
    return {
      x: x,
      y: y,
      inside: x >= 0 && y >= 0 && x < canvas.width && y < canvas.height,
      width: canvas.width,
      height: canvas.height
    };
  }

  function measure(canvas) {
    var rect = canvas.getBoundingClientRect();
    return {
      viewport: { width: global.innerWidth, height: global.innerHeight },
      visualViewport: global.visualViewport ? {
        width: global.visualViewport.width,
        height: global.visualViewport.height,
        scale: global.visualViewport.scale
      } : null,
      devicePixelRatio: global.devicePixelRatio || 1,
      cssRect: { left: rect.left, top: rect.top, width: rect.width, height: rect.height },
      backing: { width: canvas.width, height: canvas.height }
    };
  }

  function install(canvas, onEvent) {
    var report = typeof onEvent === "function" ? onEvent : function () {};
    var previousTouchAction = canvas.style.touchAction;
    var listeners = [];
    var observer = null;
    canvas.style.touchAction = "none";

    function listen(target, name, callback, options) {
      target.addEventListener(name, callback, options);
      listeners.push({ target: target, name: name, callback: callback, options: options });
    }

    function focusOnPointerDown() {
      try {
        if (global.document.activeElement !== canvas) canvas.focus({ preventScroll: true });
      } catch (_) {
        try { canvas.focus(); } catch (_) { /* Some embedded WebViews reject programmatic focus. */ }
      }
    }

    function observePointer(event) {
      var logical = mapClientToLogical(canvas, event.clientX, event.clientY);
      report({
        type: event.type,
        pointerType: event.pointerType || "unknown",
        pointerId: Number.isFinite(event.pointerId) ? event.pointerId : null,
        client: { x: event.clientX, y: event.clientY },
        logical: logical,
        metrics: measure(canvas)
      });
    }

    function preventBrowserContextMenu(event) { event.preventDefault(); }
    function observeViewport(event) { report({ type: event.type || "resize", metrics: measure(canvas) }); }

    listen(canvas, "pointerdown", focusOnPointerDown, { passive: true });
    for (var type of ["pointerdown", "pointermove", "pointerup", "pointercancel"]) {
      listen(canvas, type, observePointer, { passive: true });
    }
    listen(canvas, "contextmenu", preventBrowserContextMenu, { passive: false });
    listen(global, "resize", observeViewport, { passive: true });
    listen(global, "orientationchange", observeViewport, { passive: true });
    if (global.visualViewport) listen(global.visualViewport, "resize", observeViewport, { passive: true });
    if (typeof global.ResizeObserver === "function") {
      observer = new global.ResizeObserver(function () { observeViewport({ type: "canvas-resize" }); });
      observer.observe(canvas);
    }

    return {
      measure: function () { return measure(canvas); },
      dispose: function () {
        if (observer) observer.disconnect();
        for (var listener of listeners) {
          listener.target.removeEventListener(listener.name, listener.callback, listener.options);
        }
        canvas.style.touchAction = previousTouchAction;
      }
    };
  }

  var nativeKeys = Object.freeze({
    Escape: { key: "Escape", code: "Escape" },
    Enter: { key: "Enter", code: "Enter" },
    ArrowUp: { key: "ArrowUp", code: "ArrowUp" },
    ArrowDown: { key: "ArrowDown", code: "ArrowDown" },
    ArrowLeft: { key: "ArrowLeft", code: "ArrowLeft" },
    ArrowRight: { key: "ArrowRight", code: "ArrowRight" }
  });

  function dispatchKey(canvas, value, down) {
    var key = nativeKeys[value];
    if (!canvas || !key || typeof down !== "boolean" || typeof global.KeyboardEvent !== "function") return false;
    try { canvas.focus({ preventScroll: true }); }
    catch (_) { try { canvas.focus(); } catch (_) { return false; } }
    return canvas.dispatchEvent(new global.KeyboardEvent(down ? "keydown" : "keyup", {
      key: key.key,
      code: key.code,
      bubbles: true,
      cancelable: true,
      composed: true,
      repeat: false
    }));
  }

  global.OpenTSRuntimeInput = {
    install: install,
    mapClientToLogical: mapClientToLogical,
    measure: measure,
    dispatchKey: dispatchKey
  };
}(typeof globalThis === "undefined" ? window : globalThis));
