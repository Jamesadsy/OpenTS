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
	std::string const mixer = Read("code/audio/audiomixer.cpp");
	std::string const task = Read("code/vqalib/task.cpp");
	std::string const vqa = Read("code/vqa.cpp");
	std::string const radar = Read("code/radar.cpp");
	std::string const movies = Read("code/movies.cpp");
	std::string const vqaAudio = Read("code/vqalib/audio.cpp");
	Check(!movie.empty() && !engine.empty(), "movie and engine lifecycle sources are readable");
	Check(!task.empty() && !vqa.empty() && !radar.empty() && !movies.empty(), "full-screen and radar-pane movie routes are readable");
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

	size_t const playback = task.find("long VQA_Play(VQAHandle *vqa, long mode, int flags)");
	size_t const playbackEnd = task.find("/* Movie is finished. */", playback);
	std::string const playbackStop = playback == std::string::npos || playbackEnd == std::string::npos
		? std::string() : task.substr(playback, playbackEnd - playback);
	Check(playbackStop.find("VQADATF_DDONE|VQADATF_LDONE") != std::string::npos
		&& playbackStop.find("mode = VQAMODE_STOP") != std::string::npos
		&& playbackStop.find("mode == VQAMODE_STOP") != std::string::npos
		&& playbackStop.find("VQA_StopAudio((VQAHandleP *)vqabuf)") != std::string::npos,
		"natural full-screen completion and explicit skip converge on the same VQA audio stop");
	Check(vqa.find("VQA_Play(Handle, VQAMODE_WALK, 0)") != std::string::npos
		&& vqa.find("bool VQAClass::Advance_Frame(bool & finished)") != std::string::npos,
		"the radar-pane cameo advances through the same VQA playback stop path");
	Check(radar.find("Movie_Advance_Frame(handle, needs_volume_adjustment)") != std::string::npos
		&& radar.find("Movie_Destroy(handle)") != std::string::npos,
		"the radar-pane cameo closes its VQA handle after natural completion");
	Check(movies.find("handle->VQA->Close_And_Free_VQA()") != std::string::npos,
		"an early cameo close uses VQA close semantics rather than a separate skip path");
	size_t const closeAudio = vqaAudio.find("void VQA_CloseAudio(VQAHandleP *vqap)");
	size_t const closeAudioEnd = vqaAudio.find("/************************************************", closeAudio);
	std::string const closeAudioBody = closeAudio == std::string::npos || closeAudioEnd == std::string::npos
		? std::string() : vqaAudio.substr(closeAudio, closeAudioEnd - closeAudio);
	Check(task.find("VQA_CloseAudio((VQAHandleP *)vqap)") != std::string::npos
		&& closeAudioBody.find("VQA_StopAudio(vqap)") != std::string::npos,
		"closing a still-playing cameo invokes the common bounded movie stop handler");
	size_t const resamplerReset = mixer.find("ma_linear_resampler_reset(&voice.Resampler)");
	size_t const playing = mixer.find("voice.State.store(AudioVoiceState::PLAYING, std::memory_order_release)", resamplerReset);
	Check(resamplerReset != std::string::npos && playing != std::string::npos && resamplerReset < playing,
		"every newly started source resets retained voice resampler history");
}

} // namespace


int main(void)
{
	Test_Movie_Stop_Ordering();
	std::printf("movie-audio-quiesce-contract: %d checks, %d failures\n", Checks, Failures);
	return(Failures == 0 ? 0 : 1);
}
