#include "ProximaModule.h"

DEFINE_LOG_CATEGORY(LogProxima);
IMPLEMENT_PRIMARY_GAME_MODULE(FProximaModule, Proxima, "Proxima");

void FProximaModule::StartupModule()
{
    FDefaultGameModuleImpl::StartupModule();
}

void FProximaModule::ShutdownModule()
{
    FDefaultGameModuleImpl::ShutdownModule();
}
