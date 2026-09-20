/*******************************************************************************
 *                                O P E N  T S
 ******************************************************************************/

// Synthetic, no-owner-data proof for the retained DATADIR/USERDIR contract.

#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "cdfile.h"
#include "deploymentconfig.h"
#include "gamedirs.h"
#include "rawfile.h"

namespace {

int Failures = 0;
std::filesystem::path Root;
std::filesystem::path OriginalDirectory;

std::string Separator(void) { return(std::string(1, std::filesystem::path::preferred_separator)); }
std::string Name(std::filesystem::path const & path) { return(path.string()); }

void Check(bool condition, char const * what)
{
	std::printf("%-62s %s\n", what, condition ? "ok" : "FAILED");
	if (!condition) Failures++;
}

void Check_List(std::vector<std::string> const & actual, std::vector<std::string> const & expected, char const * what)
{
	Check(actual == expected, what);
}

void Write_File(std::filesystem::path const & path, char const * contents)
{
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	file << contents;
}

std::string Read_File(std::string const & path)
{
	RawFileClass file(path.c_str());
	if (!file.Is_Available()) return(std::string());
	int const size = file.Size();
	std::string contents(size, '\0');
	file.Open(FileClass::READ);
	file.Read(contents.data(), size);
	file.Close();
	return(contents);
}

bool File_Exists(std::filesystem::path const & path)
{
	std::error_code error;
	return(std::filesystem::exists(path, error));
}

void Reset(void)
{
	CDFileClass::Clear_Search_Drives();
	Set_Data_Directory("");
	Set_User_Directory("");
	std::filesystem::current_path(Root);
}

std::string Default_List(void) { return(DeploymentConfigClass().SearchPaths); }

void Test_Parsing_And_Folders(void)
{
	std::string const sep = Separator();
	std::string const duplicate = " INI , ini" + sep + ",MIX";
	Check_List(Parse_Search_Folders("INI,MIX"), {"INI" + sep, "MIX" + sep}, "a plain list keeps its order");
	Check_List(Parse_Search_Folders(duplicate.c_str()), {"INI" + sep, "MIX" + sep}, "folders are trimmed and case-insensitively deduplicated");
	Check_List(Parse_Search_Folders("INI,,MIX"), {"INI" + sep, "MIX" + sep}, "an empty entry is passed over");
	Check_List(Parse_Search_Folders("."), {}, "the current directory is not registered twice");

	Reset();
	Init_Search_Folders(Default_List().c_str());
	Check(CDFileClass::Search_Path(0) != NULL && std::string(CDFileClass::Search_Path(0)) == "INI" + sep,
		"the default INI folder is searched first");
	Check(CDFileClass::Search_Path(1) != NULL && std::string(CDFileClass::Search_Path(1)) == "MIX" + sep,
		"the default MIX folder is searched second");

	Reset();
	Set_Data_Directory(Name(Root / "Data").c_str());
	Check(Apply_Game_Directories(), "an existing data directory is accepted");
	Init_Search_Folders("Sorted,sorted");
	std::string const data = Name(Root / "Data") + sep;
	Check(CDFileClass::Search_Path(0) != NULL && std::string(CDFileClass::Search_Path(0)) == data,
		"the data directory itself is searched");
	Check(CDFileClass::Search_Path(1) != NULL && std::string(CDFileClass::Search_Path(1)) == data + "Sorted" + sep,
		"configured folders are relative to data and retain one entry");

	Reset();
	Set_Data_Directory(Name(Root / "Missing").c_str());
	Check(!Apply_Game_Directories() && std::strlen(Game_Directory_Error()) != 0,
		"a missing data directory fails closed with an error");
}


void Test_Bundle_UI_Search_Path(void)
{
	std::string const sep = Separator();
	Reset();
	Set_Data_Directory(Name(Root / "Data").c_str());
	Check(Apply_Game_Directories(), "bundle UI proof accepts a separate external data directory");

	Write_File(Root / "ui" / "LatoLatin-Regular.ttf", "font");
	Write_File(Root / "ui" / "options.rml", "options");
	Write_File(Root / "ui" / "campaign.rml", "campaign");
	Write_File(Root / "ui" / "mainmenu.rml", "main menu");
	Write_File(Root / "ui" / "optionsbase.rcss", "styles");
	Init_Bundle_UI_Search_Path();

	std::string const data = Name(Root / "Data") + sep;
	std::string const ui = "ui" + sep;
	Check(CDFileClass::Search_Path(0) != NULL && std::string(CDFileClass::Search_Path(0)) == data,
		"bundle UI registration leaves DATADIR as the first external search path");
	Check(CDFileClass::Search_Path(1) != NULL && std::string(CDFileClass::Search_Path(1)) == ui,
		"bundle UI registration keeps the required trailing separator");

	for (char const * name : {
		"LatoLatin-Regular.ttf", "options.rml", "campaign.rml", "mainmenu.rml", "optionsbase.rcss"}) {
		CDFileClass file(name);
		Check(file.Is_Available() && std::string(file.File_Name()) == ui + name,
			"bare-name shipped UI resource resolves from bundle ui/");
	}

	Init_Bundle_UI_Search_Path();
	int registrations = 0;
	for (int index = 0; CDFileClass::Search_Path(index) != NULL; index++) {
		if (std::string(CDFileClass::Search_Path(index)) == ui) registrations++;
	}
	Check(registrations == 1, "bundle UI search registration is idempotent");
}

void Test_User_Files(void)
{
	std::string const sep = Separator();
	Reset();
	std::filesystem::path const user = Root / "User" / "Fresh";
	Set_User_Directory(Name(user).c_str());
	Check(Apply_Game_Directories(), "an absent user directory is created");
	Check(File_Exists(user), "the user directory is created on disk");
	std::string const own = Name(user) + sep;
	Check(CDFileClass::User_Path() != NULL && std::string(CDFileClass::User_Path()) == own,
		"the user directory is registered with the file layer");
	Check(User_File_Write_Name("SUN.INI") == own + "SUN.INI", "user file writes are placed under USERDIR");

	Init_Search_Folders(Default_List().c_str());
	Write_File(Root / "MIX" / "OWN.DAT", "shipped");
	CDFileClass written("OWN.DAT");
	written.Open(FileClass::WRITE);
	written.Write("mine", 4);
	written.Close();
	Check(File_Exists(user / "OWN.DAT"), "a player write creates a player copy");
	Check(Read_File(Name(Root / "MIX" / "OWN.DAT")) == "shipped", "a player write leaves shipped data unchanged");
	CDFileClass deleted("OWN.DAT");
	deleted.Delete();
	Check(!File_Exists(user / "OWN.DAT") && File_Exists(Root / "MIX" / "OWN.DAT"),
		"a player delete never deletes the shipped copy");

	Reset();
	Check(CDFileClass::User_Path() == NULL && User_File_Write_Name("SUN.INI") == "SUN.INI",
		"an unnamed user directory retains current-directory behavior");
}

void Test_Enumeration(void)
{
	Reset();
	Write_File(Root / "ALPHA.MPR", "game");
	Write_File(Root / "INI" / "BRAVO.MPR", "ini");
	Write_File(Root / "INI" / "ALPHA.MPR", "duplicate");
	Write_File(Root / "MIX" / "CHARLIE.MPR", "mix");
	Write_File(Root / "MIX" / ".HIDDEN.MPR", "hidden");
	std::filesystem::create_directory(Root / "MIX" / "DIRECTORY.MPR");
	Init_Search_Folders(Default_List().c_str());
	std::vector<std::string> expected = {"ALPHA.MPR", "BRAVO.MPR", "CHARLIE.MPR"};
#ifdef _WIN32
	expected.insert(expected.begin(), ".HIDDEN.MPR");
#endif
	Check_List(Search_Files("*.MPR"), expected,
		"wildcard scans return regular visible files once in deterministic order");

	std::filesystem::path const user = Root / "User" / "Own";
	Set_User_Directory(Name(user).c_str());
	Apply_Game_Directories();
	Write_File(user / "CHARLIE.MPR", "own");
	Check_List(Search_Files("c?arlie.mpr"), {"CHARLIE.MPR"}, "wildcard matching is case-insensitive");
	CDFileClass file("CHARLIE.MPR");
	Check(Read_File(file.File_Name()) == "own", "the player copy wins the same search precedence as a scan");
}

void Test_Saved_Games_And_Repeatability(void)
{
	std::string const sep = Separator();
	Reset();
	Check(Saved_Game_Name("SAVE0001.SAV") == "Saved Games" + sep + "SAVE0001.SAV",
		"saved games use a dedicated child of the current directory");
	Check(File_Exists(Root / "Saved Games"), "the saved games child is created when needed");

	Reset();
	std::filesystem::path const user = Root / "User" / "Saves";
	Set_User_Directory(Name(user).c_str());
	Apply_Game_Directories();
	Check(Saved_Game_Name("SAVE0002.SAV") == Name(user / "Saved Games") + sep + "SAVE0002.SAV",
		"saved games move beneath USERDIR");
	Check(File_Exists(user / "Saved Games"), "the USERDIR saved games child is created");
	CDFileClass file("AGAIN.DAT");
	file.Open(FileClass::WRITE);
	file.Close();
	std::string const once = file.File_Name();
	file.Open(FileClass::WRITE);
	file.Close();
	Check(std::string(file.File_Name()) == once, "player file placement is repeatable");
}

bool Make_Root(void)
{
	OriginalDirectory = std::filesystem::current_path();
	Root = std::filesystem::temp_directory_path() / ("opents-gamedirs-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	std::error_code error;
	for (char const * folder : {"INI", "MIX", "Maps", "Data", "User", "ui"}) std::filesystem::create_directories(Root / folder, error);
	std::filesystem::current_path(Root, error);
	return(!error);
}

void Remove_Root(void)
{
	std::error_code error;
	std::filesystem::current_path(OriginalDirectory, error);
	std::filesystem::remove_all(Root, error);
}

}

int main(void)
{
	if (!Make_Root()) return(1);
	Test_Parsing_And_Folders();
	Test_Bundle_UI_Search_Path();
	Test_User_Files();
	Test_Enumeration();
	Test_Saved_Games_And_Repeatability();
	Reset();
	Remove_Root();
	std::printf("%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
