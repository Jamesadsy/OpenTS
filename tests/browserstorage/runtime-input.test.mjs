import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";
import vm from "node:vm";

const sourcePath = new URL("./runtime-input.js", import.meta.url);
const runtimeInputSource = await readFile(sourcePath, "utf8");

function target() {
  const listeners = new Map();
  return {
    addEventListener(name, callback, options) {
      const entries = listeners.get(name) || [];
      entries.push({ callback, options });
      listeners.set(name, entries);
    },
    removeEventListener(name, callback) {
      listeners.set(name, (listeners.get(name) || []).filter(entry => entry.callback !== callback));
    },
    dispatch(name, event = {}) {
      event.type = name;
      for (const { callback } of listeners.get(name) || []) callback(event);
    },
    listenerOptions(name) { return (listeners.get(name) || []).map(entry => entry.options); }
  };
}

function setup() {
  const doc = { activeElement: null };
  const visualViewport = Object.assign(target(), { width: 390, height: 700, scale: 1 });
  let resizeObserver;
  class ResizeObserverFake {
    constructor(callback) { this.callback = callback; resizeObserver = this; }
    observe(element) { this.element = element; }
    disconnect() { this.disconnected = true; }
  }
  const global = Object.assign(target(), {
    document: doc,
    visualViewport,
    ResizeObserver: ResizeObserverFake,
    innerWidth: 390,
    innerHeight: 700,
    devicePixelRatio: 3,
    KeyboardEvent: class KeyboardEventFake {
      constructor(type, init) { this.type = type; Object.assign(this, init); }
    }
  });
  vm.runInNewContext(runtimeInputSource, global, { filename: "runtime-input.js" });

  let rect = { left: 20, top: 30, width: 320, height: 240 };
  const canvas = Object.assign(target(), {
    style: { touchAction: "auto" },
    width: 640,
    height: 480,
    focusCalls: 0,
    dispatched: [],
    getBoundingClientRect() { return rect; },
    focus(options) { this.focusCalls += 1; this.focusOptions = options; doc.activeElement = this; },
    dispatchEvent(event) { this.dispatched.push(event); return true; },
    setRect(value) { rect = value; }
  });
  const events = [];
  const bridge = global.OpenTSRuntimeInput.install(canvas, event => events.push(event));
  return { global, canvas, doc, events, bridge, resizeObserver, setRect: value => { rect = value; } };
}

test("maps pointer and touch positions through CSS scaling into the logical canvas", () => {
  const { global, canvas } = setup();
  const point = global.OpenTSRuntimeInput.mapClientToLogical(canvas, 180, 150);
  assert.deepEqual(JSON.parse(JSON.stringify(point)), { x: 320, y: 240, inside: true, width: 640, height: 480 });
  assert.equal(global.devicePixelRatio, 3);

  canvas.setRect({ left: 10, top: 20, width: 480, height: 360 });
  const touchPoint = global.OpenTSRuntimeInput.mapClientToLogical(canvas, 130, 110);
  assert.deepEqual(JSON.parse(JSON.stringify(touchPoint)), { x: 160, y: 120, inside: true, width: 640, height: 480 });
});

test("focuses the canvas without cancelling or translating one native pointer event", () => {
  const { canvas, doc, events } = setup();
  let prevented = false;
  let stopped = false;
  canvas.dispatch("pointerdown", {
    pointerType: "touch", pointerId: 7, clientX: 180, clientY: 150,
    preventDefault() { prevented = true; },
    stopPropagation() { stopped = true; }
  });

  assert.equal(doc.activeElement, canvas);
  assert.deepEqual(JSON.parse(JSON.stringify(canvas.focusOptions)), { preventScroll: true });
  assert.equal(canvas.focusCalls, 1);
  assert.equal(canvas.style.touchAction, "none");
  assert.equal(events.length, 1);
  assert.equal(events[0].type, "pointerdown");
  assert.deepEqual(JSON.parse(JSON.stringify(events[0].logical)), { x: 320, y: 240, inside: true, width: 640, height: 480 });
  assert.equal(prevented, false);
  assert.equal(stopped, false);
  assert.doesNotMatch(runtimeInputSource, /new global\.(?:Mouse|Touch)Event|setPointerCapture|stopPropagation/);
  assert.match(runtimeInputSource, /new global\.KeyboardEvent/);
});

test("recalculates logical mappings after orientation and viewport resizing", () => {
  const { global, canvas, events, resizeObserver } = setup();
  canvas.setRect({ left: 20, top: 30, width: 240, height: 180 });
  resizeObserver.callback();
  global.dispatch("orientationchange", {});
  const point = global.OpenTSRuntimeInput.mapClientToLogical(canvas, 140, 120);
  assert.deepEqual(JSON.parse(JSON.stringify(point)), { x: 320, y: 240, inside: true, width: 640, height: 480 });
  assert.equal(events[0].type, "canvas-resize");
  assert.equal(events[1].type, "orientationchange");
  assert.equal(events[1].metrics.cssRect.width, 240);
});

test("forwards only the native menu keys as focused keydown and keyup events", () => {
  const { global, canvas, doc } = setup();
  assert.equal(global.OpenTSRuntimeInput.dispatchKey(canvas, "Enter", true), true);
  assert.equal(global.OpenTSRuntimeInput.dispatchKey(canvas, "Enter", false), true);
  assert.equal(doc.activeElement, canvas);
  assert.deepEqual(canvas.dispatched.map(event => ({ type: event.type, key: event.key, code: event.code })), [
    { type: "keydown", key: "Enter", code: "Enter" },
    { type: "keyup", key: "Enter", code: "Enter" }
  ]);
  assert.equal(canvas.dispatched[0].bubbles, true);
  assert.equal(canvas.dispatched[0].cancelable, true);
  assert.equal(global.OpenTSRuntimeInput.dispatchKey(canvas, "UntrustedCommand", true), false);
  assert.equal(global.OpenTSRuntimeInput.dispatchKey(canvas, "Escape", "down"), false);
  assert.equal(canvas.dispatched.length, 2);
});
