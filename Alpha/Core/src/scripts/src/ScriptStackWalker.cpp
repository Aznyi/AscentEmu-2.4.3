#ifdef WIN32

#include <windows.h>
#include <DbgHelp.h>
#include "StackWalker.h"

class CStackWalker : public StackWalker
{
public:
	void OnOutput(LPCSTR szText);
	void OnSymInit(LPCSTR szSearchPath, DWORD symOptions, LPCSTR szUserName);
	void OnLoadModule(LPCSTR img, LPCSTR mod, DWORD64 baseAddr, DWORD size, DWORD result, LPCSTR symType, LPCSTR pdbName, ULONGLONG fileVersion);
	void OnCallstackEntry(CallstackEntryType eType, CallstackEntry &entry);
	void OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr);
};

// Script DLLs only need the callback vtable that StackWalker expects.
// The world executable owns the full crash-reporting path.
void CStackWalker::OnOutput(LPCSTR)
{
}

void CStackWalker::OnSymInit(LPCSTR, DWORD, LPCSTR)
{
}

void CStackWalker::OnLoadModule(LPCSTR, LPCSTR, DWORD64, DWORD, DWORD, LPCSTR, LPCSTR, ULONGLONG)
{
}

void CStackWalker::OnCallstackEntry(CallstackEntryType, CallstackEntry &)
{
}

void CStackWalker::OnDbgHelpErr(LPCSTR, DWORD, DWORD64)
{
}

#endif
