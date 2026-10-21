#include "StdLog.h"

DEFINE_LOG_CATEGORY(LogStdCore);

void StdLog::WarnOnce(const FString& Key, const FString& Message)
{
	static TSet<FString> WarnedKeys;
	static FCriticalSection Lock;

	{
		FScopeLock ScopeLock(&Lock);
		bool bAlreadyWarned = false;
		WarnedKeys.Add(Key, &bAlreadyWarned);
		if (bAlreadyWarned)
		{
			return;
		}
	}

	UE_LOG(LogStdCore, Warning, TEXT("%s"), *Message);
}
