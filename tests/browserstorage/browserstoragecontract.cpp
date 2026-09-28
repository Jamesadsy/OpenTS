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
		&& harness.find("OpenTS_Browser_Request_Proof_Stop") != std::string::npos,
		"126 mounts the installed TS generation in place and stops after stable native frontend readiness");
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

} // namespace

int main(void)
{
	Test_Synthetic_Browser_Storage_Contract();
	std::printf("\n%s\n", Failures == 0 ? "PASSED" : "FAILED");
	return(Failures == 0 ? 0 : 1);
}
