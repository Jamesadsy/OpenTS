/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#ifndef OPENTS_SOURCE_ROOT
#error OPENTS_SOURCE_ROOT must point at the OpenTS source tree.
#endif

namespace
{

int Failures = 0;

void Check(bool condition, char const * description)
{
	std::printf("%-78s %s\n", description, condition ? "ok" : "FAILED");
	if (!condition) ++Failures;
}

std::size_t CountOccurrences(std::string const & text, std::string const & needle)
{
	std::size_t count = 0;
	std::size_t position = 0;
	while ((position = text.find(needle, position)) != std::string::npos) {
		++count;
		position += needle.size();
	}
	return(count);
}

std::string ReadSource(char const * relative_path)
{
	std::ifstream file(std::string(OPENTS_SOURCE_ROOT) + "/" + relative_path, std::ios::binary);
	return(std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()));
}

void Test_Synthetic_Browser_Storage_Contract(void)
{
	std::string const helper = ReadSource("code/browserstorage.cpp");
	std::string const coordinator = ReadSource("code/browsercoord.cpp");
	std::string const init = ReadSource("code/init.cpp");
	std::string const harness = ReadSource("tests/browserstorage/harness.js");
	std::string const storage = ReadSource("tests/browserstorage/browserstorage.js");
	std::string const cmake = ReadSource("code/CMakeLists.txt");
	std::size_t const bootstrap = init.rfind("static bool Bootstrap(void)");
	std::size_t const readiness = init.find("OpenTS_124H_Storage_Is_Ready()", bootstrap);
	std::size_t const boundary = init.find("OpenTS_Browser_Startup_Reach_OwnerData_Boundary();", readiness);
	std::size_t const sentinel = init.find("OpenTS_124H_AtOwnerDataBoundary()", boundary);
	std::size_t const stop = init.find("return(false);", sentinel);
	std::size_t const mixfiles = init.find("Init_Bootstrap_Mixfiles())", stop);
	Check(readiness != std::string::npos && boundary != std::string::npos
		&& sentinel != std::string::npos && stop != std::string::npos
		&& mixfiles != std::string::npos && readiness < boundary && boundary < sentinel
		&& sentinel < stop && stop < mixfiles,
		"storage readiness and synthetic sentinel probe stay before the existing owner-data return");
	std::size_t const imported_readiness = init.find("OpenTS_126_Storage_Is_Ready()", bootstrap);
	std::size_t const imported_probe = init.find("OpenTS_126_Data_Probe()", imported_readiness);
	std::size_t const imported_boundary = init.find("OpenTS_Browser_Startup_Reach_OwnerData_Boundary();", imported_probe);
	std::size_t const real_mixfiles = init.find("if (!Init_Bootstrap_Mixfiles())", imported_boundary);
	Check(imported_readiness != std::string::npos && imported_probe != std::string::npos
		&& imported_boundary != std::string::npos && real_mixfiles != std::string::npos
		&& imported_readiness < imported_probe && imported_probe < imported_boundary
		&& imported_boundary < real_mixfiles,
		"imported TS OPFS data passes its native profile probe before real engine MIX bootstrap");
	Check(helper.find("opendir(path)") != std::string::npos
		&& helper.find("rewinddir(directory)") != std::string::npos
		&& helper.find("List_Directory(\"/data\", root_first, root_rewound)") != std::string::npos
		&& helper.find("fseek(file, offset, SEEK_SET)") != std::string::npos
		&& helper.find("Verify_Sentinel(sentinel_path.c_str(), 257)") != std::string::npos,
		"native probes cover POSIX enumeration/rewind, seek, and a synthetic boundary sentinel");
	Check(helper.find("User_File_Write_Name(\"124h-synthetic-user.bin\")") != std::string::npos
		&& helper.find("OpenTS_124H_User_Write") != std::string::npos
		&& helper.find("OpenTS_124H_User_Readback_Hex") != std::string::npos,
		"native C++ owns deterministic User file write and readback");
	Check(storage.find("navigator.storage.getDirectory()") != std::string::npos
		&& storage.find("createWritable") != std::string::npos
		&& storage.find("BLOCK_BYTES = 256 * 1024") != std::string::npos
		&& storage.find("CACHE_LIMIT_BYTES = 16 * 1024 * 1024") != std::string::npos
		&& storage.find("File.prototype.arrayBuffer") == std::string::npos,
		"OPFS Data generation is locally published and runtime reads use bounded slices/cache");
	Check(storage.find("FS.filesystems.IDBFS") != std::string::npos
		&& storage.find("FS.syncfs(populate") != std::string::npos
		&& storage.find("OpenTS/124H/generations/synthetic-v1") != std::string::npos,
		"Data generation and separate IDBFS User root have explicit mount/hydrate/flush seams");
	Check(storage.find("prepareImportedOwnerData") != std::string::npos
		&& storage.find("installedFiles(scope, undefined, TS_GAME)") != std::string::npos
		&& storage.find("copiedOwnerData: false") != std::string::npos
		&& harness.find("OpenTS_Browser_Frontend_Ready") != std::string::npos
		&& harness.find("OpenTS_Browser_Request_Proof_Stop") == std::string::npos
		&& harness.find("finishImportedOwnerProcess") == std::string::npos,
		"126 mounts the installed TS generation in place and leaves the native frontend interactive");
	Check(harness.find("noInitialRun: true") != std::string::npos
		&& harness.find("await storage.hydrateUser(Module, false)") < harness.find("Module.callMain(")
		&& harness.find("location.replace(\"?stage=readback\")") != std::string::npos
		&& harness.find("location.replace(\"?stage=failure\")") != std::string::npos
		&& harness.find("await storage.hydrateUser(Module, true)") != std::string::npos
		&& harness.find("check(nativeValue(\"OpenTS_Browser_Status\") === 0") < harness.find("Module.callMain("),
		"process starts only after storage hydration and the harness covers reload and fail-closed hydration");
	Check(harness.find("OpenTS_Browser_Step") != std::string::npos
		&& harness.find("requestAnimationFrame(onFrame)") != std::string::npos
		&& harness.find("heartbeatCount += 1") != std::string::npos
		&& harness.find("Select_Game") == std::string::npos
		&& harness.find("Main_Loop") == std::string::npos,
		"browser only schedules one native Step per callback and records an independent heartbeat");
	Check(coordinator.find("OpenTS_124H_Browser_Fault_Finish_Proof") != std::string::npos
		&& harness.find("nativeProcessFaultAndFinish") != std::string::npos
		&& harness.find("faultFinishProof === 15") != std::string::npos,
		"real browser harness exercises the native process Fault phase and idempotent Finish");
	Check(cmake.find("-sNO_EXIT_RUNTIME=1") != std::string::npos
		&& cmake.find("-sFORCE_FILESYSTEM=1") != std::string::npos
		&& cmake.find("-lidbfs.js") != std::string::npos
		&& cmake.find("_OpenTS_124H_AtOwnerDataBoundary") != std::string::npos
		&& cmake.find("OpenTS-browser-storage-proof.html") != std::string::npos
		&& cmake.find("ASYNCIFY") == std::string::npos
		&& cmake.find("JSPI") == std::string::npos,
		"Emscripten exports the bounded storage proof with normal filesystem support, without Asyncify or JSPI");
	Check(cmake.find("_OpenTS_126_Storage_Mark_Ready") != std::string::npos
		&& cmake.find("_OpenTS_126_Data_Probe") != std::string::npos
		&& cmake.find("_OpenTS_Browser_Frontend_Ready") != std::string::npos
		&& cmake.find("_OpenTS_Browser_Bootstrap_Mixfiles_Opened") != std::string::npos
		&& cmake.find("_OpenTS_Browser_Secondary_Mixfiles_Opened") != std::string::npos,
		"Emscripten exports the imported-data and engine-bootstrap acceptance probes");
}

void Test_Physical_Runtime_Input_And_Presentation(void)
{
	std::string const html = ReadSource("tests/browserstorage/harness.html");
	std::string const runtime = ReadSource("tests/browserstorage/harness.js");
	std::string const input = ReadSource("tests/browserstorage/runtime-input.js");
	std::string const sdl = ReadSource("thirdparty/SDL/src/video/emscripten/SDL_emscriptenevents.c");
	std::string const entry = ReadSource("platform/win32compat/src/entry.cpp");
	std::string const message = ReadSource("platform/win32compat/src/message.cpp");
	std::string const touch = ReadSource("platform/win32compat/src/touch.cpp");
	std::string const touch_test = ReadSource("tests/ioscontract/iostouchcontract.cpp");
	std::string const menu = ReadSource("code/grphmenu.cpp");
	std::string const menu_test = ReadSource("tests/newmenucontinuation/newmenucontinuationcontract.cpp");
	Check(html.find("Tiberian Sun development runtime") != std::string::npos
		&& html.find("local OPFS") != std::string::npos
		&& html.find("role=\"status\"") != std::string::npos
		&& html.find("<details id=\"runtime-diagnostics\">") != std::string::npos
		&& html.find("touch-action: none") != std::string::npos
		&& html.find("OpenTS 124H local storage proof") == std::string::npos
		&& html.find("Preparing synthetic local storage") == std::string::npos,
		"physical TS runtime starts with truthful phase, local custody, responsive canvas, and collapsed diagnostics");
	Check(runtime.find("OpenTS_Browser_Request_Proof_Stop") == std::string::npos
		&& runtime.find("finishImportedOwnerProcess") == std::string::npos
		&& runtime.find("requestAnimationFrame(onFrame)") != std::string::npos
		&& runtime.find("OpenTS_Browser_Frontend_Ready") != std::string::npos
		&& runtime.find("selector-interactive") != std::string::npos
		&& runtime.find("product-menu-interactive") != std::string::npos,
		"imported TS owner runtime remains live after stable selector and selected-product menus");
	Check(runtime.find("opfs-mount") != std::string::npos
		&& runtime.find("userdata-hydration") != std::string::npos
		&& runtime.find("wasm-ready") != std::string::npos
		&& runtime.find("renderer-ready") != std::string::npos
		&& runtime.find("bootstrap-mix") != std::string::npos
		&& runtime.find("newmenu-ready") != std::string::npos
		&& runtime.find("title-media-stage") != std::string::npos
		&& runtime.find("first-stable-menu") != std::string::npos,
		"runtime timeline records mount, hydration, WASM, renderer, MIX, NewMenu, title, and interactive-menu stages");
	Check(input.find("(clientX - rect.left) * canvas.width / rect.width") != std::string::npos
		&& input.find("(clientY - rect.top) * canvas.height / rect.height") != std::string::npos
		&& input.find("getBoundingClientRect()") != std::string::npos
		&& input.find("devicePixelRatio") != std::string::npos
		&& input.find("ResizeObserver") != std::string::npos
		&& input.find("orientationchange") != std::string::npos,
		"input diagnostics map CSS-scaled and DPR-backed pointers in logical canvas space and recalculate after viewport changes");
	Check(input.find("pointerdown") != std::string::npos
		&& input.find("canvas.focus({ preventScroll: true })") != std::string::npos
		&& input.find("passive: true") != std::string::npos
		&& input.find("dispatchEvent") == std::string::npos
		&& input.find("setPointerCapture") == std::string::npos
		&& input.find("stopPropagation") == std::string::npos,
		"canvas focus observes pointer input without capturing, synthesizing, or duplicating native events");
	Check(sdl.find("event->targetX * xscale") != std::string::npos
		&& sdl.find("window_data->window->w / client_w") != std::string::npos
		&& sdl.find("event->targetX / (client_w - 1)") != std::string::npos
		&& touch.find("event.tfinger.x * width") != std::string::npos
		&& touch.find("event.tfinger.y * height") != std::string::npos,
		"Emscripten and native touch recognizer convert CSS pointer coordinates to OpenTS logical window coordinates");
	Check(message.find("event.motion.which == SDL_TOUCH_MOUSEID") != std::string::npos
		&& message.find("event.button.which == SDL_TOUCH_MOUSEID") != std::string::npos
		&& CountOccurrences(message, "#if defined(OPENTS_IOS) || defined(__EMSCRIPTEN__)") == 2
		&& entry.find("SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, \"0\")")
			< entry.find("OpenTS_Browser_Begin();")
		&& entry.find("SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, \"0\")")
			< entry.find("OpenTS_Browser_Begin();")
		&& touch.find("button == SDL_BUTTON_RIGHT ? WM_RBUTTONDOWN : WM_LBUTTONDOWN") != std::string::npos
		&& touch.find("button == SDL_BUTTON_RIGHT ? WM_RBUTTONUP : WM_LBUTTONUP") != std::string::npos
		&& touch_test.find("Win32_Touch_Test_Button_Down_Count() == 1") != std::string::npos
		&& touch_test.find("Win32_Touch_Test_Button_Up_Count() == 1") != std::string::npos,
		"one iPhone tap reaches the native menu once while SDL compatibility mouse duplicates are suppressed");
	Check(menu.find("PendingKey == KN_LMOUSE || PendingKey == KN_RETURN") != std::string::npos
		&& menu.find("PresentationActionStarted") != std::string::npos
		&& menu_test.find("fixture.Starts == 1 && fixture.Sound == 1 && fixture.VQ == 1") != std::string::npos,
		"native GraphicMenu consumes a translated pointer activation once and retains C++ menu semantics");
	Check(sdl.find("Emscripten_HandleOrientationChange") != std::string::npos
		&& sdl.find("Emscripten_HandleResize(EMSCRIPTEN_EVENT_RESIZE") != std::string::npos
		&& message.find("SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED") != std::string::npos,
		"orientation and canvas pixel-size changes flow into the native window resize path");
}

} // namespace

int main(void)
{
	Test_Synthetic_Browser_Storage_Contract();
	Test_Physical_Runtime_Input_And_Presentation();
	std::printf("\n%s\n", Failures == 0 ? "PASSED" : "FAILED");
	return(Failures == 0 ? 0 : 1);
}
