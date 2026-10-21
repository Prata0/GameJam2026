#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

STDCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogStdCore, Log, All);

namespace StdLog
{
	/**
	 * Logs a warning the first time a given key is seen, and never again for the rest of the process.
	 * Used for missing assets and settings, which would otherwise repeat on every map load or every frame.
	 */
	STDCORE_API void WarnOnce(const FString& Key, const FString& Message);
}
