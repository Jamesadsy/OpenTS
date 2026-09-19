/*******************************************************************************
 *                                O P E N  T S
 ******************************************************************************/

// GameDirs only observes logging as diagnostics.  Keep this deterministic harness free of
// host log-directory policy while exercising the actual directory implementation.
#include "dbgprint.h"

void Debug_Init(void) {}
void Debug_Init_Console(void) {}
void Debug_Console_Hold(void) {}
char const * Debug_Log_File_Name(void) { return(""); }
char const * Debug_Directory(void) { return(""); }
bool Delete_Files_Older_Than(char const *, char const *, unsigned) { return(false); }
void __cdecl DebugString(char const *, ...) {}
void __cdecl DebugStringNoPrefix(char const *, ...) {}
char const * Last_Error_Text(unsigned long) { return(""); }
