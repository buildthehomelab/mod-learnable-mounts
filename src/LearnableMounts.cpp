/*
 * mod-learnable-mounts: turns mount items that still summon the mount straight from the bag into
 * WotLK-style "Teaches you how to summon this mount" items, so the mount lands in the Mounts tab.
 *
 * In 3.3.5 every normal mount item uses spell 55884 (Learning) with the mount spell in slot 2
 * (trigger 6, "learn spell"); the core's Player::CastItemUseSpell teaches Spells[1] and the item is
 * used up. Items that were removed before 3.0 or never released, such as the Fluorescent Green
 * Mechanostrider (13325) and the old vanilla racial mounts, were never converted and still cast
 * the mount spell directly.
 *
 * The change is made to the loaded item template rather than in SQL. mod-individual-progression
 * rewrites many item_template rows back to vanilla in its own world SQL, and the DB updater re-runs
 * that file whenever IP changes it, which would silently undo an SQL update from this module.
 *
 * Released under GNU GPL v2; redistribute/modify under version 2 of the License, or (at your
 * option) any later version.
 */

#include "Config.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "SpellAuraDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "StringConvert.h"
#include "Tokenize.h"

namespace
{
    constexpr uint32 SPELL_LEARNING_WOTLK = 55884;

    // Values every stock 3.3.5 mount item uses on its Learning slot.
    constexpr uint32 LEARN_SPELL_CATEGORY          = 330;
    constexpr int32  LEARN_SPELL_CATEGORY_COOLDOWN = 3000;

    // Every item in stock AC whose only spell is an on-use mount spell from the Mounts skill line.
    // All are pre-3.0 or unreleased items that Blizzard never converted.
    constexpr char const* DEFAULT_ITEMS = "13325,1041,1133,1134,5663,2413,2415,5874,5875,8583,8589,8590,14062,16339,25596,29225";

    bool IsMountSpell(uint32 spellId)
    {
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
        return info && info->HasAura(SPELL_AURA_MOUNTED) && info->IsAbilityOfSkillType(SKILL_MOUNTS);
    }

    // Returns false (and says why) if the item was left alone.
    bool MakeLearnable(uint32 itemId)
    {
        std::vector<ItemTemplate*> const* store = sObjectMgr->GetItemTemplateStoreFast();
        ItemTemplate* proto = itemId < store->size() ? (*store)[itemId] : nullptr;
        if (!proto)
        {
            LOG_ERROR("module", "mod-learnable-mounts: item {} is missing from item_template, skipped", itemId);
            return false;
        }

        if (proto->Spells[0].SpellId == int32(SPELL_LEARNING_WOTLK))
        {
            LOG_INFO("module", "mod-learnable-mounts: item {} ({}) already teaches its mount, skipped", itemId, proto->Name1);
            return false;
        }

        // Only the simple case: the mount spell is the item's one and only on-use spell.
        // Anything else is a custom item that needs a human to look at it.
        uint32 mountSpell = 0;
        for (_Spell const& spell : proto->Spells)
        {
            if (!spell.SpellId)
                continue;

            if (mountSpell || spell.SpellTrigger != ITEM_SPELLTRIGGER_ON_USE || !IsMountSpell(spell.SpellId))
            {
                LOG_ERROR("module", "mod-learnable-mounts: item {} ({}) is not a plain on-use mount item "
                    "(expected a single on-use spell from the Mounts skill line), skipped", itemId, proto->Name1);
                return false;
            }

            mountSpell = spell.SpellId;
        }

        if (!mountSpell)
        {
            LOG_ERROR("module", "mod-learnable-mounts: item {} ({}) has no spells, skipped", itemId, proto->Name1);
            return false;
        }

        for (_Spell& spell : proto->Spells)
            spell = _Spell();

        // Charges -1 = the item is used up once the mount is learned, including copies players
        // already carry (their stored charge count is 0, which the core treats as spent).
        _Spell& learn = proto->Spells[0];
        learn.SpellId               = SPELL_LEARNING_WOTLK;
        learn.SpellTrigger          = ITEM_SPELLTRIGGER_ON_USE;
        learn.SpellCharges          = -1;
        learn.SpellCooldown         = -1;
        learn.SpellCategory         = LEARN_SPELL_CATEGORY;
        learn.SpellCategoryCooldown = LEARN_SPELL_CATEGORY_COOLDOWN;

        _Spell& taught = proto->Spells[1];
        taught.SpellId               = mountSpell;
        taught.SpellTrigger          = ITEM_SPELLTRIGGER_LEARN_SPELL_ID;
        taught.SpellCooldown         = -1;
        taught.SpellCategoryCooldown = -1;

        if (proto->Description.empty())
            proto->Description = "Teaches you how to summon this mount.";

        LOG_INFO("module", "mod-learnable-mounts: item {} ({}) now teaches mount spell {}", itemId, proto->Name1, mountSpell);
        return true;
    }

    class LearnableMountsWorldScript : public WorldScript
    {
    public:
        LearnableMountsWorldScript()
            : WorldScript("LearnableMountsWorldScript", { WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED }) { }

        // Runs once, after every item template and spell is loaded and before players can log in.
        void OnBeforeWorldInitialized() override
        {
            if (!sConfigMgr->GetOption<bool>("LearnableMounts.Enable", true))
                return;

            std::string items = sConfigMgr->GetOption<std::string>("LearnableMounts.Items", DEFAULT_ITEMS);
            for (std::string_view token : Acore::Tokenize(items, ',', false))
            {
                // Allow "13325, 12345".
                while (!token.empty() && token.front() == ' ')
                    token.remove_prefix(1);
                while (!token.empty() && token.back() == ' ')
                    token.remove_suffix(1);

                Optional<uint32> itemId = Acore::StringTo<uint32>(token);
                if (!itemId || !*itemId)
                {
                    LOG_ERROR("module", "mod-learnable-mounts: '{}' in LearnableMounts.Items is not an item id, skipped", token);
                    continue;
                }

                MakeLearnable(*itemId);
            }
        }
    };
}

void AddLearnableMountsScripts()
{
    new LearnableMountsWorldScript();
}
