#pragma once

#include "RE/Fallout.h"
#include "F4SE/F4SE.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <nlohmann/json.hpp>

struct TintData
{
	uint16_t index;
	float alpha;
	std::string color;
	uint32_t blendOp; // Keeping this just in case, though maybe not in JSON initially

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(TintData, index, alpha, color, blendOp)

	bool operator==(const TintData& a_rhs) const {
		return index == a_rhs.index && color == a_rhs.color && alpha == a_rhs.alpha && blendOp == a_rhs.blendOp;
	}
};

struct GeometryData
{
	std::vector<float> morphs;
	std::vector<float> weights;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(GeometryData, morphs, weights)
};

struct AssetsData
{
	std::string hairID;
	std::string eyesID;
	std::vector<std::string> otherHeadParts; // to support multiple headparts if needed

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AssetsData, hairID, eyesID, otherHeadParts)
};

struct NPC_Appearance
{
	std::string name;
	std::string type; // "Unique", "Settler", "Raider"
	GeometryData geometry;
	AssetsData assets;
	std::vector<TintData> tints;
	std::string skinColor; // e.g. "#FFFFFF"

	uint64_t appearanceHash;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(NPC_Appearance, name, type, geometry, assets, tints, skinColor, appearanceHash)
};

struct RuntimeState
{
	bool isApplying{false};
	uint64_t lastHash{0};
};

class PersistenceManager
{
public:
	static PersistenceManager* GetSingleton()
	{
		static PersistenceManager singleton;
		return &singleton;
	}

	void SaveToWatchlist(RE::Actor* a_actor);
	void RemoveFromWatchlist(std::uint32_t a_formID);
	bool IsInWatchlist(std::uint32_t a_formID);
	
	void ApplyAppearance(RE::Actor* a_actor);

	void InitializeHooks();
	void RegisterMenuWatcher();

	// JSON File operations
	void LoadFromJson();
	void SaveToJson();

	// Serialization (optional, keeping minimal for triggering LoadFromJson)
	void LoadCallback(const F4SE::SerializationInterface* a_intfc);
	void SaveCallback(const F4SE::SerializationInterface* a_intfc);
	void RevertCallback(const F4SE::SerializationInterface* a_intfc);

	uint64_t ComputeHash(const NPC_Appearance& data);

	// Helpers
	std::string FormIDToString(uint32_t a_formID);
	uint32_t StringToFormID(const std::string& a_str);
	std::string ColorToHex(uint32_t a_color);
	uint32_t HexToColor(const std::string& a_hex);

private:
	PersistenceManager() = default;

	// Key is standard formID, but for unique actors we might use base ID.
	// For now, storing by FormID in runtime memory.
	std::unordered_map<uint32_t, NPC_Appearance> _npcData;
	std::unordered_map<uint32_t, RuntimeState> _runtimeCache;
	std::mutex _lock;
};
