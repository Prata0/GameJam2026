#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FStdCoreModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	FDelegateHandle ModifyCookHandle;
};
