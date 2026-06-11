// Copyright Epic Games, Inc. All Rights Reserved.

#include "GF_RimaV2RuntimeModule.h"

#define LOCTEXT_NAMESPACE "FGF_RimaV2RuntimeModule"

void FGF_RimaV2RuntimeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory;
	// the exact timing is specified in the .uplugin file per-module
}

void FGF_RimaV2RuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.
	// For modules that support dynamic reloading, we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGF_RimaV2RuntimeModule, GF_RimaV2Runtime)
