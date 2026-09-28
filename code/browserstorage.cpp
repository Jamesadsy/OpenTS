/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * Browser storage contract probes. The 124H path keeps its synthetic sentinel;
 * the 126 path checks the mounted owner-data profile before engine bootstrap.
 ******************************************************************************/

#ifdef __EMSCRIPTEN__

#include "gamedirs.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace
{

bool StorageReady = false;
bool ImportedOwnerDataReady = false;
bool ImportedOwnerDataProbePassed = false;
bool DataProbePassed = false;
bool BoundaryProbePassed = false;
bool DataRootStable = true;
int RootEntryCount = 0;
int NestedEntryCount = 0;
int BoundaryProbeCount = 0;
int DataProbeFailureMask = 0;
long BoundarySentinelSize = -1;
std::string StableDataRoot;
std::string UserReadbackHex;
long UserReadbackSize = -1;

bool Is_Directory(char const * path)
{
	struct stat info = {};
	return(path != NULL && stat(path, &info) == 0 && S_ISDIR(info.st_mode));
}

bool Read_Directory(DIR * directory, std::vector<std::string> & names)
{
	struct dirent * entry = NULL;
	while ((entry = readdir(directory)) != NULL) {
		if (std::strcmp(entry->d_name, ".") != 0 && std::strcmp(entry->d_name, "..") != 0) {
			names.emplace_back(entry->d_name);
		}
	}
	std::sort(names.begin(), names.end());
	return(true);
}

bool List_Directory(char const * path, std::vector<std::string> & names,
	std::vector<std::string> & rewound_names)
{
	DIR * directory = opendir(path);
	if (directory == NULL) return(false);
	bool passed = Read_Directory(directory, names);
	rewinddir(directory);
	passed = Read_Directory(directory, rewound_names) && passed;
	int const close_result = closedir(directory);
	return(passed && close_result == 0);
}

bool Has_Name(std::vector<std::string> const & names, char const * name)
{
	return(std::find(names.begin(), names.end(), name) != names.end());
}

unsigned char Root_Byte(int index)
{
	return((unsigned char)((11 + index * 17 + (index >> 3) * 29) & 255));
}

unsigned char User_Byte(int index)
{
	return((unsigned char)((91 + index * 29 + (index >> 4) * 13) & 255));
}

bool Verify_Range(char const * path, long offset, int count)
{
	FILE * file = std::fopen(path, "rb");
	if (file == NULL) return(false);
	bool passed = std::fseek(file, offset, SEEK_SET) == 0;
	std::vector<unsigned char> bytes((std::size_t)count);
	if (passed) passed = std::fread(bytes.data(), 1, bytes.size(), file) == bytes.size();
	if (passed) {
		for (int index = 0; index < count; ++index) {
			if (bytes[(std::size_t)index] != Root_Byte((int)offset + index)) {
				passed = false;
				break;
			}
		}
	}
	if (std::fseek(file, 0, SEEK_END) != 0 || std::ftell(file) != 97) passed = false;
	std::fclose(file);
	return(passed);
}

bool Verify_Sentinel(char const * path, long expected_size)
{
	struct stat info = {};
	if (stat(path, &info) != 0 || !S_ISREG(info.st_mode) || info.st_size != expected_size) return(false);
	FILE * file = std::fopen(path, "rb");
	if (file == NULL) return(false);
	static char const prefix[] = "VV124H-SYNTHETIC-ONLY:";
	char bytes[sizeof(prefix)] = {};
	bool const passed = std::fread(bytes, 1, sizeof(prefix) - 1, file) == sizeof(prefix) - 1
		&& std::memcmp(bytes, prefix, sizeof(prefix) - 1) == 0;
	std::fclose(file);
	return(passed);
}

} // namespace

extern "C" int OpenTS_124H_Storage_Mark_Ready(void)
{
	StorageReady = Is_Directory("/data") && Is_Directory("/userdata");
	return(StorageReady ? 1 : 0);
}

extern "C" int OpenTS_124H_Storage_Is_Ready(void)
{
	return(StorageReady && Is_Directory("/data") && Is_Directory("/userdata") ? 1 : 0);
}

extern "C" int OpenTS_124H_Data_Probe(void)
{
	if (!OpenTS_124H_Storage_Is_Ready()) return(0);
	std::vector<std::string> root_first;
	std::vector<std::string> root_rewound;
	std::vector<std::string> nested_first;
	std::vector<std::string> nested_rewound;
	bool const root_listed = List_Directory("/data", root_first, root_rewound);
	bool const nested_listed = List_Directory("/data/NESTED", nested_first, nested_rewound);
	RootEntryCount = (int)root_first.size();
	NestedEntryCount = (int)nested_first.size();
	struct stat root_file = {};
	struct stat nested_file = {};
	bool const stats = stat("/data/ROOT.TST", &root_file) == 0 && S_ISREG(root_file.st_mode)
		&& root_file.st_size == 97
		&& stat("/data/NESTED/SAMPLE.BIN", &nested_file) == 0 && S_ISREG(nested_file.st_mode)
		&& nested_file.st_size == 513;
	struct stat absent = {};
	errno = 0;
	bool const absent_failed = stat("/data/ABSENT.BIN", &absent) != 0 && errno == ENOENT
		&& std::fopen("/data/ABSENT.BIN", "rb") == NULL;
	FILE * write_attempt = std::fopen("/data/WRITE.DEN", "wb");
	bool const immutable = write_attempt == NULL;
	if (write_attempt != NULL) std::fclose(write_attempt);
	bool const root_rewound_ok = root_listed && root_first == root_rewound;
	bool const nested_rewound_ok = nested_listed && nested_first == nested_rewound;
	bool const names = Has_Name(root_first, "ROOT.TST") && Has_Name(root_first, "NESTED")
		&& Has_Name(root_first, "TIBSUN.MIX") && Has_Name(nested_first, "SAMPLE.BIN");
	bool const range = Verify_Range("/data/ROOT.TST", 31, 23);
	bool const sentinel = Verify_Sentinel("/data/TIBSUN.MIX", 257);
	DataProbeFailureMask = 0;
	if (!root_rewound_ok) DataProbeFailureMask |= 1;
	if (!nested_rewound_ok) DataProbeFailureMask |= 2;
	if (!names) DataProbeFailureMask |= 4;
	if (!stats) DataProbeFailureMask |= 8;
	if (!absent_failed) DataProbeFailureMask |= 16;
	if (!immutable) DataProbeFailureMask |= 32;
	if (!range) DataProbeFailureMask |= 64;
	if (!sentinel) DataProbeFailureMask |= 128;
	DataProbePassed = DataProbeFailureMask == 0;
	return(DataProbePassed ? 1 : (DataProbeFailureMask == 0 ? -1 : -DataProbeFailureMask));
}

extern "C" int OpenTS_124H_AtOwnerDataBoundary(void)
{
	++BoundaryProbeCount;
	if (!OpenTS_124H_Storage_Is_Ready() || !DataProbePassed) return(0);
	std::string const configured_root = Data_Directory();
	if (configured_root != "/data/") return(0);
	std::string const sentinel_path = configured_root + "TIBSUN.MIX";
	struct stat info = {};
	if (stat(sentinel_path.c_str(), &info) == 0) BoundarySentinelSize = (long)info.st_size;
	BoundaryProbePassed = Verify_Sentinel(sentinel_path.c_str(), 257);
	return(BoundaryProbePassed ? 1 : 0);
}

extern "C" char const * OpenTS_124H_Data_Root(void)
{
	std::string const current = Data_Directory();
	if (!current.empty()) {
		if (StableDataRoot.empty()) StableDataRoot = current;
		else if (StableDataRoot != current) DataRootStable = false;
	}
	return(StableDataRoot.c_str());
}

extern "C" int OpenTS_124H_Data_Root_Stable(void) { return(DataRootStable ? 1 : 0); }
extern "C" int OpenTS_124H_Data_Probe_Passed(void) { return(DataProbePassed ? 1 : 0); }
extern "C" int OpenTS_124H_Boundary_Probe_Passed(void) { return(BoundaryProbePassed ? 1 : 0); }
extern "C" int OpenTS_124H_Boundary_Probe_Count(void) { return(BoundaryProbeCount); }
extern "C" int OpenTS_124H_Root_Entry_Count(void) { return(RootEntryCount); }
extern "C" int OpenTS_124H_Nested_Entry_Count(void) { return(NestedEntryCount); }
extern "C" long OpenTS_124H_Boundary_Sentinel_Size(void) { return(BoundarySentinelSize); }

extern "C" int OpenTS_124H_User_Write(void)
{
	if (!OpenTS_124H_Storage_Is_Ready()) return(0);
	std::string const path = User_File_Write_Name("124h-synthetic-user.bin");
	FILE * file = std::fopen(path.c_str(), "wb");
	if (file == NULL) return(0);
	bool passed = true;
	for (int index = 0; index < 257; ++index) {
		unsigned char const byte = User_Byte(index);
		if (std::fwrite(&byte, 1, 1, file) != 1) { passed = false; break; }
	}
	if (std::fflush(file) != 0) passed = false;
	if (std::fclose(file) != 0) passed = false;
	return(passed ? 1 : 0);
}

extern "C" char const * OpenTS_124H_User_Readback_Hex(void)
{
	UserReadbackHex.clear();
	UserReadbackSize = -1;
	std::string const path = User_File_Write_Name("124h-synthetic-user.bin");
	FILE * file = std::fopen(path.c_str(), "rb");
	if (file == NULL) return(NULL);
	std::vector<unsigned char> bytes(1024);
	std::size_t const count = std::fread(bytes.data(), 1, bytes.size(), file);
	bool const read_error = std::ferror(file) != 0;
	bool const at_end = std::feof(file) != 0;
	std::fclose(file);
	if (read_error || !at_end) return(NULL);
	UserReadbackSize = (long)count;
	static char const digits[] = "0123456789abcdef";
	UserReadbackHex.reserve(count * 2);
	for (std::size_t index = 0; index < count; ++index) {
		UserReadbackHex.push_back(digits[bytes[index] >> 4]);
		UserReadbackHex.push_back(digits[bytes[index] & 15]);
	}
	return(UserReadbackHex.c_str());
}

extern "C" long OpenTS_124H_User_Readback_Size(void) { return(UserReadbackSize); }

extern "C" int OpenTS_124H_User_Remove(void)
{
	std::string const path = User_File_Write_Name("124h-synthetic-user.bin");
	return(std::remove(path.c_str()) == 0 ? 1 : 0);
}

extern "C" int OpenTS_126_Storage_Mark_Ready(void)
{
	ImportedOwnerDataReady = Is_Directory("/data") && Is_Directory("/userdata");
	return(ImportedOwnerDataReady ? 1 : 0);
}

extern "C" int OpenTS_126_Storage_Is_Ready(void)
{
	return(ImportedOwnerDataReady && Is_Directory("/data") && Is_Directory("/userdata") ? 1 : 0);
}

extern "C" int OpenTS_126_Data_Probe(void)
{
	if (!OpenTS_126_Storage_Is_Ready()) return(0);
	struct stat tibsun = {};
	struct stat scores = {};
	bool const required_archives = stat("/data/TIBSUN.MIX", &tibsun) == 0
		&& S_ISREG(tibsun.st_mode) && tibsun.st_size > 0
		&& stat("/data/SCORES.MIX", &scores) == 0 && S_ISREG(scores.st_mode) && scores.st_size > 0;
	FILE * archive = required_archives ? std::fopen("/data/TIBSUN.MIX", "rb") : NULL;
	unsigned char signature[4] = {};
	bool const readable = archive != NULL && std::fread(signature, 1, sizeof(signature), archive) == sizeof(signature);
	if (archive != NULL) std::fclose(archive);
	std::vector<std::string> root;
	std::vector<std::string> rewound;
	bool const enumerable = List_Directory("/data", root, rewound) && root == rewound;
	ImportedOwnerDataProbePassed = required_archives && readable && enumerable && !root.empty();
	return(ImportedOwnerDataProbePassed ? 1 : 0);
}

extern "C" int OpenTS_126_Data_Probe_Passed(void)
{
	return(ImportedOwnerDataProbePassed ? 1 : 0);
}

#endif // __EMSCRIPTEN__
