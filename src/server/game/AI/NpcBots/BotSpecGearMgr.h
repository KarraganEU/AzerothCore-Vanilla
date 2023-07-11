#ifndef BOT_SPECGEARMGR_H_
#define BOT_SPECGEARMGR_H_



#include <utility>
#include <ItemTemplate.h>
#include <botcommon.h>

class BotSpecGearMgr
{
	public:
		float getItemSpecScore(ItemTemplate const* item, uint32 suffixId, uint32 suffixFactor);
		std::map<uint32, float> getStatWeights(uint8 spec);
		static BotSpecGearMgr* instance();
        //typedef std::pair<ItemTemplate const*, std::pair<uint32, uint32>> parsedItemLink;
        struct parseResult {
            ItemTemplate const* proto=0;
            uint32 suffixId=0;
            uint32 suffixFactor=0;
        };
        parseResult parseItemLink(std::string message);
	private:
		BotSpecGearMgr();
		//~BotSpecGearMgr();
        typedef std::map<uint32, float> StatWeights;
		std::map<uint32, StatWeights> _statWeights; //Spec->Stat->Weightvalue
        typedef int32 ItemStats[MAX_BOT_ITEM_MOD];
        void addRawItemStats(ItemTemplate const* item, ItemStats istats, uint32 botLevel);
        void addSpellStatEffects(const ItemTemplate* item, ItemStats istats);
        void addItemSuffixStats(uint32 suffixId, uint32 sufficFactor, ItemStats istats);
};

#define sBotSpecGearMgr BotSpecGearMgr::instance()

#endif
