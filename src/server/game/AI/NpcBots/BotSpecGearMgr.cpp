#include "BotSpecGearMgr.h"
#include <map>
#include <DBCStores.h>
#include <boost/algorithm/string/find.hpp>

BotSpecGearMgr::BotSpecGearMgr()
{
	//init statweight tables
	//std::map<uint32, StatWeights> const stats = {
	//	{0, {0, 2}}
	//};
	_statWeights.insert({ BOT_SPEC_WARRIOR_FURY,
		{
			{ITEM_MOD_STRENGTH, 2.f},
			{2, 3.2f} 
		}
	});    
}

void BotSpecGearMgr::addRawItemStats(ItemTemplate const* item, ItemStats istats, uint32 botLevel=1)
{
    //TODO For green items with random enchants see https://wowwiki-archive.fandom.com/wiki/ItemRandomSuffix
    ScalingStatDistributionEntry const* ssd = item->ScalingStatDistribution ? sScalingStatDistributionStore.LookupEntry(item->ScalingStatDistribution) : NULL;

    uint32 ssd_level = botLevel;
    if (ssd && ssd_level > ssd->MaxLevel)
        ssd_level = ssd->MaxLevel;

    ScalingStatValuesEntry const* ssv = item->ScalingStatValue ? sScalingStatValuesStore.LookupEntry(ssd_level) : NULL;

    for (uint8 i = 0; i != MAX_ITEM_PROTO_STATS; ++i)
    {
        uint32 statType = 0;
        int32  val = 0;
        if (ssd && ssv)
        {
            if (ssd->StatMod[i] < 0)
                continue;
            statType = ssd->StatMod[i];
            val = (ssv->getssdMultiplier(item->ScalingStatValue) * ssd->Modifier[i]) / 10000;
        }
        else
        {
            if (i >= item->StatsCount)
                continue;
            statType = item->ItemStat[i].ItemStatType;
            val = item->ItemStat[i].ItemStatValue;
        }

        if (val == 0)
            continue;

        istats[statType] += val;
    }

    istats[BOT_STAT_MOD_RESIST_HOLY] += item->HolyRes;
    istats[BOT_STAT_MOD_RESIST_FIRE] += item->FireRes;
    istats[BOT_STAT_MOD_RESIST_NATURE] += item->NatureRes;
    istats[BOT_STAT_MOD_RESIST_FROST] += item->FrostRes;
    istats[BOT_STAT_MOD_RESIST_SHADOW] += item->ShadowRes;
    istats[BOT_STAT_MOD_RESIST_ARCANE] += item->ArcaneRes;

    istats[BOT_STAT_MOD_ARMOR] += item->Armor;
    istats[BOT_STAT_MOD_BLOCK_VALUE] += item->Block;

    if (ssv)
    {
        int32 extraDPS = ssv->getDPSMod(item->ScalingStatValue);
        if (extraDPS)
        {
            float average = extraDPS * item->Delay / 1000.0f;
            float mod = ssv->IsTwoHand(item->ScalingStatValue) ? 0.2f : 0.3f;

            istats[BOT_STAT_MOD_DAMAGE_MIN] += (1.0f - mod) * average;
            istats[BOT_STAT_MOD_DAMAGE_MAX] += (1.0f + mod) * average;
        }
    }
    else
    {
        istats[BOT_STAT_MOD_DAMAGE_MIN] += item->Damage[0].DamageMin + item->Damage[1].DamageMin;
        istats[BOT_STAT_MOD_DAMAGE_MAX] += item->Damage[0].DamageMax + item->Damage[1].DamageMax;
    }

    if (true)//_botclass == BOT_CLASS_DRUID)
    {
        int32 dpsMod = 0;
        int32 feral_bonus = 0;

        if (ssv)
        {
            dpsMod = ssv->getDPSMod(item->ScalingStatValue);
            feral_bonus += ssv->getFeralBonus(item->ScalingStatValue);
        }

        feral_bonus += item->getFeralBonus(dpsMod);
        if (feral_bonus)
            istats[BOT_STAT_MOD_FERAL_ATTACK_POWER] += feral_bonus;
    }

    addSpellStatEffects(item, istats);
    
    //for (uint8 i = 0; i != MAX_ENCHANTMENT_SLOT; ++i)
    //{
    //    
    //    //uint32 enchant_id = 99999;
    //    EnchantmentSlot eslot = EnchantmentSlot(i);
    //    uint32 enchant_id = item->GetEnchantmentId(eslot);
    //    if (!enchant_id)
    //        continue;
    //    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(4);
    //    if (!pEnchant)
    //        continue;

    //    uint32 enchant_display_type;
    //    uint32 enchant_amount;
    //    uint32 enchant_spell_id;

    //    for (uint8 s = 0; s != MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS; ++s)
    //    {
    //        enchant_display_type = pEnchant->type[s];
    //        enchant_amount = pEnchant->amount[s];
    //        enchant_spell_id = pEnchant->spellid[s];
    //        switch (enchant_display_type)
    //        {
    //            case ITEM_ENCHANTMENT_TYPE_DAMAGE:
    //                istats[BOT_STAT_MOD_DAMAGE_MIN] += enchant_amount;
    //                istats[BOT_STAT_MOD_DAMAGE_MAX] += enchant_amount;
    //                break;
    //            case ITEM_ENCHANTMENT_TYPE_RESISTANCE:
    //                if (!enchant_amount)
    //                {
    //                    //ItemRandomSuffixEntry const* item_rand = sItemRandomSuffixStore.LookupEntry(abs(item->GetItemRandomPropertyId()));
    //                    //if (item_rand)
    //                    //{
    //                    //    for (uint8 k = 0; k < MAX_ITEM_ENCHANTMENT_EFFECTS; ++k)
    //                    //    {
    //                    //        if (item_rand->Enchantment[k] == enchant_id)
    //                    //        {
    //                    //            enchant_amount = uint32((item_rand->AllocationPct[k] * item->GetItemSuffixFactor()) / 10000);
    //                    //            break;
    //                    //        }
    //                    //    }
    //                    //}
    //                }
    //                istats[BOT_STAT_MOD_RESISTANCE_START + enchant_spell_id] += enchant_amount;
    //                break;
    //            case ITEM_ENCHANTMENT_TYPE_STAT:
    //            {
    //                //if (!enchant_amount)
    //                //{
    //                //    ItemRandomSuffixEntry const* item_rand_suffix = sItemRandomSuffixStore.LookupEntry(abs(item->GetItemRandomPropertyId()));
    //                //    if (item_rand_suffix)
    //                //    {
    //                //        for (uint8 k = 0; k != MAX_ITEM_ENCHANTMENT_EFFECTS; ++k)
    //                //        {
    //                //            if (item_rand_suffix->Enchantment[k] == enchant_id)
    //                //            {
    //                //                enchant_amount = uint32((item_rand_suffix->AllocationPct[k] * item->GetItemSuffixFactor()) / 10000);
    //                //                break;
    //                //            }
    //                //        }
    //                //    }
    //                //}

    //                switch (enchant_spell_id)
    //                {
    //                case ITEM_MOD_MANA:
    //                case ITEM_MOD_HEALTH:
    //                case ITEM_MOD_AGILITY:
    //                case ITEM_MOD_STRENGTH:
    //                case ITEM_MOD_INTELLECT:
    //                case ITEM_MOD_SPIRIT:
    //                case ITEM_MOD_STAMINA:
    //                case ITEM_MOD_DEFENSE_SKILL_RATING:
    //                case ITEM_MOD_DODGE_RATING:
    //                case ITEM_MOD_PARRY_RATING:
    //                case ITEM_MOD_BLOCK_RATING:
    //                case ITEM_MOD_HIT_MELEE_RATING:
    //                case ITEM_MOD_HIT_RANGED_RATING:
    //                case ITEM_MOD_HIT_SPELL_RATING:
    //                case ITEM_MOD_CRIT_MELEE_RATING:
    //                case ITEM_MOD_CRIT_RANGED_RATING:
    //                case ITEM_MOD_CRIT_SPELL_RATING:
    //                case ITEM_MOD_HASTE_MELEE_RATING:
    //                case ITEM_MOD_HASTE_RANGED_RATING:
    //                case ITEM_MOD_HASTE_SPELL_RATING:
    //                case ITEM_MOD_HIT_RATING:
    //                case ITEM_MOD_CRIT_RATING:
    //                case ITEM_MOD_HASTE_RATING:
    //                case ITEM_MOD_RESILIENCE_RATING:
    //                case ITEM_MOD_EXPERTISE_RATING:
    //                case ITEM_MOD_ATTACK_POWER:
    //                case ITEM_MOD_RANGED_ATTACK_POWER:
    //                case ITEM_MOD_MANA_REGENERATION:
    //                case ITEM_MOD_ARMOR_PENETRATION_RATING:
    //                case ITEM_MOD_SPELL_POWER:
    //                case ITEM_MOD_HEALTH_REGEN:
    //                case ITEM_MOD_SPELL_PENETRATION:
    //                case ITEM_MOD_BLOCK_VALUE:
    //                    istats[enchant_spell_id] += enchant_amount;
    //                    break;
    //                default:
    //                    break;
    //                }
    //                break;
    //            }
    //        }
    //    }
    //}
}

// Handle stats, that are written as spells -> in 3.3.5 those are increase SpellPower/Attack Power/Ranged Attack Power
void BotSpecGearMgr::addSpellStatEffects(const ItemTemplate* item, ItemStats istats)
{
    for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; i++)    {
        
        uint32 spellId = item->Spells[i].SpellId;
        if (spellId>0) {
            SpellEntry const* spell = sSpellStore.LookupEntry(spellId);
            for (uint8 j = 0; j < MAX_SPELL_EFFECTS; j++) {
                //+1 because the basepoints in the spelldbc, which are read here, are for some reason 1 lower than the actual modifier
                if (spell->EffectApplyAuraName[j] == 13) { // 13 -> Spell applies Aura that modifies spell power
                    istats[BOT_STAT_MOD_SPELL_POWER] += spell->EffectBasePoints[j] + 1;
                }
                if (spell->EffectApplyAuraName[j] == 99) { // 99 -> mod attack power
                    istats[BOT_STAT_MOD_ATTACK_POWER] += spell->EffectBasePoints[j] + 1;
                }
                if (spell->EffectApplyAuraName[j] == 124) { //124 -> mod RAP
                    istats[BOT_STAT_MOD_RANGED_ATTACK_POWER] += spell->EffectBasePoints[j] + 1;
                }
            }
        }

    }
}

//(green) Items with random stats do not have the stats directly on the item, but indirectly through enchantments
void BotSpecGearMgr::addItemSuffixStats(uint32 suffixId, uint32 suffixFactor, ItemStats istats)
{
    if (!suffixId)
        return;

    ItemRandomSuffixEntry const* suffix = sItemRandomSuffixStore.LookupEntry(suffixId);
    for (int k = 0; k < MAX_ITEM_ENCHANTMENT_EFFECTS; ++k)
    {
        uint32 enchantId = suffix->Enchantment[k];
        SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
        if (pEnchant)
        {
            for (int s = 0; s < MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS; ++s)
            {
                uint32 enchant_display_type = pEnchant->type[s];
                uint32 enchant_amount = pEnchant->amount[s];
                uint32 enchant_spell_id = pEnchant->spellid[s];
                if (enchant_display_type == ITEM_ENCHANTMENT_TYPE_STAT) {
                    if (!enchant_amount) {
                        enchant_amount = int32((suffix->AllocationPct[k] * suffixFactor) / 10000);                        
                    }
                    istats[enchant_spell_id] += enchant_amount;
                }
            }
        }
    }
}

BotSpecGearMgr* BotSpecGearMgr::instance()
{
	static BotSpecGearMgr instance;
	return &instance;
}

BotSpecGearMgr::parseResult BotSpecGearMgr::parseItemLink(std::string message)
{
    //Check for Linked Item
    //Item links have this kind of format: "\124cff0070dd\124Hitem:13042::::::::80:::::\124h[Sword of the Magistrate]\124h\124r", where the first block signifies color, and the part after 'Hitem:' is the itemID    
    std::string token = "|Hitem:";
    std::string link = message;
    std::size_t pos = link.find(token);
    parseResult res;
    if (pos != std::string::npos) {
        link = link.substr(pos + token.length());
        pos = link.find(":");        
        res.proto = sObjectMgr->GetItemTemplate(std::stoi(link.substr(0, pos)));

        //find suffix data - like shadow wrath, monkey, gorilla, etc
        boost::iterator_range<std::string::iterator> suffixIdItr = boost::find_nth(link, ":", 5);
        boost::iterator_range<std::string::iterator> suffixFactorItr = boost::find_nth(link, ":", 6);
        boost::iterator_range<std::string::iterator> suffixEndItr = boost::find_nth(link, ":", 7);
        if (suffixFactorItr && suffixIdItr && suffixEndItr) {
            ptrdiff_t idIdx = std::distance(link.begin(), suffixIdItr.end());
            ptrdiff_t factorIdx = std::distance(link.begin(), suffixFactorItr.end());
            ptrdiff_t endIdx = std::distance(link.begin(), suffixEndItr.end());
            std::string suffixId = link.substr(idIdx, factorIdx - (idIdx+1));
            std::string suffixFactor = link.substr(factorIdx, endIdx - (factorIdx+1));
            res.suffixFactor = std::stoi(suffixFactor);
            res.suffixId = std::abs(std::stoi(suffixId));            
        }
  
    }
    return res;
}

float BotSpecGearMgr::getItemSpecScore(ItemTemplate const* item, uint32 suffixid=0, uint32 suffixFactor=0)
{
    ItemStats stats{};
    addItemSuffixStats(suffixid, suffixFactor, stats);
    addRawItemStats(item, stats, 4);
	return 0.0f;
}

std::map<uint32, float> BotSpecGearMgr::getStatWeights(uint8 spec)
{    
	if (auto res = _statWeights.find(spec); res != _statWeights.end()) {
		return res->second;
	}
	return std::map<uint32, float>();
}
