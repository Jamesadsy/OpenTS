/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Source contract for the VQA movie renderer's bounded quiesce and safe ring
// lifetime. It complements the mixer output tests; it does not claim physical
// browser audio silence.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

namespace
{

int Failures = 0;
int Checks = 0;


void Check(bool condition, char const * description)
{
	Checks++;
	if (!condition) {
		Failures++;
	}
	std::printf("%-76s %s\n", description, condition ? "ok" : "FAILED");
}


std::string Read(char const * relative)
{
	std::ifstream source(std::string(OPENTS_SOURCE_DIR) + "/" + relative);
	return(std::string(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>()));
}


void Test_Movie_Stop_Ordering(void)
{
	std::string const movie = Read("code/audio/audiomovie.cpp");
	std::string const engine = Read("code/audio/audioengine.cpp");
	Check(!movie.empty() && !engine.empty(), "movie and engine lifecycle sources are readable");
	Check(movie.find("int const QUIESCE_RAMP_MS = 32;") != std::string::npos,
		"movie skip requests a block-sized, localized renderer ramp");
	Check(movie.find("int const QUIESCE_WAIT_MS = 250;") != std::string::npos,
		"renderer acknowledgement has a short bounded wait");

	size_t const stop = movie.find("long Stop_Audio_Handler(VQAHandleP * vqap)");
	size_t const close = movie.find("long Close_Audio_Handler(VQAHandleP * vqap)", stop);
	std::string const stop_body = stop == std::string::npos || close == std::string::npos
		? std::string() : movie.substr(stop, close - stop);
	size_t const detach = stop_body.find("Feeder_Ref().Detach");
	size_t const request = stop_body.find("Events().Fade(_sink.Handle, QUIESCE_RAMP_MS)");
	size_t const wait = stop_body.find("Wait_Finished(_sink.Handle, QUIESCE_WAIT_MS)");
	size_t const reset = stop_body.find("_sink.Stream->Reset()");
	Check(detach < request && request < wait && wait < reset,
		"movie production stops, renderer quiesces and acknowledges before ring reset");
	Check(stop_body.find("if (!quiesced)") < reset,
		"timeout branch retains ring storage instead of destroying a live renderer source");
	Check(stop_body.find("_sink.FinalPlaybackFrame = _sink.Stream->Frames_Consumed()") != std::string::npos,
		"movie stop snapshots its final consumed playback frame");
	Check(close != std::string::npos
		&& movie.find("Release_Stream_Slot(_sink.Slot, _sink.QuiesceTimedOut ? _sink.Handle : AudioHandle())", close)
			!= std::string::npos,
		"movie close passes a timed-out voice to deferred slot release");

	size_t const release = engine.find("void AudioEngineClass::Release_Stream_Slot(int slot, AudioHandle retiringRenderer)");
	size_t const reap = engine.find("void AudioEngineClass::Reap_Streams(void)");
	Check(release != std::string::npos && reap != std::string::npos,
		"the shared stream owner supports delayed renderer release");
	Check(engine.find("!Pool.Is_Finished(retiringRenderer)", release) != std::string::npos,
		"a timed-out renderer keeps its source slot reserved until its terminal acknowledgement");
	Check(engine.find("ReleasePending\n\t\t\t&& Pool.Is_Finished(Streams[i].RetiringHandle)", reap) != std::string::npos,
		"the engine reclaims a retained movie slot only after the render owner is finished");
	Check(movie.find("return(VQAERR_NONE);", stop) != std::string::npos,
		"a quiesce timeout does not change native skip's nonblocking success semantics");
}

} // namespace


int main(void)
{
	Test_Movie_Stop_Ordering();
	std::printf("movie-audio-quiesce-contract: %d checks, %d failures\n", Checks, Failures);
	return(Failures == 0 ? 0 : 1);
}
