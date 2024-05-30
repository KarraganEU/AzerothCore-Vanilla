#ifndef BOT_SPECGEARMGR_H_
#define BOT_SPECGEARMGR_H_



#include <utility>
#include <ItemTemplate.h>
#include <botcommon.h>
enum BotSpecMods : uint8 {
    BOT_SPEC_STAT_MOD_DPS = MAX_BOT_ITEM_MOD,
    BOT_SPEC_STAT_MOD_MSOCKET,
    BOT_SPEC_STAT_MOD_BSOCKET,
    BOT_SPEC_STAT_MOD_YSOCKET,
    BOT_SPEC_STAT_MOD_RSOCKET,
    BOT_SPEC_STAT_MOD_ARCANE_SP,
    BOT_SPEC_STAT_MOD_FIRE_SP,
    BOT_SPEC_STAT_MOD_FROST_SP,
    BOT_SPEC_STAT_MOD_SHADOW_SP,
    BOT_SPEC_STAT_MOD_NATURE_SP,
    BOT_SPEC_STAT_MOD_WEP_SPEED,

    //identical idx as last mod
    BOT_SPEC_STAT_END = BOT_SPEC_STAT_MOD_WEP_SPEED,
};

enum BotMultiSpec : uint8 {
    BOT_SPEC_DRUID_FERAL_CAT = BOT_SPEC_END +1,
    BOT_SPEC_DRUID_FERAL_BEAR
};

class BotSpecGearMgr
{
private:
    float _baseThreshold;
public:
	float getItemSpecScore(ItemTemplate const* item, uint32 suffixId, uint32 suffixFactor, uint32 spec, uint32 level);
    float getItemSpecScore(Item const* item, uint32 spec, uint32 botLevel);
    float getThresholdLevel();
	std::unordered_map<uint32, float> getStatWeights(uint8 spec);
	static BotSpecGearMgr* instance();
    //typedef std::pair<ItemTemplate const*, std::pair<uint32, uint32>> parsedItemLink;
private:
	BotSpecGearMgr();
	//~BotSpecGearMgr();
    typedef std::unordered_map<uint32, float> StatWeights;
	std::unordered_map<uint32, StatWeights> _statWeights; //Spec->Stat->Weightvalue
    typedef int32 ItemStats[BOT_SPEC_STAT_END+1];
    void addRawItemStats(ItemTemplate const* item, ItemStats istats, uint32 botLevel);
    void addSpellStatEffects(const ItemTemplate* item, ItemStats istats);
    void addItemSuffixStats(uint32 suffixId, uint32 sufficFactor, ItemStats istats);
    void addSocketStats(ItemTemplate const* item, ItemStats istats);
};

#define sBotSpecGearMgr BotSpecGearMgr::instance()

#endif
