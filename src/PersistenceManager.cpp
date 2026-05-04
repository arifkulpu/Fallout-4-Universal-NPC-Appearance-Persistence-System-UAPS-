#include "PCH.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include "PersistenceManager.h"
#include "RE/Bethesda/TESBoundAnimObjects.h"
#include "RE/Bethesda/Actor.h"
#include "RE/Bethesda/TESDataHandler.h"
#include "RE/Bethesda/BGSHeadPart.h"
#include "RE/Bethesda/BSScript/Internal/VirtualMachine.h"
#include "RE/Bethesda/BSScriptUtil.h"
#include "F4SE/API.h"
#include <fstream>
#include <thread>

constexpr auto UAPS_JSON_FILE = "Data/F4SE/Plugins/UAPS_Data.json";

namespace RE {
	namespace BGSCharacterTint {
		enum class TintType : uint32_t {
			kPalette = 0,
			kTexture = 1
		};

		struct Entry {
			virtual ~Entry() = default;
			virtual TintType GetType() const = 0;

			uint16_t index;
			uint8_t  pad0A[2];
		};

		struct PaletteEntry : public Entry {
			virtual TintType GetType() const override { return TintType::kPalette; }
			uint32_t color;
			float    alpha;
		};

		struct TextureEntry : public Entry {
			virtual TintType GetType() const override { return TintType::kTexture; }
			// Texture entries might have more data, but index/alpha are often enough
			float alpha;
		};

		struct Entries {
			BSSimpleList<Entry*> entries; 
		};
	}
}

// ... [Keep previous FormIDToString, StringToFormID, ColorToHex, HexToColor, ComputeHash, SaveToWatchlist, RemoveFromWatchlist, IsInWatchlist]

std::string PersistenceManager::FormIDToString(uint32_t a_formID) {
	auto form = RE::TESForm::GetFormByID(a_formID);
	if (form) {
		auto file = form->GetFile(0);
		if (file) {
			uint32_t localID = form->GetLocalFormID();
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "%s|%08X", file->filename, localID);
			return std::string(buffer);
		}
	}
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%08X", a_formID);
	return std::string(buffer);
}

uint32_t PersistenceManager::StringToFormID(const std::string& a_str) {
	auto pos = a_str.find('|');
	if (pos == std::string::npos) {
		try { return std::stoul(a_str, nullptr, 16); } catch (...) { return 0; }
	}
	std::string modName = a_str.substr(0, pos);
	std::string idStr = a_str.substr(pos + 1);
	auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) return 0;
	try {
		uint32_t localID = std::stoul(idStr, nullptr, 16);
		return dataHandler->LookupFormID(localID, modName);
	} catch (...) { return 0; }
}

std::string PersistenceManager::ColorToHex(uint32_t a_color) {
	char buffer[16];
	snprintf(buffer, sizeof(buffer), "#%06X", a_color & 0xFFFFFF);
	return std::string(buffer);
}

uint32_t PersistenceManager::HexToColor(const std::string& a_hex) {
	if (a_hex.empty() || a_hex[0] != '#') return 0;
	try { return std::stoul(a_hex.substr(1), nullptr, 16); } catch (...) { return 0; }
}

uint64_t PersistenceManager::ComputeHash(const NPC_Appearance& data) {
	uint64_t hash = 14695981039346656037ULL;
	auto fnv = [&hash](const void* d, size_t size) {
		const uint8_t* ptr = static_cast<const uint8_t*>(d);
		for (size_t i = 0; i < size; ++i) {
			hash ^= ptr[i];
			hash *= 1099511628211ULL;
		}
	};
	fnv(data.skinColor.data(), data.skinColor.size());
	fnv(data.assets.hairID.data(), data.assets.hairID.size());
	fnv(data.assets.eyesID.data(), data.assets.eyesID.size());
	fnv(data.geometry.morphs.data(), data.geometry.morphs.size() * sizeof(float));
	fnv(data.geometry.weights.data(), data.geometry.weights.size() * sizeof(float));
	for (const auto& t : data.tints) {
		fnv(&t.index, sizeof(t.index));
		fnv(&t.color, sizeof(t.color));
		fnv(&t.alpha, sizeof(t.alpha));
		fnv(&t.type, sizeof(t.type));
	}
	return hash;
}

void PersistenceManager::SaveToWatchlist(RE::Actor* a_actor) {
	if (!a_actor) return;
	RE::TESNPC* npc = a_actor->GetNPC();
	if (!npc) return;

	std::lock_guard<std::mutex> lock(_lock);
	NPC_Appearance data;
	
	if (npc->IsUnique()) data.type = "Unique";
	else data.type = (a_actor->GetFormID() >= 0xFF000000) ? "Settler" : "Raider";
	data.name = npc->GetFullName();

	uint32_t sColor = (npc->bodyTintColorR << 16) | (npc->bodyTintColorG << 8) | npc->bodyTintColorB;
	data.skinColor = ColorToHex(sColor);
	
	// Appearance Source (faceNPC) logic
	if (npc->faceNPC) {
		data.appearanceSource = FormIDToString(npc->faceNPC->formID);
		spdlog::info("Detected appearance source (Template): {} for NPC: {}", data.appearanceSource, data.name);
	} else {
		data.appearanceSource = "Self";
	}
	
	if (npc->morphSliderValues) {
		for (auto& entry : *npc->morphSliderValues) {
			data.geometry.morphs.push_back(entry.second);
		}
	}

	for (auto hp : npc->GetHeadParts()) {
		if (hp) {
			std::string hpStr = FormIDToString(hp->formID);
			if (hpStr.find("Hair") != std::string::npos) data.assets.hairID = hpStr;
			else if (hpStr.find("Eyes") != std::string::npos) data.assets.eyesID = hpStr;
			else data.assets.otherHeadParts.push_back(hpStr);
		}
	}

	if (npc->tintingData) {
		auto entriesList = reinterpret_cast<RE::BGSCharacterTint::Entries*>(npc->tintingData);
		for (auto entry : entriesList->entries) {
			if (entry) {
				TintData td;
				td.index = entry->index;
				td.type = static_cast<uint32_t>(entry->GetType());
				if (td.type == 0) { // Palette
					auto palette = static_cast<RE::BGSCharacterTint::PaletteEntry*>(entry);
					td.color = palette->color;
					td.alpha = palette->alpha;
				} else { // Texture
					auto texture = static_cast<RE::BGSCharacterTint::TextureEntry*>(entry);
					td.alpha = texture->alpha;
					td.color = 0;
				}
				data.tints.push_back(td);
			}
		}
		spdlog::info("Saved {} tints for NPC: {}", data.tints.size(), data.name);
	}

	data.appearanceHash = ComputeHash(data);
	uint32_t storeID = (data.type == "Unique") ? npc->formID : a_actor->GetFormID();
	_npcData[storeID] = data;
	
	spdlog::info("Saved appearance for NPC: {} ({})", data.name, FormIDToString(storeID));
	SaveToJson();
}

void PersistenceManager::RemoveFromWatchlist(std::uint32_t a_formID) {
	std::lock_guard<std::mutex> lock(_lock);
	_npcData.erase(a_formID);
	_runtimeCache.erase(a_formID);
	spdlog::info("Removed NPC: {:08X}", a_formID);
	SaveToJson();
}

bool PersistenceManager::IsInWatchlist(std::uint32_t a_formID) {
	std::lock_guard<std::mutex> lock(_lock);
	return _npcData.contains(a_formID);
}

// ------------------------------------------------------------
// SAFE PAPYRUS INVOCATION HELPERS
// ------------------------------------------------------------
static void CallPapyrusChangeHeadPart(RE::Actor* a_actor, RE::BGSHeadPart* a_part)
{
	auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	if (!vm || !a_actor || !a_part) return;

	auto& policy = vm->GetObjectHandlePolicy();

	uint64_t handle = policy.GetHandleForObject(static_cast<uint32_t>(a_actor->GetFormType()), a_actor);
	if (handle != policy.EmptyHandle()) {
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
		static_cast<RE::BSScript::IVirtualMachine*>(vm)->DispatchMethodCall(handle, "Actor", "ChangeHeadPart", callback, a_part);
	}
}

static void CallPapyrusUpdateAppearance(RE::Actor* a_actor)
{
	auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	if (!vm || !a_actor) return;

	auto& policy = vm->GetObjectHandlePolicy();

	uint64_t handle = policy.GetHandleForObject(static_cast<uint32_t>(a_actor->GetFormType()), a_actor);
	if (handle != policy.EmptyHandle()) {
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
		static_cast<RE::BSScript::IVirtualMachine*>(vm)->DispatchMethodCall(handle, "Actor", "UpdateAppearance", callback);
	}
}

static void CallPapyrusQueueRestoring(RE::Actor* a_actor)
{
	auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	if (!vm || !a_actor) return;

	auto& policy = vm->GetObjectHandlePolicy();

	uint64_t handle = policy.GetHandleForObject(static_cast<uint32_t>(a_actor->GetFormType()), a_actor);
	if (handle != policy.EmptyHandle()) {
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
		static_cast<RE::BSScript::IVirtualMachine*>(vm)->DispatchMethodCall(handle, "Actor", "QueueRestoring", callback);
	}
}
// ------------------------------------------------------------

void PersistenceManager::ApplyAppearance(RE::Actor* a_actor) {
	if (!a_actor) return;
	RE::TESNPC* npc = a_actor->GetNPC();
	if (!npc) return;

	std::lock_guard<std::mutex> lock(_lock);
	uint32_t lookupID = npc->IsUnique() ? npc->formID : a_actor->GetFormID();

	if (!_npcData.contains(lookupID)) return;

	auto& cache = _runtimeCache[lookupID];
	if (cache.isApplying) return;

	// Safety: Don't apply during loading screens
	auto ui = RE::UI::GetSingleton();
	if (ui && ui->GetMenuOpen("LoadingMenu")) {
		return;
	}

	auto& savedData = _npcData[lookupID];
	if (cache.lastHash == savedData.appearanceHash && savedData.appearanceHash != 0) {
		return;
	}

	cache.isApplying = true;
	spdlog::info("Queueing appearance application for NPC: {}", savedData.name);

	// TASK QUEUE (Execute on Main Thread to prevent CTD)
	F4SE::GetTaskInterface()->AddTask([a_actor, npc, savedData, lookupID]() {
		try {
			// Safety: actor and its 3D must still be alive and valid
			if (!a_actor || !npc || !a_actor->Get3D()) {
				PersistenceManager::GetSingleton()->ResetApplyingFlag(lookupID);
				return;
			}

			spdlog::info("Applying appearance to: {} ({:08X})", savedData.name, lookupID);

			// 1. FaceGen (Morphs) – direct write, no Papyrus needed
			if (npc->morphSliderValues && savedData.geometry.morphs.size() == npc->morphSliderValues->size()) {
				uint32_t index = 0;
				for (auto& [key, val] : *npc->morphSliderValues) {
					val = savedData.geometry.morphs[index++];
				}
			}

			// 2. HeadParts – Papyrus VM üzerinden ChangeHeadPart (Artık güvenli çünkü kPostLoadGame sonrası çalışır)
			auto* pm = PersistenceManager::GetSingleton();
			if (!savedData.assets.hairID.empty()) {
				uint32_t hairFormID = pm->StringToFormID(savedData.assets.hairID);
				if (hairFormID) {
					auto newHair = RE::TESForm::GetFormByID<RE::BGSHeadPart>(hairFormID);
					if (newHair) CallPapyrusChangeHeadPart(a_actor, newHair);
				}
			}

			if (!savedData.assets.eyesID.empty()) {
				uint32_t eyesFormID = pm->StringToFormID(savedData.assets.eyesID);
				if (eyesFormID) {
					auto newEyes = RE::TESForm::GetFormByID<RE::BGSHeadPart>(eyesFormID);
					if (newEyes) CallPapyrusChangeHeadPart(a_actor, newEyes);
				}
			}

			// 3. Skin colour – direct NPC field write
			uint32_t parsedColor = pm->HexToColor(savedData.skinColor);
			npc->bodyTintColorR = (parsedColor >> 16) & 0xFF;
			npc->bodyTintColorG = (parsedColor >> 8) & 0xFF;
			npc->bodyTintColorB = parsedColor & 0xFF;

			// 4. Tints – rebuild the entry list safely
			if (npc->tintingData && !savedData.tints.empty()) {
				auto entriesList = reinterpret_cast<RE::BGSCharacterTint::Entries*>(npc->tintingData);

				// BSSimpleList has no clear(); drain by pop_front
				while (!entriesList->entries.empty()) {
					delete entriesList->entries.front();
					entriesList->entries.pop_front();
				}

				for (const auto& td : savedData.tints) {
					if (td.type == 0) { // Palette
						auto* newEntry = new RE::BGSCharacterTint::PaletteEntry();
						if (newEntry) {
							newEntry->index = td.index;
							newEntry->color = td.color;
							newEntry->alpha = td.alpha;
							entriesList->entries.push_front(newEntry);
						}
					} else { // Texture
						auto* newEntry = new RE::BGSCharacterTint::TextureEntry();
						if (newEntry) {
							newEntry->index = td.index;
							newEntry->alpha = td.alpha;
							entriesList->entries.push_front(newEntry);
						}
					}
				}
				spdlog::info("Restored {} tints for {}", savedData.tints.size(), savedData.name);
			}

			// 5. Update appearance via Papyrus & Engine
			// Using multiple update methods to ensure the engine registers the change
			CallPapyrusQueueRestoring(a_actor);
			CallPapyrusUpdateAppearance(a_actor);
			
			// Direct engine update if possible
			a_actor->Update3DPosition(true);

			spdlog::info("Appearance applied on main thread for {}", savedData.name);
		} catch (const std::exception& e) {
			spdlog::error("Error in TaskQueue applying appearance: {}", e.what());
		} catch (...) {
			spdlog::error("Unknown error in TaskQueue applying appearance.");
		}

		// Reset applying flag regardless of success/failure
		PersistenceManager::GetSingleton()->ResetApplyingFlag(lookupID);
	});

	cache.lastHash = savedData.appearanceHash;
	// Note: isApplying is reset inside the task after execution
}

void PersistenceManager::LoadFromJson() {
	try {
		std::ifstream file(UAPS_JSON_FILE);
		if (!file.is_open()) return;
		nlohmann::json j;
		file >> j;

		if (j.contains("data")) {
			_npcData.clear();
			_runtimeCache.clear();
			
			for (auto& [keyStr, val] : j["data"].items()) {
				uint32_t formID = StringToFormID(keyStr);
				if (formID != 0) {
					NPC_Appearance app = val.get<NPC_Appearance>();
					_npcData[formID] = app;
				}
			}
			spdlog::info("Loaded UAPS data from JSON. Entries: {}", _npcData.size());
		}
	} catch (const std::exception& e) {
		spdlog::error("Error loading JSON: {}", e.what());
	}
}

void PersistenceManager::SaveToJson() {
	std::thread([this]() {
		try {
			std::lock_guard<std::mutex> lock(_lock);
			nlohmann::json j;
			j["uaps_version"] = "1.0";
			
			nlohmann::json dataObj = nlohmann::json::object();
			for (const auto& [formID, app] : _npcData) {
				std::string keyStr = FormIDToString(formID);
				if (!keyStr.empty()) {
					dataObj[keyStr] = app;
				}
			}
			j["data"] = dataObj;

			std::ofstream file(UAPS_JSON_FILE);
			if (file.is_open()) {
				file << j.dump(2);
				spdlog::info("Saved UAPS data to JSON.");
			}
		} catch (const std::exception& e) {
			spdlog::error("Error saving JSON: {}", e.what());
		}
	}).detach();
}

void PersistenceManager::SaveCallback(const F4SE::SerializationInterface* a_intfc) { 
	std::lock_guard<std::mutex> lock(_lock);
	try {
		nlohmann::json j;
		nlohmann::json dataObj = nlohmann::json::object();
		for (const auto& [formID, app] : _npcData) {
			dataObj[std::to_string(formID)] = app;
		}
		j["data"] = dataObj;

		std::string dumped = j.dump();
		if (!a_intfc->OpenRecord('DATA', 1)) {
			spdlog::error("Failed to open F4SE record for saving.");
			return;
		}
		a_intfc->WriteRecordData(dumped.data(), static_cast<uint32_t>(dumped.size()));
		spdlog::info("Serialized {} NPC entries to co-save.", _npcData.size());
	} catch (const std::exception& e) {
		spdlog::error("Error in SaveCallback: {}", e.what());
	}
}

void PersistenceManager::LoadCallback(const F4SE::SerializationInterface* a_intfc) {
	std::lock_guard<std::mutex> lock(_lock);
	_npcData.clear();
	_runtimeCache.clear();

	uint32_t type, version, length;
	while (a_intfc->GetNextRecordInfo(type, version, length)) {
		if (type == 'DATA') {
			std::string data;
			data.resize(length);
			a_intfc->ReadRecordData(data.data(), length);

			try {
				auto j = nlohmann::json::parse(data);
				if (j.contains("data")) {
					for (auto& [keyStr, val] : j["data"].items()) {
						uint32_t formID = std::stoul(keyStr);
						auto resolvedID = a_intfc->ResolveFormID(formID);
						if (resolvedID) {
							_npcData[*resolvedID] = val.get<NPC_Appearance>();
						}
					}
				}
				spdlog::info("Loaded {} NPC entries from co-save.", _npcData.size());
			} catch (const std::exception& e) {
				spdlog::error("Error parsing serialized data: {}", e.what());
			}
		}
	}
}

void PersistenceManager::RevertCallback(const F4SE::SerializationInterface*) {
	std::lock_guard<std::mutex> lock(_lock);
	_npcData.clear();
	_runtimeCache.clear();
	// Disable hook until kPostLoadGame fires so we never fire during loading screen
	_gameLoaded.store(false);
	spdlog::info("Serialization: Reverted UAPS data.");
}

namespace HookPoints {
	typedef RE::NiAVObject* (*_Actor_Load3D)(RE::Actor* a_this, bool a_backgroundLoading);
	REL::Relocation<_Actor_Load3D> Actor_Load3D_Original;

	RE::NiAVObject* Actor_Load3D_Hook(RE::Actor* a_this, bool a_backgroundLoading) {
		RE::NiAVObject* result = nullptr;
		if (Actor_Load3D_Original.address()) {
			result = Actor_Load3D_Original(a_this, a_backgroundLoading);
		}

		// Guard: only apply after kPostLoadGame / kNewGame has fired.
		if (a_this && a_this->Get3D() && PersistenceManager::GetSingleton()->IsGameLoaded()) {
			// Basic sanity check to ensure we are looking at an Actor
			if (a_this->GetFormType() == RE::ENUM_FORM_ID::kACHR) {
				PersistenceManager::GetSingleton()->ApplyAppearance(a_this);
			}
		}
		return result;
	}
	
	void Install() {
		auto version = REL::Module::get().version();

		// Fallout 4 VTable Index for Actor::Load3D:
		// 1.10.163 (Old Gen): 0x86 (134)
		// 1.10.980 - 1.10.984 (Next Gen 1): 0x89 (137)
		// 1.11.184 - 1.11.193+ (Next Gen 2): 0x8A (138) - Some reports suggest 0x8A or even 0x8C.
		
		uint32_t load3DIndex = 0x86;
		if (version >= REL::Version{ 1, 11, 184, 0 }) {
			load3DIndex = 0x8A;
		} else if (version >= REL::Version{ 1, 10, 980, 0 }) {
			load3DIndex = 0x89;
		}

		spdlog::info("Installing Actor::Load3D Hook... Runtime: {}.{}.{}.{} -> Using Index: 0x{:X}",
			version.major(), version.minor(), version.patch(), version.build(), load3DIndex);

		REL::Relocation<uintptr_t> actorVtbl{ RE::VTABLE::Actor[0] };

		if (actorVtbl.address()) {
			spdlog::info("Actor VTable found at: {:016X}", actorVtbl.address());
			try {
				Actor_Load3D_Original = actorVtbl.write_vfunc(load3DIndex, Actor_Load3D_Hook);
				if (Actor_Load3D_Original.address()) {
					spdlog::info("Actor::Load3D Hooked. Original at: {:016X}", Actor_Load3D_Original.address());
				} else {
					spdlog::error("write_vfunc returned null for Actor::Load3D!");
				}
			} catch (const std::exception& e) {
				spdlog::critical("Exception while hooking Actor::Load3D: {}", e.what());
			}
		} else {
			spdlog::critical("Failed to find Actor VTable address! Relocation for VTABLE::Actor[0] returned 0.");
		}
	}
}

void PersistenceManager::InitializeHooks() {
	spdlog::info("PersistenceManager::InitializeHooks() called.");
	HookPoints::Install();
	LoadFromJson();
	spdlog::info("PersistenceManager::InitializeHooks() complete.");
	spdlog::default_logger()->flush();
}

void PersistenceManager::OnPostLoadGame() {
	spdlog::info("kPostLoadGame received – enabling appearance hook.");
	_gameLoaded.store(true);

	// Re-apply for any actors that are already loaded
	if (auto processLists = RE::ProcessLists::GetSingleton()) {
		for (auto& actorHandle : processLists->highActorHandles) {
			if (auto actor = actorHandle.get()) {
				ApplyAppearance(actor.get());
			}
		}
	}
}

void PersistenceManager::OnNewGame() {
	spdlog::info("kNewGame received – enabling appearance hook.");
	_gameLoaded.store(true);
}

void PersistenceManager::RegisterMenuWatcher() {}
