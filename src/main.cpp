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
	spdlog::info("ReloadWatchlist from Papyrus: Reloading JSON data.");
	PersistenceManager::GetSingleton()->LoadFromJson();
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
		PersistenceManager::GetSingleton()->InitializeHooks();
		break;
	}
}

extern "C" DLLEXPORT constinit auto F4SEPlugin_Version = []() constexpr {
	F4SE::PluginVersionData v{};
	v.PluginVersion({ 1, 0, 0, 0 });
	v.PluginName("UniversalAppearance");
	v.AuthorName("arifkulpu");
	v.UsesAddressLibrary(true);
	v.UsesSigScanning(false);
	v.IsLayoutDependent(true);
	v.HasNoStructUse(false);
	v.CompatibleVersions({ 
		REL::Version{ 1, 10, 163, 0 }, // Old Gen (1.10.163)
		REL::Version{ 1, 10, 980, 0 }, // Next Gen 1 (1.10.980)
		REL::Version{ 1, 10, 984, 0 }, // Next Gen 1 Update (1.10.984)
		REL::Version{ 1, 11, 191, 0 }, // Next Gen 2 (1.11.191)
		REL::Version{ 1, 11, 193, 0 }  // Next Gen 2 Update (1.11.193)
	});
	return v;
}();

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

	auto runtimeVersion = a_f4se->RuntimeVersion();
	spdlog::info("Universal Appearance System loading...");
	spdlog::info("Runtime Version: {}.{}.{}.{}", runtimeVersion.major(), runtimeVersion.minor(), runtimeVersion.patch(), runtimeVersion.build());
	spdlog::info("F4SE Version: {}.{}.{}", a_f4se->F4SEVersion().major(), a_f4se->F4SEVersion().minor(), a_f4se->F4SEVersion().patch());

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = "UniversalAppearance";
	a_info->version = 1;

	if (a_f4se->IsEditor()) {
		spdlog::critical("Loaded in editor, marking as incompatible.");
		return false;
	}

	if (runtimeVersion < REL::Version{ 1, 10, 163, 0 }) {
		spdlog::critical("Unsupported runtime version! Minimum required is 1.10.163.");
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

	auto serialization = F4SE::GetSerializationInterface();
	if (serialization) {
		serialization->SetUniqueID('UAPS');
		serialization->SetSaveCallback([](const F4SE::SerializationInterface* a_intfc) {
			PersistenceManager::GetSingleton()->SaveCallback(a_intfc);
		});
		serialization->SetLoadCallback([](const F4SE::SerializationInterface* a_intfc) {
			PersistenceManager::GetSingleton()->LoadCallback(a_intfc);
		});
		serialization->SetRevertCallback([](const F4SE::SerializationInterface* a_intfc) {
			PersistenceManager::GetSingleton()->RevertCallback(a_intfc);
		});
		spdlog::info("Registered F4SE Serialization Callbacks.");
	}

	return true;
}
