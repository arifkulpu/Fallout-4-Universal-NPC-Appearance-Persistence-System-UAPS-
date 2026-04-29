#pragma once

#include <nlohmann/json.hpp>
#include <set>

class PersistenceManager
{
public:
	static PersistenceManager* GetSingleton()
	{
		static PersistenceManager singleton;
		return &singleton;
	}

	void LoadWatchlist();
	void SaveToWatchlist(RE::Actor* a_actor);
	void RemoveFromWatchlist(std::uint32_t a_formID);
	bool IsInWatchlist(std::uint32_t a_formID);
	
	void ApplyAppearance(RE::Actor* a_actor);

	void InitializeHooks();
	void RegisterMenuWatcher();

private:

	PersistenceManager() = default;
	std::set<std::uint32_t> _watchlist;
	fs::path _storagePath = "Data/F4SE/Plugins/UniversalAppearance/NPCs";
};
