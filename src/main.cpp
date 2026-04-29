#include "PCH.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include "PersistenceManager.h"
#include <variant>

using namespace RE;

#define DLLEXPORT __declspec(dllexport)

void OnSLMMenuExit(std::monostate)
{
	spdlog::info("SLM Exit triggered from Papyrus.");
	auto player = RE::PlayerCharacter::GetSingleton();
	PersistenceManager::GetSingleton()->SaveToWatchlist(player);
}

void ReloadWatchlist(std::monostate)
{
	spdlog::info("Reloading watchlist from Papyrus.");
	PersistenceManager::GetSingleton()->LoadWatchlist();
}

void RemoveFromWatchlist(std::monostate, uint32_t a_formID)
{
	spdlog::info("Removing form {:08X} from watchlist.", a_formID);
	PersistenceManager::GetSingleton()->RemoveFromWatchlist(a_formID);
}

bool F4SEAPI RegisterFunctions(RE::BSScript::IVirtualMachine* a_vm)
{
	a_vm->BindNativeMethod("UAPS"sv, "OnSLMMenuExit"sv, OnSLMMenuExit);
	a_vm->BindNativeMethod("UAPS"sv, "ReloadWatchlist"sv, ReloadWatchlist);
	a_vm->BindNativeMethod("UAPS"sv, "RemoveFromWatchlist"sv, RemoveFromWatchlist);

	return true;
}

void MessageHandler(F4SE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case F4SE::MessagingInterface::kGameDataReady:
		PersistenceManager::GetSingleton()->LoadWatchlist();
		PersistenceManager::GetSingleton()->InitializeHooks();
		PersistenceManager::GetSingleton()->RegisterMenuWatcher();
		break;
	}
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
	auto path = F4SE::log::log_directory();
	if (!path) return false;
	*path /= "UniversalAppearance.log"sv;

	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::warn);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	spdlog::info("Universal Appearance System loading...");

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = "UniversalAppearance";
	a_info->version = 1;

	if (a_f4se->IsEditor()) {
		spdlog::critical("Loaded in editor, marking as incompatible.");
		return false;
	}

	if (a_f4se->RuntimeVersion() < F4SE::RUNTIME_1_10_163) {
		spdlog::critical("Unsupported runtime version!");
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	spdlog::info("Universal Appearance System loaded.");

	F4SE::Init(a_f4se);

	auto papyrus = F4SE::GetPapyrusInterface();
	if (papyrus) {
		papyrus->Register(RegisterFunctions);
	}

	auto messaging = F4SE::GetMessagingInterface();
	if (messaging) {
		messaging->RegisterListener(MessageHandler);
	}

	return true;
}
