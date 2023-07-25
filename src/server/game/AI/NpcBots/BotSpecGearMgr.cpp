#include "BotSpecGearMgr.h"
#include <map>
#include <DBCStores.h>


void BotSpecGearMgr::addRawItemStats(ItemTemplate const* item, ItemStats istats, uint32 botLevel = 1)
{
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

    if (istats[BOT_STAT_MOD_DAMAGE_MIN] && BOT_STAT_MOD_DAMAGE_MAX)
    {
        //WEPDPS                        //Get Average Dmg                                                           //Divide by AttackTime in Seconds
        istats[BOT_SPEC_STAT_MOD_DPS] += ((istats[BOT_STAT_MOD_DAMAGE_MIN] + istats[BOT_STAT_MOD_DAMAGE_MAX]) / 2.f) / (item->Delay / 1000.f);
        istats[BOT_SPEC_STAT_MOD_WEP_SPEED] += item->Delay / 1000.f;
    }

    if (false)//_botclass == BOT_CLASS_DRUID)
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
    for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; i++) {

        uint32 spellId = item->Spells[i].SpellId;
        if (spellId > 0) {
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
                if (spell->EffectApplyAuraName[j] == 85) { //85 -> mod mp5
                    istats[BOT_STAT_MOD_MANA_REGENERATION] += spell->EffectBasePoints[j] + 1;
                }
            }
        }

    }
}

//(green) Items with random stats do not have the stats directly on the item, but indirectly through enchantments
//For green items with random enchants see https ://wowwiki-archive.fandom.com/wiki/ItemRandomSuffix
void BotSpecGearMgr::addItemSuffixStats(uint32 suffixId, uint32 suffixFactor, ItemStats istats)
{
    if (!suffixId)
        return;

    ItemRandomSuffixEntry const* suffix = sItemRandomSuffixStore.LookupEntry(suffixId);
    ItemRandomPropertiesEntry const* randomProp = sItemRandomPropertiesStore.LookupEntry(suffixId);    

    if (!suffix && !randomProp)
        return;

    for (int k = 0; k < MAX_ITEM_ENCHANTMENT_EFFECTS; ++k)
    {
        uint32 enchantId = suffix ? suffix->Enchantment[k] : randomProp->Enchantment[k];

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
                //Deal with wrath props + school damage
                else if (enchant_display_type == ITEM_ENCHANTMENT_TYPE_EQUIP_SPELL) {
                    SpellEntry const* spell = sSpellStore.LookupEntry(enchant_spell_id);
                    for (uint8 j = 0; j < MAX_SPELL_EFFECTS; j++) {
                        if (spell->EffectApplyAuraName[j] == 13) { // 13 -> Spell applies Aura that modifies spell power
                            //School = 2^index
                            SPELL_SCHOOL_SHADOW;
                            uint8 school = spell->EffectMiscValue[j];
                            uint8 modPoints = spell->EffectBasePoints[j] + 1;

                            switch (school) {
                                case SPELL_SCHOOL_MASK_ARCANE:
                                    istats[BOT_SPEC_STAT_MOD_ARCANE_SP] += modPoints;
                                    break;
                                case SPELL_SCHOOL_MASK_FIRE:
                                    istats[BOT_SPEC_STAT_MOD_FIRE_SP] += modPoints;
                                    break;
                                case SPELL_SCHOOL_MASK_FROST:
                                    istats[BOT_SPEC_STAT_MOD_FROST_SP] += modPoints;
                                    break;
                                case SPELL_SCHOOL_MASK_NATURE:
                                    istats[BOT_SPEC_STAT_MOD_NATURE_SP] += modPoints;
                                    break;
                                case SPELL_SCHOOL_MASK_SHADOW:
                                    istats[BOT_SPEC_STAT_MOD_SHADOW_SP] += modPoints;
                                    break;
                            }                            
                        } 
                    }
                }
                //TODO check non stat effects
            }
        }
    }
}

void BotSpecGearMgr::addSocketStats(ItemTemplate const* item, ItemStats istats) {
    for (int i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i) {
        switch (item->Socket[i].Color) {
            case SOCKET_COLOR_BLUE:
                istats[BOT_SPEC_STAT_MOD_BSOCKET] += 1;
                break;
            case SOCKET_COLOR_YELLOW:
                istats[BOT_SPEC_STAT_MOD_YSOCKET] += 1;
                break;
            case SOCKET_COLOR_RED:
                istats[BOT_SPEC_STAT_MOD_RSOCKET] += 1;
                break;
            case SOCKET_COLOR_META:
                istats[BOT_SPEC_STAT_MOD_BSOCKET] += 1;
                break;
        }
    }
}

BotSpecGearMgr* BotSpecGearMgr::instance()
{
    static BotSpecGearMgr instance;
    return &instance;
}


float BotSpecGearMgr::getItemSpecScore(Item const* item, uint32 spec, uint32 botLevel) {
    if (!item)
        return 0.0f;
    return getItemSpecScore(item->GetTemplate(), item->GetItemRandomPropertyId(), item->GetItemSuffixFactor(), spec, botLevel);
}

float BotSpecGearMgr::getItemSpecScore(ItemTemplate const* item, uint32 suffixid, uint32 suffixFactor, uint32 spec, uint32 botLevel)
{
    if (!item)
        return 0.0f;        
    
    //BuildStatList
    ItemStats stats{};
    addRawItemStats(item, stats, botLevel);
    addItemSuffixStats(suffixid, suffixFactor, stats);
    addSpellStatEffects(item, stats);
    addSocketStats(item, stats);

    //calc ItemScore with Weights
    float itemScore = 0.0f;
    StatWeights weights = getStatWeights(spec); //todo actual spec
    for (auto const& [stat, weight] : weights) {
        itemScore += stats[stat] * weight;
    }
    return itemScore;
}

std::map<uint32, float> BotSpecGearMgr::getStatWeights(uint8 spec)
{
    if (auto res = _statWeights.find(spec); res != _statWeights.end()) {
        return res->second;
    }
    return std::map<uint32, float>();
}

BotSpecGearMgr::BotSpecGearMgr()
{
    //init statweight tables
    _statWeights.insert({ BOT_SPEC_WARRIOR_FURY, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 45.0f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 45.0f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 45.0f},
            {BOT_SPEC_STAT_MOD_DPS, 27.5f},
            {BOT_STAT_MOD_STRENGTH, 3.25f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 3.125f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 2.5f},
            {BOT_STAT_MOD_HIT_RATING, 2.5f},
            {BOT_STAT_MOD_CRIT_RATING, 1.875f},
            {BOT_STAT_MOD_HASTE_RATING, 1.875f},
            {BOT_STAT_MOD_AGILITY, 1.25f},
            {BOT_STAT_MOD_ATTACK_POWER, 1.25f},
            {BOT_STAT_MOD_ARMOR, 0.025f}}
        });
    _statWeights.insert({ BOT_SPEC_WARRIOR_PROTECTION, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 48.0f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 48.0f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 48.0f},
            {BOT_SPEC_STAT_MOD_DPS, 20.0f},
            {BOT_STAT_MOD_STAMINA, 2.0f},
            {BOT_STAT_MOD_DEFENSE_SKILL_RATING, 1.6f},
            {BOT_STAT_MOD_DODGE_RATING, 1.4f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 1.34f},
            {BOT_STAT_MOD_AGILITY, 1.2f},
            {BOT_STAT_MOD_BLOCK_VALUE, 1.18f},
            {BOT_STAT_MOD_PARRY_RATING, 1.16f},
            {BOT_STAT_MOD_BLOCK_RATING, 0.7f},
            {BOT_STAT_MOD_STRENGTH, 0.66f},
            {BOT_STAT_MOD_CRIT_RATING, 0.4f},
            {BOT_STAT_MOD_HASTE_RATING, 0.4f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 0.4f},
            {BOT_STAT_MOD_HIT_RATING, 0.4f},
            {BOT_STAT_MOD_ATTACK_POWER, 0.12f},
            {BOT_STAT_MOD_ARMOR, 0.1f}}
        });
    _statWeights.insert({ BOT_SPEC_WARRIOR_ARMS, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 17.561f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 17.561f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 17.561f},
            {BOT_SPEC_STAT_MOD_DPS, 6.049f},
            {BOT_STAT_MOD_HIT_RATING, 1.561f},
            {BOT_STAT_MOD_STRENGTH, 1.073f},
            {BOT_STAT_MOD_CRIT_RATING, 0.927f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 0.829f},
            {BOT_STAT_MOD_AGILITY, 0.78f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 0.634f},
            {BOT_STAT_MOD_ATTACK_POWER, 0.488f},
            {BOT_STAT_MOD_HASTE_RATING, 0.341f},
            {BOT_STAT_MOD_ARMOR, 0.01f}}
        });
    _statWeights.insert({ BOT_SPEC_PALADIN_HOLY, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 45.104f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 45.104f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 45.104f},
            {BOT_STAT_MOD_INTELLECT, 3.264f},
            {BOT_STAT_MOD_CRIT_RATING, 2.671f},
            {BOT_STAT_MOD_MANA_REGENERATION, 2.374f},
            {BOT_STAT_MOD_SPELL_POWER, 2.374f},
            {BOT_STAT_MOD_HASTE_RATING, 1.78f}}
        });
    _statWeights.insert({ BOT_SPEC_PALADIN_RETRIBUTION, {
            {BOT_SPEC_STAT_MOD_WEP_SPEED, 2.096f},
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 47.904f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 47.904f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 47.904f},
            {BOT_SPEC_STAT_MOD_DPS, 5.988f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 4.491f},
            {BOT_STAT_MOD_STRENGTH, 2.994f},
            {BOT_STAT_MOD_HIT_RATING, 2.695f},
            {BOT_STAT_MOD_CRIT_RATING, 2.156f},
            {BOT_STAT_MOD_AGILITY, 2.096f},
            {BOT_STAT_MOD_HASTE_RATING, 2.036f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 1.886f},
            {BOT_STAT_MOD_ATTACK_POWER, 1.228f},
            {BOT_STAT_MOD_SPELL_POWER, 1.198f}}
        });
    _statWeights.insert({ BOT_SPEC_PALADIN_PROTECTION, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 60.0f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 60.0f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 60.0f},
            {BOT_SPEC_STAT_MOD_DPS, 12.5f},
            {BOT_STAT_MOD_STAMINA, 2.5f},
            {BOT_STAT_MOD_DODGE_RATING, 2.35f},
            {BOT_STAT_MOD_DEFENSE_SKILL_RATING, 2.15f},
            {BOT_STAT_MOD_BLOCK_VALUE, 2.15f},
            {BOT_STAT_MOD_PARRY_RATING, 2.15f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 1.975f},
            {BOT_STAT_MOD_AGILITY, 1.9f},
            {BOT_STAT_MOD_HIT_RATING, 1.45f},
            {BOT_STAT_MOD_BLOCK_RATING, 1.3f},
            {BOT_STAT_MOD_ARMOR, 0.15f},
            {BOT_STAT_MOD_ATTACK_POWER, 0.15f},
            {BOT_STAT_MOD_SPELL_POWER, 0.15f},
            {BOT_STAT_MOD_CRIT_RATING, 0.075f}}
        });
    _statWeights.insert({ BOT_SPEC_HUNTER_MARKSMANSHIP, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 40.0f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 40.0f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 40.0f},
            {BOT_SPEC_STAT_MOD_DPS, 12.5f},
            {BOT_STAT_MOD_HIT_RATING, 3.25f},
            {BOT_STAT_MOD_HASTE_RATING, 2.75f},
            {BOT_STAT_MOD_AGILITY, 2.5f},
            {BOT_STAT_MOD_CRIT_RATING, 1.75f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 1.75f}}
        });
    _statWeights.insert({ BOT_SPEC_HUNTER_SURVIVAL, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 40.0f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 40.0f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 40.0f},
            {BOT_SPEC_STAT_MOD_DPS, 12.5f},
            {BOT_STAT_MOD_HIT_RATING, 3.25f},
            {BOT_STAT_MOD_HASTE_RATING, 2.75f},
            {BOT_STAT_MOD_AGILITY, 2.5f},
            {BOT_STAT_MOD_CRIT_RATING, 1.75f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 1.75f},
            {BOT_STAT_MOD_ATTACK_POWER, 1.0f}}
        });
    _statWeights.insert({ BOT_SPEC_HUNTER_BEASTMASTERY, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 40.0f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 40.0f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 40.0f},
            {BOT_SPEC_STAT_MOD_DPS, 12.5f},
            {BOT_STAT_MOD_HIT_RATING, 3.25f},
            {BOT_STAT_MOD_HASTE_RATING, 2.75f},
            {BOT_STAT_MOD_AGILITY, 2.5f},
            {BOT_STAT_MOD_CRIT_RATING, 1.75f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 1.75f},
            {BOT_STAT_MOD_ATTACK_POWER, 1.0f}}
        });
    _statWeights.insert({ BOT_SPEC_ROGUE_ASSASINATION, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 36.8f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 36.8f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 36.8f},
            {BOT_SPEC_STAT_MOD_DPS, 8.0f},
            {BOT_STAT_MOD_AGILITY, 2.3f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 2.0f},
            {BOT_STAT_MOD_HIT_RATING, 2.0f},
            {BOT_STAT_MOD_HASTE_RATING, 1.8f},
            {BOT_STAT_MOD_CRIT_RATING, 1.5f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 1.5f},
            {BOT_STAT_MOD_STRENGTH, 1.1f},
            {BOT_STAT_MOD_ATTACK_POWER, 1.0f}}
        });
    _statWeights.insert({ BOT_SPEC_ROGUE_SUBTLETY, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 36.8f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 36.8f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 36.8f},
            {BOT_SPEC_STAT_MOD_DPS, 8.0f},
            {BOT_STAT_MOD_AGILITY, 2.3f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 2.0f},
            {BOT_STAT_MOD_HIT_RATING, 2.0f},
            {BOT_STAT_MOD_HASTE_RATING, 1.8f},
            {BOT_STAT_MOD_CRIT_RATING, 1.5f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 1.5f},
            {BOT_STAT_MOD_STRENGTH, 1.1f},
            {BOT_STAT_MOD_ATTACK_POWER, 1.0f}}
        });
    _statWeights.insert({ BOT_SPEC_ROGUE_COMBAT, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 36.8f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 36.8f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 36.8f},
            {BOT_SPEC_STAT_MOD_DPS, 8.0f},
            {BOT_STAT_MOD_AGILITY, 2.3f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 2.0f},
            {BOT_STAT_MOD_HIT_RATING, 2.0f},
            {BOT_STAT_MOD_HASTE_RATING, 1.8f},
            {BOT_STAT_MOD_CRIT_RATING, 1.5f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 1.5f},
            {BOT_STAT_MOD_STRENGTH, 1.1f},
            {BOT_STAT_MOD_ATTACK_POWER, 1.0f}}
        });
    _statWeights.insert({ BOT_SPEC_PRIEST_SHADOW, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_SHADOW_SP, 3.333f},
            {BOT_STAT_MOD_CRIT_RATING, 3.333f},
            {BOT_STAT_MOD_HASTE_RATING, 3.333f},
            {BOT_STAT_MOD_HIT_RATING, 3.333f},
            {BOT_STAT_MOD_SPELL_POWER, 3.333f},
            {BOT_STAT_MOD_SPIRIT, 0.333f},
            {BOT_STAT_MOD_INTELLECT, 0.167f}}
        });
    _statWeights.insert({ BOT_SPEC_PRIEST_DISCIPLINE, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 50.667f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 50.667f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 50.667f},
            {BOT_STAT_MOD_HASTE_RATING, 4.0f},
            {BOT_STAT_MOD_MANA_REGENERATION, 3.333f},
            {BOT_STAT_MOD_SPELL_POWER, 2.667f},
            {BOT_STAT_MOD_INTELLECT, 2.333f},
            {BOT_STAT_MOD_SPIRIT, 2.333f},
            {BOT_STAT_MOD_CRIT_RATING, 1.333f}}
        });
    _statWeights.insert({ BOT_SPEC_PRIEST_HOLY, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 50.667f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 50.667f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 50.667f},
            {BOT_STAT_MOD_HASTE_RATING, 4.0f},
            {BOT_STAT_MOD_MANA_REGENERATION, 3.333f},
            {BOT_STAT_MOD_SPELL_POWER, 2.667f},
            {BOT_STAT_MOD_INTELLECT, 2.333f},
            {BOT_STAT_MOD_SPIRIT, 2.333f},
            {BOT_STAT_MOD_CRIT_RATING, 1.333f}}
        });
    _statWeights.insert({ BOT_SPEC_DK_BLOOD, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 48.0f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 48.0f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 48.0f},
            {BOT_SPEC_STAT_MOD_DPS, 20.0f},
            {BOT_STAT_MOD_STAMINA, 2.0f},
            {BOT_STAT_MOD_DEFENSE_SKILL_RATING, 1.6f},
            {BOT_STAT_MOD_DODGE_RATING, 1.4f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 1.34f},
            {BOT_STAT_MOD_HIT_RATING, 1.34f},
            {BOT_STAT_MOD_AGILITY, 1.2f},
            {BOT_STAT_MOD_PARRY_RATING, 1.16f},
            {BOT_STAT_MOD_STRENGTH, 0.66f},
            {BOT_STAT_MOD_CRIT_RATING, 0.4f},
            {BOT_STAT_MOD_HASTE_RATING, 0.4f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 0.4f},
            {BOT_STAT_MOD_ATTACK_POWER, 0.12f},
            {BOT_STAT_MOD_ARMOR, 0.1f}}
        });
    _statWeights.insert({ BOT_SPEC_DK_FROST, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_DPS, 62.5f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 41.6f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 41.6f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 41.6f},
            {BOT_STAT_MOD_STRENGTH, 2.6f},
            {BOT_STAT_MOD_ATTACK_POWER, 1.25f},
            {BOT_STAT_MOD_CRIT_RATING, 1.25f},
            {BOT_STAT_MOD_HASTE_RATING, 1.25f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 1.25f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 1.25f},
            {BOT_STAT_MOD_HIT_RATING, 1.25f},
            {BOT_STAT_MOD_AGILITY, 0.625f}}
        });
    _statWeights.insert({ BOT_SPEC_DK_UNHOLY, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_DPS, 62.5f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 41.6f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 41.6f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 41.6f},
            {BOT_STAT_MOD_STRENGTH, 2.6f},
            {BOT_STAT_MOD_ATTACK_POWER, 1.25f},
            {BOT_STAT_MOD_CRIT_RATING, 1.25f},
            {BOT_STAT_MOD_HASTE_RATING, 1.25f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 1.25f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 1.25f},
            {BOT_STAT_MOD_HIT_RATING, 1.25f},
            {BOT_STAT_MOD_AGILITY, 0.625f}}
        });
    _statWeights.insert({ BOT_SPEC_SHAMAN_ENHANCEMENT, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 53.333f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 53.333f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 53.333f},
            {BOT_SPEC_STAT_MOD_DPS, 16.667f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 4.51f},
            {BOT_STAT_MOD_HASTE_RATING, 3.078f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 3.0f},
            {BOT_STAT_MOD_AGILITY, 2.232f},
            {BOT_STAT_MOD_STRENGTH, 2.0f},
            {BOT_STAT_MOD_ATTACK_POWER, 1.667f},
            {BOT_STAT_MOD_HIT_RATING, 1.342f},
            {BOT_STAT_MOD_SPELL_POWER, 0.758f},
            {BOT_STAT_MOD_CRIT_RATING, 0.318f}}
        });
    _statWeights.insert({ BOT_SPEC_SHAMAN_ELEMENTAL, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 63.333f},
            {BOT_STAT_MOD_CRIT_RATING, 3.333f},
            {BOT_STAT_MOD_HASTE_RATING, 3.333f},
            {BOT_STAT_MOD_HIT_RATING, 3.333f},
            {BOT_STAT_MOD_SPELL_POWER, 3.333f},
            {BOT_STAT_MOD_INTELLECT, 0.567f}}
        });
    _statWeights.insert({ BOT_SPEC_SHAMAN_RESTORATION, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 73.077f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 73.077f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 73.077f},
            {BOT_STAT_MOD_MANA_REGENERATION, 4.615f},
            {BOT_STAT_MOD_HASTE_RATING, 4.615f},
            {BOT_STAT_MOD_INTELLECT, 3.846f},
            {BOT_STAT_MOD_CRIT_RATING, 3.846f},
            {BOT_STAT_MOD_SPELL_POWER, 3.846f}}
        });
    _statWeights.insert({ BOT_SPEC_MAGE_ARCANE, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_ARCANE_SP, 3.333f},
            {BOT_STAT_MOD_SPELL_POWER, 3.333f},
            {BOT_STAT_MOD_HASTE_RATING, 3.03f},
            {BOT_STAT_MOD_HIT_RATING, 2.727f},
            {BOT_STAT_MOD_CRIT_RATING, 2.423f},
            {BOT_STAT_MOD_INTELLECT, 2.12f},
            {BOT_STAT_MOD_MANA_REGENERATION, 1.517f},
            {BOT_STAT_MOD_SPIRIT, 0.607f}}
        });
    _statWeights.insert({ BOT_SPEC_MAGE_FIRE, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 63.333f},
            {BOT_STAT_MOD_HIT_RATING, 4.167f},
            {BOT_SPEC_STAT_MOD_FIRE_SP, 3.333f},
            {BOT_STAT_MOD_SPELL_POWER, 3.333f},
            {BOT_STAT_MOD_HASTE_RATING, 3.057f},
            {BOT_STAT_MOD_CRIT_RATING, 2.5f},
            {BOT_STAT_MOD_INTELLECT, 1.39f},
            {BOT_STAT_MOD_MANA_REGENERATION, 1.39f},
            {BOT_STAT_MOD_SPIRIT, 0.557f}}
        });
    _statWeights.insert({ BOT_SPEC_MAGE_FROST, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 63.333f},
            {BOT_STAT_MOD_HIT_RATING, 5.15f},
            {BOT_STAT_MOD_HASTE_RATING, 4.39f},
            {BOT_SPEC_STAT_MOD_FROST_SP, 3.333f},
            {BOT_STAT_MOD_SPELL_POWER, 3.333f},
            {BOT_STAT_MOD_CRIT_RATING, 2.137f},
            {BOT_STAT_MOD_MANA_REGENERATION, 1.463f},
            {BOT_STAT_MOD_INTELLECT, 0.617f},
            {BOT_STAT_MOD_SPIRIT, 0.557f}}
        });
    _statWeights.insert({ BOT_SPEC_WARLOCK_DEMONOLOGY, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 63.333f},
            {BOT_STAT_MOD_HIT_RATING, 5.333f},
            {BOT_STAT_MOD_HASTE_RATING, 4.0f},
            {BOT_SPEC_STAT_MOD_SHADOW_SP, 3.333f},
            {BOT_STAT_MOD_SPELL_POWER, 3.333f},
            {BOT_STAT_MOD_CRIT_RATING, 2.667f},
            {BOT_STAT_MOD_INTELLECT, 1.333f}}
        });
    _statWeights.insert({ BOT_SPEC_WARLOCK_DESTRUCTION, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 63.333f},
            {BOT_STAT_MOD_HIT_RATING, 5.333f},
            {BOT_STAT_MOD_HASTE_RATING, 4.0f},
            {BOT_SPEC_STAT_MOD_SHADOW_SP, 3.333f},
            {BOT_STAT_MOD_SPELL_POWER, 3.333f},
            {BOT_STAT_MOD_CRIT_RATING, 2.667f},
            {BOT_STAT_MOD_INTELLECT, 1.333f}}
        });
    _statWeights.insert({ BOT_SPEC_WARLOCK_AFFLICTION, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 63.333f},
            {BOT_STAT_MOD_HIT_RATING, 4.667f},
            {BOT_STAT_MOD_HASTE_RATING, 3.667f},
            {BOT_SPEC_STAT_MOD_SHADOW_SP, 3.333f},
            {BOT_STAT_MOD_SPELL_POWER, 3.333f},
            {BOT_STAT_MOD_CRIT_RATING, 2.0f},
            {BOT_STAT_MOD_INTELLECT, 0.667f}}
        });
    _statWeights.insert({ BOT_SPEC_DRUID_BALANCE, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 63.333f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 63.333f},
            {BOT_STAT_MOD_HIT_RATING, 4.0f},
            {BOT_SPEC_STAT_MOD_ARCANE_SP, 3.333f},
            {BOT_STAT_MOD_SPELL_POWER, 3.333f},
            {BOT_STAT_MOD_HASTE_RATING, 2.667f},
            {BOT_STAT_MOD_MANA_REGENERATION, 2.0f},
            {BOT_STAT_MOD_CRIT_RATING, 2.0f},
            {BOT_STAT_MOD_INTELLECT, 1.333f},
            {BOT_STAT_MOD_SPIRIT, 1.0f}}
        });
    _statWeights.insert({ BOT_SPEC_DRUID_RESTORATION, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 59.375f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 59.375f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 59.375f},
            {BOT_STAT_MOD_HASTE_RATING, 3.125f},
            {BOT_STAT_MOD_SPELL_POWER, 3.125f},
            {BOT_STAT_MOD_MANA_REGENERATION, 1.563f},
            {BOT_STAT_MOD_SPIRIT, 1.238f},
            {BOT_STAT_MOD_INTELLECT, 0.891f},
            {BOT_STAT_MOD_CRIT_RATING, 0.313f}}
        });
    _statWeights.insert({ BOT_SPEC_DRUID_FERAL_CAT, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 44.131f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 44.131f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 44.131f},
            {BOT_STAT_MOD_AGILITY, 2.758f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 2.59f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 2.407f},
            {BOT_STAT_MOD_HIT_RATING, 2.407f},
            {BOT_STAT_MOD_STRENGTH, 2.266f},
            {BOT_STAT_MOD_CRIT_RATING, 1.623f},
            {BOT_STAT_MOD_ATTACK_POWER, 1.0f},
            {BOT_STAT_MOD_FERAL_ATTACK_POWER, 1.0f},
            {BOT_STAT_MOD_HASTE_RATING, 0.927f}}
        });
    _statWeights.insert({ BOT_SPEC_DRUID_FERAL_BEAR, {
            {BOT_SPEC_STAT_MOD_MSOCKET, 100.0f},
            {BOT_SPEC_STAT_MOD_RSOCKET, 37.468f},
            {BOT_SPEC_STAT_MOD_YSOCKET, 37.468f},
            {BOT_SPEC_STAT_MOD_BSOCKET, 37.468f},
            {BOT_STAT_MOD_EXPERTISE_RATING, 2.28f},
            {BOT_STAT_MOD_STAMINA, 1.561f},
            {BOT_STAT_MOD_AGILITY, 1.482f},
            {BOT_STAT_MOD_STRENGTH, 1.133f},
            {BOT_STAT_MOD_HIT_RATING, 1.123f},
            {BOT_STAT_MOD_CRIT_RATING, 1.009f},
            {BOT_STAT_MOD_ARMOR_PENETRATION_RATING, 0.879f},
            {BOT_STAT_MOD_HASTE_RATING, 0.831f},
            {BOT_STAT_MOD_ATTACK_POWER, 0.5f},
            {BOT_STAT_MOD_FERAL_ATTACK_POWER, 0.5f},
            {BOT_STAT_MOD_ARMOR, 0.286f},
            {BOT_STAT_MOD_DODGE_RATING, 0.243f},
            {BOT_STAT_MOD_DEFENSE_SKILL_RATING, 0.232f},
            {BOT_STAT_MOD_RESILIENCE_RATING, 0.116f}}
        });
}
