/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "ftimer.h"
#include "milsectmr.h"
#include "monotonic.h"
#include "mstimer.h"
#include "stimer.h"
#include "timer.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>


int Frame = 0;


namespace
{
	std::int64_t TestNow = 0;
	int Failures = 0;


	std::int64_t Fake_Monotonic_Clock(void)
	{
		return(TestNow);
	}


	class IntegerArchive
	{
	public:
		IntegerArchive(std::vector<int> & values, bool loading) : Values(values), Loading(loading) {}

		void Serialize(int & value)
		{
			if (Loading) {
				value = Values[Position++];
			} else {
				Values.push_back(value);
			}
		}

	private:
		std::vector<int> & Values;
		std::size_t Position = 0;
		bool Loading;
	};


	void Check(std::string const & name, bool condition)
	{
		if (!condition) {
			std::cerr << name << " failed\n";
			Failures++;
		}
	}


	template<typename Actual, typename Expected>
	void Check_Equal(std::string const & name, Actual actual, Expected expected)
	{
		if (actual != expected) {
			std::cerr << name << ": expected " << expected << ", got " << actual << '\n';
			Failures++;
		}
	}


	template<typename T>
	std::vector<int> Save(T & timer)
	{
		std::vector<int> values;
		IntegerArchive writer(values, false);
		timer.Serialize(writer);
		return(values);
	}


	template<typename T>
	void Load(T & timer, std::vector<int> & values)
	{
		IntegerArchive reader(values, true);
		timer.Serialize(reader);
	}


	void Test_Monotonic_Service(void)
	{
		TestNow = 41;
		Check_Equal("monotonic first reading", Monotonic_Milliseconds(), std::int64_t{41});
		TestNow = 42;
		Check("monotonic readings increase", Monotonic_Milliseconds() > std::int64_t{41});

		MillisecondTimerClass precise;
		TestNow = 57;
		Check_Equal("high-resolution timer follows monotonic service", static_cast<int>(static_cast<double>(precise)), 57);
	}


	void Test_System_Timer_Scale_And_Elapsed(void)
	{
		TestNow = 160;
		SystemTimerClass scaled;
		Check_Equal("system timer preserves sixteen-millisecond scale", static_cast<int>(scaled), 10);
		TestNow = 175;
		Check_Equal("system timer remains in scale bucket", static_cast<int>(scaled), 10);
		TestNow = 176;
		Check_Equal("system timer advances at existing scale", static_cast<int>(scaled), 11);

		TestNow = 100;
		BasicTimerClass<MillisecondSystemTimerClass> elapsed(7);
		TestNow = 132;
		Check_Equal("elapsed timer retains elapsed difference", elapsed.Value(), 39);
	}


	void Test_Frame_Timer_Serialization(void)
	{
		Frame = 100;
		BasicTimerClass<FrameTimerClass> saved(5);
		std::vector<int> values = Save(saved);
		Frame = 300;
		BasicTimerClass<FrameTimerClass> loaded;
		Load(loaded, values);
		Check_Equal("frame-origin timer retains raw frame anchor", loaded.Value(), 205);
	}


	void Test_Process_Timer_Rebase(void)
	{
		TestNow = 100;
		BasicTimerClass<MillisecondSystemTimerClass> saved(5);
		TestNow = 112;
		Check_Equal("process timer records pre-save elapsed", saved.Value(), 17);
		std::vector<int> values = Save(saved);

		TestNow = 1000;
		BasicTimerClass<MillisecondSystemTimerClass> loaded;
		Load(loaded, values);
		Check_Equal("process timer rebases after load", loaded.Value(), 17);
		TestNow = 1011;
		Check_Equal("rebased process timer keeps progressing", loaded.Value(), 28);
	}


	void Test_Count_Up_And_Stopped_Sentinel(void)
	{
		TestNow = 100;
		TTimerClass<MillisecondSystemTimerClass> saved(4);
		TestNow = 109;
		Check_Equal("count-up has expected active value", saved.Value(), 13);
		std::vector<int> values = Save(saved);

		TestNow = 1000;
		TTimerClass<MillisecondSystemTimerClass> loaded;
		Load(loaded, values);
		Check_Equal("count-up survives save rebase", loaded.Value(), 13);
		TestNow = 1007;
		Check_Equal("rebased count-up progresses", loaded.Value(), 20);

		TestNow = 109;
		saved.Stop();
		int const stopped_value = saved.Value();
		std::vector<int> stopped_values = Save(saved);
		TestNow = 2000;
		TTimerClass<MillisecondSystemTimerClass> stopped_loaded;
		Load(stopped_loaded, stopped_values);
		Check("stopped timer sentinel remains inactive", !stopped_loaded.Is_Active());
		Check_Equal("stopped timer retains accumulated value", stopped_loaded.Value(), stopped_value);
	}


	void Test_Countdown_Round_Trip(void)
	{
		TestNow = 100;
		CDTimerClass<MillisecondSystemTimerClass> saved(20);
		TestNow = 108;
		Check_Equal("countdown has expected pre-save remainder", saved.Value(), 12);
		std::vector<int> values = Save(saved);

		TestNow = 1000;
		CDTimerClass<MillisecondSystemTimerClass> loaded;
		Load(loaded, values);
		Check_Equal("countdown survives save rebase", loaded.Value(), 12);
		TestNow = 1005;
		Check_Equal("rebased countdown keeps counting", loaded.Value(), 7);
	}
}


int main(void)
{
	Monotonic_Set_Test_Clock(Fake_Monotonic_Clock);
	Test_Monotonic_Service();
	Test_System_Timer_Scale_And_Elapsed();
	Test_Frame_Timer_Serialization();
	Test_Process_Timer_Rebase();
	Test_Count_Up_And_Stopped_Sentinel();
	Test_Countdown_Round_Trip();
	Monotonic_Reset_Test_Clock();

	if (Failures != 0) {
		std::cerr << Failures << " timer persistence checks failed\n";
		return(1);
	}

	std::cout << "All timer persistence checks passed\n";
	return(0);
}
