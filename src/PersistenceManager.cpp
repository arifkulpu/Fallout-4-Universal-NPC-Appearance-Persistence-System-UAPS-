#include "PCH.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include "PersistenceManager.h"
#include <fstream>

void PersistenceManager::LoadWatchlist()
{
	if (!fs::exists(_storagePath)) {
		fs::create_directories(_storagePath);
		return;
	}

	auto dataHandler = RE::TESDataHandler::GetSingleton();

	for (const auto& entry : fs::directory_iterator(_storagePath)) {
		if (entry.path().extension() == ".json") {
			try {
				std::ifstream i(entry.path());
				nlohmann::json j;
				i >> j;

				std::string plugin = j["plugin"];
				uint32_t localID = j["localID"];
				std::string npcName = j.contains("name") ? j["name"].get<std::string>() : "Unknown";

				// Custom lookup
				RE::TESForm* form = nullptr;
				for (auto file : dataHandler->files) {
					if (file && _stricmp(file->filename, plugin.c_str()) == 0) {
						uint32_t globalID = 0;
						if (file->compileIndex != 0xFF) {
							globalID = (file->compileIndex << 24) | (localID & 0x00FFFFFF);
						} else if (file->smallFileCompileIndex != 0xFFFF) {
							globalID = 0xFE000000 | (file->smallFileCompileIndex << 12) | (localID & 0x00000FFF);
						} else {
							continue;
						}
						form = RE::TESForm::GetFormByID(globalID);
						break;
					}
				}

				if (form) {
					_watchlist.insert(form->formID);
					spdlog::info("Tracking NPC: {} ({:08X})", npcName, form->formID);
				}
			} catch (const std::exception& e) {
				spdlog::error("Failed to load NPC data {}: {}", entry.path().string(), e.what());
			}
		}
	}
}

void PersistenceManager::RemoveFromWatchlist(std::uint32_t a_formID)
{
	auto it = _watchlist.find(a_formID);
	if (it != _watchlist.end()) {
		_watchlist.erase(it);

		auto form = RE::TESForm::GetFormByID(a_formID);
		if (form) {
			auto npc = form->As<RE::TESNPC>();
			if (npc) {
				auto file = npc->GetFile(0);
				std::string pluginName = file ? std::string(file->filename) : "Fallout4.esm";
				uint32_t localID = file ? (a_formID & 0x00FFFFFF) : a_formID;
				std::string fileName = fmt::format("{}_{:08X}.json", pluginName, localID);
				fs::remove(_storagePath / fileName);
				
				auto actor = RE::TESForm::GetFormByID<RE::Actor>(a_formID);
				if (actor) {
					actor->formFlags &= ~0x400; // Unset Persistent flag
				}
				
				spdlog::info("Removed NPC {} from watchlist and deleted data.", npc->GetFullName());
			}
		}
	}
}

void PersistenceManager::SaveToWatchlist(RE::Actor* a_actor)
{
	if (!a_actor) return;

	RE::TESNPC* npc = a_actor->GetNPC();
	if (!npc) return;

	nlohmann::json j;

	auto formID = npc->formID;
	auto file = npc->GetFile(0);
	std::string pluginName = file ? std::string(file->filename) : "Fallout4.esm";
	uint32_t localID = file ? (formID & 0x00FFFFFF) : formID;

	j["plugin"] = pluginName;
	j["localID"] = localID;
	j["name"] = npc->GetFullName();

	// Morphs
	if (npc->morphSliderValues) {
		auto& morphs = j["morphs"];
		for (auto& entry : *npc->morphSliderValues) {
			morphs[std::to_string(entry.first)] = entry.second;
		}
	}

	std::string fileName = fmt::format("{}_{:08X}.json", pluginName, localID);
	std::ofstream o(_storagePath / fileName);
	o << j.dump(4);

	_watchlist.insert(formID);
	
	a_actor->formFlags |= 0x400; // Set Persistent
	
	spdlog::info("Saved NPC {} to watchlist.", npc->GetFullName());
}

bool PersistenceManager::IsInWatchlist(std::uint32_t a_formID)
{
	return _watchlist.find(a_formID) != _watchlist.end();
}

void PersistenceManager::ApplyAppearance(RE::Actor* a_actor)
{
	if (!a_actor) return;
	
	RE::TESNPC* npc = a_actor->GetNPC();
	if (!npc || _watchlist.find(npc->formID) == _watchlist.end()) return;

	auto formID = npc->formID;
	auto file = npc->GetFile(0);
	std::string pluginName = file ? std::string(file->filename) : "Fallout4.esm";
	uint32_t localID = file ? (formID & 0x00FFFFFF) : formID;
	std::string fileName = fmt::format("{}_{:08X}.json", pluginName, localID);
	
	fs::path path = _storagePath / fileName;
	if (!fs::exists(path)) return;

	try {
		std::ifstream i(path);
		nlohmann::json j;
		i >> j;

		spdlog::info("Applying appearance to NPC: {}", npc->GetFullName());

		if (j.contains("morphs") && npc->morphSliderValues) {
			auto& morphData = j["morphs"];
			for (auto& [key, val] : morphData.items()) {
				uint32_t morphID = std::stoul(key);
				npc->morphSliderValues->insert({ morphID, val.get<float>() });
			}
		}

		// Refresh 3D
		a_actor->Reset3D(true, 0, false, 0);
		a_actor->formFlags |= 0x400; // Ensure Persistent

	} catch (const std::exception& e) {
		spdlog::error("Error applying appearance for {}: {}", npc->GetFullName(), e.what());
	}
}

void PersistenceManager::InitializeHooks()
{
	spdlog::info("PersistenceManager hooks initialized.");
}

class MenuWatcher : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
	virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source) override
	{
		if (a_event.menuName == "RaceSexMenu"sv && !a_event.opening) {
			spdlog::info("RaceSexMenu closed. Checking for changes...");
			
			// Önce oyuncuyu kontrol et
			auto player = RE::PlayerCharacter::GetSingleton();
			if (player) {
				PersistenceManager::GetSingleton()->SaveToWatchlist(player);
			}

			// Eğer konsolda bir NPC seçiliyse (genelde slm komutu için seçilir), onu da kaydet
			auto console = RE::Console::GetPickRef();
			if (console) {
				auto actor = console.get()->As<RE::Actor>();
				if (actor && actor != player) {
					spdlog::info("Detected NPC edit via console: {:08X}. Saving...", actor->formID);
					PersistenceManager::GetSingleton()->SaveToWatchlist(actor);
				}
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	static void Register()
	{
		auto ui = RE::UI::GetSingleton();
		if (ui) {
			ui->GetEventSource<RE::MenuOpenCloseEvent>()->RegisterSink(new MenuWatcher());
			spdlog::info("Registered RaceSexMenu watcher.");
		}
	}
};

void PersistenceManager::RegisterMenuWatcher()
{
	MenuWatcher::Register();
}
