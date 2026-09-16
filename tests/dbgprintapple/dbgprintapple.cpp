/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// This is an Apple-only, no-owner-data contract harness for the real stderr diagnostic
// implementation.  It deliberately uses only a pipe and a test-owned temporary file.

#include "dbgprint.h"

#include <atomic>
#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace
{

int Failures = 0;


void Check(bool condition, char const * what)
{
	std::printf("%-72s %s\n", what, condition ? "ok" : "FAILED");
	if (!condition) {
		Failures++;
	}
}


bool Is_Digit(char value)
{
	return(value >= '0' && value <= '9');
}


bool Has_Timestamp(std::string const & line)
{
	return(line.size() >= 15
		&& line[0] == '['
		&& Is_Digit(line[1]) && Is_Digit(line[2])
		&& line[3] == ':'
		&& Is_Digit(line[4]) && Is_Digit(line[5])
		&& line[6] == ':'
		&& Is_Digit(line[7]) && Is_Digit(line[8])
		&& line[9] == '.'
		&& Is_Digit(line[10]) && Is_Digit(line[11]) && Is_Digit(line[12])
		&& line[13] == ']' && line[14] == ' ');
}


template<typename Action>
std::string Capture_Stderr(Action action, bool & setup_ok)
{
	int const caller_errno = errno;
	setup_ok = false;
	std::fflush(stderr);

	int const saved_stderr = ::dup(STDERR_FILENO);
	int pipe_fds[2] = {-1, -1};
	if (saved_stderr < 0 || ::pipe(pipe_fds) != 0) {
		if (saved_stderr >= 0) {
			::close(saved_stderr);
		}
		errno = caller_errno;
		return(std::string());
	}

	if (::dup2(pipe_fds[1], STDERR_FILENO) < 0) {
		::close(pipe_fds[0]);
		::close(pipe_fds[1]);
		::close(saved_stderr);
		errno = caller_errno;
		return(std::string());
	}
	::close(pipe_fds[1]);
	pipe_fds[1] = -1;

	action();
	std::fflush(stderr);
	bool const restored = ::dup2(saved_stderr, STDERR_FILENO) == 0;
	::close(saved_stderr);

	std::string captured;
	char buffer[1024];
	ssize_t count;
	while ((count = ::read(pipe_fds[0], buffer, sizeof(buffer))) > 0) {
		captured.append(buffer, static_cast<std::size_t>(count));
	}
	::close(pipe_fds[0]);

	setup_ok = restored && count == 0;
	errno = caller_errno;
	return(captured);
}


bool No_Second_Timestamp(std::string const & line)
{
	if (!Has_Timestamp(line)) {
		return(false);
	}
	return(line.find('[', 1) == std::string::npos
		&& line.find("] ", 15) == std::string::npos);
}


void Test_Formatting_And_Continuation(void)
{
	bool capture_ok = false;
	std::string const output = Capture_Stderr([] {
		DebugString("printf-style %d:", 17);
		DebugStringNoPrefix(" continued\n");
	}, capture_ok);

	Check(capture_ok && !output.empty(), "printf-style output reaches stderr");
	Check(capture_ok && No_Second_Timestamp(output), "logical line has one timestamp prefix");
	Check(capture_ok && output.size() >= 15
		&& output.substr(15) == "printf-style 17: continued\n",
		"DebugStringNoPrefix continues the same logical record");
}


void Test_Concurrent_Writers(void)
{
	constexpr int THREAD_COUNT = 6;
	constexpr int MESSAGES_PER_THREAD = 32;
	std::atomic<bool> start{false};

	bool capture_ok = false;
	std::string const output = Capture_Stderr([&] {
		std::vector<std::thread> writers;
		for (int thread_id = 0; thread_id < THREAD_COUNT; thread_id++) {
			writers.emplace_back([&, thread_id] {
				while (!start.load(std::memory_order_acquire)) {
					std::this_thread::yield();
				}
				for (int message = 0; message < MESSAGES_PER_THREAD; message++) {
					DebugString("dbgprintapple-thread=%d message=%d payload=0123456789abcdef\n",
						thread_id, message);
				}
			});
		}

		start.store(true, std::memory_order_release);
		for (std::thread & writer : writers) {
			writer.join();
		}
	}, capture_ok);

	std::vector<bool> seen(THREAD_COUNT * MESSAGES_PER_THREAD, false);
	int lines = 0;
	bool intact = capture_ok;
	std::size_t offset = 0;
	while (offset < output.size()) {
		std::size_t const newline = output.find('\n', offset);
		if (newline == std::string::npos) {
			intact = false;
			break;
		}

		std::string const line = output.substr(offset, newline - offset);
		offset = newline + 1;
		lines++;
		if (!No_Second_Timestamp(line)) {
			intact = false;
			continue;
		}

		std::size_t const prefix_length = 15;
		std::string const body = line.substr(prefix_length);
		int thread_id = -1;
		int message = -1;
		if (std::sscanf(body.c_str(), "dbgprintapple-thread=%d message=%d payload=0123456789abcdef",
			&thread_id, &message) != 2
			|| thread_id < 0 || thread_id >= THREAD_COUNT
			|| message < 0 || message >= MESSAGES_PER_THREAD) {
			intact = false;
			continue;
		}

		std::string const expected = "dbgprintapple-thread=" + std::to_string(thread_id)
			+ " message=" + std::to_string(message) + " payload=0123456789abcdef";
		if (body != expected) {
			intact = false;
			continue;
		}

		std::size_t const index = static_cast<std::size_t>(thread_id * MESSAGES_PER_THREAD + message);
		if (seen[index]) {
			intact = false;
		} else {
			seen[index] = true;
		}
	}

	for (bool message_seen : seen) {
		intact = intact && message_seen;
	}
	Check(lines == THREAD_COUNT * MESSAGES_PER_THREAD, "concurrent writers produce the expected records");
	Check(intact && offset == output.size(), "concurrent records have no torn or interleaved messages");
}


void Test_Errno(void)
{
	bool debug_preserved = false;
	bool continuation_preserved = false;
	bool capture_ok = false;
	Capture_Stderr([&] {
		errno = E2BIG;
		DebugString("errno-preserving\n");
		debug_preserved = errno == E2BIG;
		errno = ENAMETOOLONG;
		DebugStringNoPrefix("errno-continuation\n");
		continuation_preserved = errno == ENAMETOOLONG;
	}, capture_ok);
	Check(capture_ok && debug_preserved && continuation_preserved, "logging preserves C errno");
}


void Test_No_Console_And_No_Path(void)
{
	bool capture_ok = false;
	auto const before = std::chrono::steady_clock::now();
	std::string const output = Capture_Stderr([] {
		Debug_Init_Console();
		Debug_Console_Hold();
	}, capture_ok);
	auto const elapsed = std::chrono::steady_clock::now() - before;

	Check(capture_ok && output.empty(), "console entry points do not create Apple console output");
	Check(capture_ok && elapsed < std::chrono::seconds(1), "console entry points return without blocking");

	char const * const first_file = Debug_Log_File_Name();
	char const * const second_file = Debug_Log_File_Name();
	char const * const first_directory = Debug_Directory();
	char const * const second_directory = Debug_Directory();
	Check(first_file != nullptr && second_file != nullptr && first_file == second_file
		&& std::strcmp(first_file, "") == 0,
		"Debug_Log_File_Name truthfully reports no persistent path");
	Check(first_directory != nullptr && second_directory != nullptr && first_directory == second_directory
		&& std::strcmp(first_directory, "") == 0,
		"Debug_Directory truthfully reports no persistent path");
}


void Test_Fail_Closed_Pruning(void)
{
	char directory_template[] = "/tmp/opents-dbgprintapple-XXXXXX";
	char * const directory = ::mkdtemp(directory_template);
	bool const directory_created = directory != nullptr;
	bool sentinel_created = false;
	std::string sentinel;
	if (directory_created) {
		sentinel = std::string(directory) + "/DEBUG_keep.LOG";
		FILE * file = std::fopen(sentinel.c_str(), "wb");
		sentinel_created = file != nullptr;
		if (file != nullptr) {
			std::fputs("sentinel\n", file);
			std::fclose(file);
		}
	}

	bool const result = sentinel_created && !Delete_Files_Older_Than(directory, "DEBUG_*.LOG", 14);
	FILE * file = sentinel_created ? std::fopen(sentinel.c_str(), "rb") : nullptr;
	char contents[32] = {};
	if (file != nullptr) {
		std::fread(contents, 1, sizeof(contents) - 1, file);
		std::fclose(file);
	}
	Check(result && std::strcmp(contents, "sentinel\n") == 0,
		"disabled Apple pruning fails closed without deleting files");

	if (sentinel_created) {
		::unlink(sentinel.c_str());
	}
	if (directory_created) {
		::rmdir(directory);
	}
}


void Test_Last_Error_Text(void)
{
	char const * const first = Last_Error_Text(0);
	char const * const second = Last_Error_Text(0xFFFFFFFFul);
	Check(first != nullptr && second != nullptr && first == second && std::strlen(first) > 0,
		"Last_Error_Text is deterministic and non-null without Win32 semantics");
}

} // namespace


int main(void)
{
	std::printf("OpenTS Apple dbgprint contract\n\n");
	Debug_Init();

	Test_Formatting_And_Continuation();
	Test_Concurrent_Writers();
	Test_Errno();
	Test_No_Console_And_No_Path();
	Test_Fail_Closed_Pruning();
	Test_Last_Error_Text();

	std::printf("\n12 Apple dbgprint contract checks: %s\n",
		Failures == 0 ? "PASSED" : "FAILED");
	return(Failures == 0 ? 0 : 1);
}
