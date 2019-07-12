// This is not build through script

#pragma once
#include "../Vitruvia.h"

SOURCE_INIT

enum ItemQuality
{
	ITEM_QUALITY_DEFAULT,
	ITEM_QUALITY_GENUINE,
	ITEM_QUALITY_VINTAGE,
	ITEM_QUALITY_UNUSUAL,
	ITEM_QUALITY_UNK,
	ITEM_QUALITY_COMMUNITY,
	ITEM_QUALITY_DEVELOPER,
	ITEM_QUALITY_SELFMADE,
	ITEM_QUALITY_CUSTOMIZED,
	ITEM_QUALITY_STRANGE,
	ITEM_QUALITY_COMPLETED,
	ITEM_QUALITY_UNK2,
	ITEM_QUALITY_TOURNAMENT
};

enum ItemRarity
{
	ITEM_RARITY_DEFAULT,
	ITEM_RARITY_COMMON,
	ITEM_RARITY_UNCOMMON,
	ITEM_RARITY_RARE,
	ITEM_RARITY_MYTHICAL,
	ITEM_RARITY_LEGENDARY,
	ITEM_RARITY_ANCIENT,
	ITEM_RARITY_IMMORTAL
};

class C_EconItem
{
	unsigned short* GetEconItemData();
	void UpdateEquippedState(unsigned int state);
public:
	uint32_t * GetInventory();
	uint32_t * GetAccountID();
	uint16_t* GetDefIndex();
	uint64_t* GetItemID();
	uint64_t* GetOriginalID();
	unsigned char* GetFlags();
	void SetQuality(ItemQuality quality);
	void SetRarity(ItemRarity rarity);
	void SetOrigin(int origin);
	void SetLevel(int level);
	void SetInUse(bool in_use);
	void SetCustomName(const char* name);
	void SetCustomDesc(const char* name);
	void SetPaintSeed(float seed);
	void SetPaintKit(float kit);
	void SetPaintWear(float wear);
	void SetStatTrak(int val);
	void AddSticker(int index, int kit, float wear, float scale, float rotation);
	void EquipTT();
	void EquipCT();
	void EquipAll();

	static C_EconItem* CreateEconItem()
	{
		static auto fnCreateSharedObjectSubclass_EconItem_
			= reinterpret_cast<Source::C_EconItem*(__stdcall*)()>(
				*reinterpret_cast<uintptr_t*>(FindPattern(CLIENT_DLL,
					enc("C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? E8 ? ? ? ? 83 F8 FF 75 09 8D 45 E4 50 E8 ? ? ? ? 8D 45 E4 C7 45 ? ? ? ? ? 50 C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? E8 ? ? ? ? 83 F8 FF 75 09 8D 45 E4 50 E8 ? ? ? ? 8D 45 E4 C7 45 ? ? ? ? ? 50 C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? E8 ? ? ? ? 83 F8 FF 75 09 8D 45 E4 50 E8 ? ? ? ? 8D 45 E4 C7 45 ? ? ? ? ? 50 C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? E8 ? ? ? ? 83 F8 FF 75 09 8D 45 E4 50 E8 ? ? ? ? 8D 45 E4")) + 3)
				);

		return fnCreateSharedObjectSubclass_EconItem_();
	}

	static std::uintptr_t GetItemSchema()
	{
		static auto fnGetItemSchema
			= reinterpret_cast<uintptr_t(__stdcall*)()>(
				FindPattern(CLIENT_DLL, enc("A1 ? ? ? ? 85 C0 75 53"))
				);
		return fnGetItemSchema();
	}

	template<typename TYPE>
	void SetAttributeValue(int index, TYPE val)
	{
		auto v15 = (DWORD*)GetItemSchema();
		auto v16 = *(DWORD *)(v15[72] + 4 * index);

		static auto fnSetDynamicAttributeValue
			= reinterpret_cast<int(__thiscall*)(C_EconItem*, DWORD, void*)>(
				FindPattern(CLIENT_DLL, enc("55 8B EC 83 E4 F8 83 EC 3C 53 8B 5D 08 56 57 6A 00"))
				);

		fnSetDynamicAttributeValue(this, v16, &val);
	}
};

SOURCE_END