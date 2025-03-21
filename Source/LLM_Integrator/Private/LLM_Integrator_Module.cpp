// Copyright Epic Games, Inc. All Rights Reserved.

#include "LLM_Integrator_Module.h"

#define LOCTEXT_NAMESPACE "FLLM_IntegratorModule"

void FLLM_IntegratorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FLLM_IntegratorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLLM_IntegratorModule, LLM_Integrator)