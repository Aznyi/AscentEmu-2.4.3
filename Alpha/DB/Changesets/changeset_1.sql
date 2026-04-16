-- Update Shrine of Dath Remars Object Type to match retail
UPDATE gameobject_names SET TYPE = 10 WHERE entry = 180516;

-- Add support for quest suggested player counts so the client can display
-- group-size hints instead of hardcoded zeroes.
ALTER TABLE `quests`
ADD COLUMN IF NOT EXISTS `SuggestedPlayers` int(10) unsigned NOT NULL DEFAULT 0
AFTER `Type`;

-- Mutually-exclusive quest group semantics
ALTER TABLE `quests`
ADD COLUMN IF NOT EXISTS `ExclusiveGroup` int(10) NOT NULL DEFAULT 0
AFTER `NextQuestId`;

-- Fix Invalid loot for non existent item in issue 7
DELETE cl
FROM creatureloot cl
LEFT JOIN items i ON i.entry = cl.itemid
WHERE cl.entryid IN (24560, 25169)
  AND i.entry IS NULL;

-- Fix Draenei starter quest chain visibility at Proenitus.
-- "Botanist Taerix" should not appear until "You Survived!" is turned in,
-- and "Urgent Delivery!" should stay hidden until the healing crystal quest
-- has been completed.
UPDATE quests
SET PrevQuestId = 9279,
    RequiredQuest1 = 9279
WHERE entry = 9371;

UPDATE quests
SET PrevQuestId = 9280,
    RequiredQuest1 = 9280
WHERE entry = 9409;

DELETE pl
FROM pickpocketingloot pl
LEFT JOIN items i ON i.entry = pl.itemid
WHERE pl.entryid IN (24960)
  AND i.entry IS NULL;


-- Fix invalid AI Agent spell loading in issue
DELETE FROM ai_agents
WHERE entry IN (10390,10391,10394,10482,11439,19428,21022)
  AND spell IN (15584,15613,11586,11571);

-- Fix Auberdine to Menethil Boat Spawn
INSERT INTO gameobject_spawns
(Entry, map, position_x, position_y, position_z, Facing,
 orientation1, orientation2, orientation3, orientation4,
 State, Flags, Faction, Scale, stateNpcLink)
VALUES
(176231, 1, 6425.14, 552.19, 8.65, 4.71239,
 0, 0, 0.707107, -0.707107,
 1, 0, 0, 1.0, 0);

-- Fix Auerdine to Rut Village
INSERT INTO gameobject_spawns
(Entry, map, position_x, position_y, position_z, Facing,
 orientation1, orientation2, orientation3, orientation4,
 State, Flags, Faction, Scale, stateNpcLink)
VALUES
(176244, 1, 6342.21, 557.16, 7.95, 1.5708,
 0, 0, 0.707107, 0.707107,
 1, 0, 0, 1.0, 0);

-- Remove Warsong Gulch objects that are already spawned and managed by the
-- battleground script in WarsongGulch.cpp.
--
-- The script-owned objects are:
-- - home flags
-- - speed / restoration / berserk buffs
-- - starting gates

SELECT `id`, `Entry`, `map`, `position_x`, `position_y`, `position_z`
FROM `gameobject_spawns`
WHERE `map` = 489
  AND `Entry` IN (179830, 179831, 179871, 179899, 179904, 179906, 179905, 179907, 179916, 179917, 179918, 179919, 179921)
ORDER BY `Entry`, `id`;

DELETE FROM `gameobject_spawns`
WHERE `map` = 489
  AND `Entry` IN (179830, 179831, 179871, 179899, 179904, 179906, 179905, 179907, 179916, 179917, 179918, 179919, 179921);

SELECT COUNT(*) AS `remaining_wsg_script_owned_spawns`
FROM `gameobject_spawns`
WHERE `map` = 489
  AND `Entry` IN (179830, 179831, 179871, 179899, 179904, 179906, 179905, 179907, 179916, 179917, 179918, 179919, 179921);

-- Cleanup for Alterac Valley graveyard guard world spawns.
--
-- AV now owns graveyard guards in code. The stock AV world data contains
-- overlapping guard ranks at the
-- same graveyard positions:
--   12050 / 12053 -> base guards
--   13326 / 13328 -> seasoned
--   13331 / 13332 -> veteran
--   13422 / 13421 -> champion
--
-- Leaving any of these DB rows in place causes pop-in/pop-out visuals when
-- cells load, because the battleground now spawns its own guard set.

SELECT `id`, `entry`, `map`, `position_x`, `position_y`, `position_z`
FROM `creature_spawns`
WHERE `map` = 30
  AND `entry` IN (12050, 12053, 13326, 13328, 13331, 13332, 13421, 13422)
ORDER BY `entry`, `id`;

DELETE FROM `creature_spawns`
WHERE `map` = 30
  AND `entry` IN (12050, 12053, 13326, 13328, 13331, 13332, 13421, 13422);

SELECT COUNT(*) AS `remaining_av_graveyard_guard_spawns`
FROM `creature_spawns`
WHERE `map` = 30
  AND `entry` IN (12050, 12053, 13326, 13328, 13331, 13332, 13421, 13422);

-- Remove Alterac Valley spirit guides that are already spawned by the
-- battleground script in AlteracValley.cpp via CBattleground::SpawnSpiritGuide.
--
-- Run this against the selected Ascent world database.
-- Keep other AV creatures; bosses, captains, guards, and linked tower/bunker
-- NPCs are still expected to come from DB spawns.

SELECT `id`, `entry`, `map`, `position_x`, `position_y`, `position_z`
FROM `creature_spawns`
WHERE `map` = 30
  AND `entry` IN (13116, 13117)
ORDER BY `entry`, `id`;

DELETE FROM `creature_spawns`
WHERE `map` = 30
  AND `entry` IN (13116, 13117);

SELECT COUNT(*) AS `remaining_av_script_owned_spirit_guides`
FROM `creature_spawns`
WHERE `map` = 30
  AND `entry` IN (13116, 13117);


-- Alterac Valley start gates should spawn closed and be opened by the battleground start event.
-- gameobject_spawns columns:
-- id, Entry, map, position_x, position_y, position_z, Facing,
-- orientation1, orientation2, orientation3, orientation4,
-- State, Flags, Faction, Scale, stateNpcLink

SELECT `id`, `Entry`, `map`, `position_x`, `position_y`, `position_z`, `State`, `Flags`, `Faction`, `Scale`
FROM `gameobject_spawns`
WHERE `id` IN (3000191, 3000192);

UPDATE `gameobject_spawns`
SET `State` = 1,
    `Flags` = 32
WHERE `id` IN (3000191, 3000192)
  AND `Entry` = 180424
  AND `map` = 30;

SELECT `id`, `Entry`, `map`, `position_x`, `position_y`, `position_z`, `State`, `Flags`, `Faction`, `Scale`
FROM `gameobject_spawns`
WHERE `id` IN (3000191, 3000192);

-- Stray Area Trigger NPC removal from AlteracValley that was present in CMangos TBC DB
DELETE FROM creature_spawns WHERE id = 3002000 AND entry = 22515 AND map = 30;

-- Stray spawn of Wing Commanders
DELETE FROM creature_spawns WHERE id = 3000632;
DELETE FROM creature_spawns WHERE id = 3001076;

-- Purge Mine spawns in AlteracValley allow the core to handle it
-- Review counts before delete
SELECT COUNT(*) AS irondeep_rows
FROM creature_spawns
WHERE map = 30
  AND entry IN (10987,11600,11602,11657,13078,13079,13080,13081,13098,13099,13396,13397)
  AND ((position_x - 880.0) * (position_x - 880.0)
     + (position_y - (-400.0)) * (position_y - (-400.0))
     + (position_z - 58.0) * (position_z - 58.0)) <= (150.0 * 150.0);

SELECT COUNT(*) AS coldtooth_rows
FROM creature_spawns
WHERE map = 30
  AND entry IN (11603,11604,11605,11677,13086,13088,13096,13097,13316,13317)
  AND ((position_x - (-862.0)) * (position_x - (-862.0))
     + (position_y - (-82.0)) * (position_y - (-82.0))
     + (position_z - 68.0) * (position_z - 68.0)) <= (150.0 * 150.0);

-- Delete only mine-state creatures inside each mine radius
DELETE FROM creature_spawns
WHERE map = 30
  AND entry IN (10987,11600,11602,11657,13078,13079,13080,13081,13098,13099,13396,13397)
  AND ((position_x - 880.0) * (position_x - 880.0)
     + (position_y - (-400.0)) * (position_y - (-400.0))
     + (position_z - 58.0) * (position_z - 58.0)) <= (150.0 * 150.0);

DELETE FROM creature_spawns
WHERE map = 30
  AND entry IN (11603,11604,11605,11677,13086,13088,13096,13097,13316,13317)
  AND ((position_x - (-862.0)) * (position_x - (-862.0))
     + (position_y - (-82.0)) * (position_y - (-82.0))
     + (position_z - 68.0) * (position_z - 68.0)) <= (150.0 * 150.0);

-- Remove Spirit Healers they are dynamically spawned in AV based on current control
DELETE FROM creature_spawns
WHERE map = 30
  AND entry IN (13116, 13117);

-- Correct requirements for Lokholar the Ice Lord
UPDATE `quests`
SET
    `ReqItemId1` = 17306,      -- Stormpike Soldier's Blood
    `ReqItemCount1` = 5,
    `ReqItemId2` = 17309,      -- Storm Crystal
    `ReqItemCount2` = 1,
    `ReqItemId3` = 0,
    `ReqItemCount3` = 0,
    `ReqItemId4` = 0,
    `ReqItemCount4` = 0
WHERE `entry` = 6801;


-- Stray Alterac Valley NPCs
DELETE FROM creature_spawns WHERE id = 3000031;
DELETE FROM creature_spawns WHERE id = 3000463;
DELETE FROM creature_spawns WHERE id = 3000464;
DELETE FROM creature_spawns WHERE id = 3000031;
DELETE FROM creature_spawns WHERE id = 3000644;
DELETE FROM creature_spawns WHERE id = 3000505;

-- CMangos TBC DB spawn cleanup
-- Arathi Basin node banners and faction civilians are now runtime-spawned by ArathiBasin.cpp.
-- Remove the old map 529 DB node spawns so they do not duplicate runtime state.

DELETE FROM creature_spawns
WHERE id BETWEEN 5290001 AND 5290110;

DELETE FROM gameobject_spawns
WHERE id BETWEEN 5290005 AND 5290029;

DELETE FROM gameobject_spawns
WHERE id BETWEEN 5290253 AND 5290254;

DELETE FROM gameobject_spawns
WHERE id BETWEEN 5290300 AND 5290314;

-- EoTs cleanup
DELETE FROM `gameobject_spawns` WHERE `id` BETWEEN 90056 AND 90102;
DELETE FROM `gameobject_spawns` WHERE `id` BETWEEN 93956 AND 93967;

DELETE FROM `creature_spawns` WHERE `id` BETWEEN 97126 AND 97133;
DELETE FROM `creature_spawns` WHERE `id` BETWEEN 98024 AND 98025;

-- Defias Miner fix for 6685 Piercing Shot awkwardness
DELETE FROM ai_agents WHERE entry = 598 AND spell = 6685;

-- Fix Deadmines Doors
UPDATE `gameobject_spawns` SET Faction = 114 WHERE Entry = 13965;
UPDATE `gameobject_spawns` SET Faction = 114 WHERE Entry = 16400;

-- Fix missing Deadmines spawns
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48775, 1732, 36, -21.874, -802.941, 19.7633, 1.72788, 0, 2350, 17, 64, 2048, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48785, 1732, 36, -9.91802, -740.42, 9.01033, 2.05949, 0, 2350, 17, 64, 2048, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48789, 1732, 36, -31.7616, -727.924, 8.49408, 5.81574, 0, 2350, 17, 64, 2048, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48799, 1732, 36, -55.1231, -828.416, 42.0565, 0.945801, 0, 2350, 17, 64, 67584, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48809, 1732, 36, -80.2899, -782.787, 17.3611, 3.48851, 0, 2350, 17, 64, 67584, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48818, 1732, 36, -85.1105, -856.553, 17.3856, 3.36848, 0, 2350, 17, 64, 2048, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48823, 1732, 36, -100.382, -779.345, 22.2591, 3.47228, 0, 2350, 17, 64, 2048, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48828, 1732, 36, -120.761, -835.888, 16.976, 3.56047, 0, 2350, 17, 64, 67584, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48831, 1732, 36, -111.524, -796.345, 16.9339, 3.94243, 0, 2350, 17, 64, 67584, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48835, 1732, 36, -142.136, -874.435, 1.87754, 2.52062, 0, 2350, 17, 64, 67584, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48839, 1732, 36, -79.8492, -729.316, 8.9472, 1.93382, 0, 2350, 17, 64, 67584, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48771, 657, 36, -103.389, -722.862, 8.53874, 4.52907, 0, 2347, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48776, 657, 36, -58.6838, -784.557, 17.9948, 3.83072, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48777, 657, 36, -21.3089, -735.867, 8.63692, 6.17616, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48779, 657, 36, -65.5106, -794.538, 39.3616, 4.48339, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48783, 657, 36, -77.7185, -796.636, 38.4104, 1.39716, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48784, 657, 36, -42.3903, -787.452, 18.5799, 2.19802, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48786, 657, 36, -13.7505, -724.854, 7.99955, 6.07117, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48787, 657, 36, -41.5577, -798.322, 39.3181, 0.976463, 0, 2347, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48788, 657, 36, -28.8178, -795.674, 19.3963, 6.02139, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48790, 657, 36, -75.4199, -783.754, 26.4547, 3.01942, 0, 2347, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48792, 657, 36, -51.9895, -789.721, 38.6283, 2.71524, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48798, 657, 36, -57.9513, -828.381, 41.7918, 1.74674, 0, 2347, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48800, 657, 36, -18.7577, -829.35, 19.7652, 1.13927, 0, 2347, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48803, 657, 36, -29.9255, -844.072, 19.1873, 5.30773, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48805, 657, 36, -20.4278, -836.713, 19.6958, 4.98484, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48806, 657, 36, -83.6326, -776.282, 26.793, 0.613624, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48810, 657, 36, -90.5243, -787.03, 26.8547, 4.45676, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48814, 657, 36, -89.2655, -854.165, 17.2242, 5.62322, 0, 2347, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48816, 657, 36, -102.106, -848.209, 17.0374, 5.68397, 0, 2347, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48817, 657, 36, -115.062, -838.69, 16.951, 5.63544, 0, 2347, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48824, 657, 36, -105.602, -793.749, 28.1933, 5.20108, 0, 2347, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48830, 657, 36, -128.671, -789.915, 17.2395, 0.138394, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48840, 657, 36, -61.0675, -732.599, 8.82062, 4.2344, 0, 2347, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48842, 657, 36, -50.0819, -722.807, 8.85201, 2.34329, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48843, 657, 36, -41.303, -730.5, 8.96366, 0.171102, 0, 2347, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48844, 657, 36, -89.9524, -719.418, 8.58565, 4.69833, 0, 2347, 17, 64, 16843008, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48845, 657, 36, -96.3392, -721.272, 8.45571, 2.29264, 0, 2347, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48847, 657, 36, -80.4466, -727.673, 8.92332, 5.83547, 0, 2347, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (3600078, 657, 36, -65.7304, -833.171, 41.0901, 1.22054, 0, 0, 17, 0, 0, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48797, 647, 36, -56.5084, -826.068, 41.9293, 1.55843, 0, 7113, 17, 64, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48773, 3450, 36, -103.315, -722.459, 8.55308, 2.19934, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48793, 3450, 36, -39.1564, -790.329, 18.7908, 4.86947, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48794, 3450, 36, -39.2416, -797.73, 39.4147, 5.91667, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48795, 3450, 36, -54.4516, -789.142, 38.9295, 2.09439, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48796, 3450, 36, -17.1351, -726.366, 7.97807, 2.95223, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48801, 3450, 36, -59.3829, -829.778, 41.6553, 1.32203, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48802, 3450, 36, -19.7972, -826.536, 19.7721, 1.13927, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48808, 3450, 36, -80.8657, -775.375, 26.8138, 5.62398, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48811, 3450, 36, -78.5346, -785.142, 26.3327, 2.5928, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48812, 3450, 36, -18.9556, -834.099, 19.8559, 4.98484, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48813, 3450, 36, -75.7603, -798.501, 38.451, 3.74235, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48821, 3450, 36, -102.911, -850.387, 17.0821, 4.04916, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48832, 3450, 36, -133.252, -791.493, 17.5385, 2.80998, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48833, 3450, 36, -30.3178, -793.076, 19.2237, 6.02139, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48834, 3450, 36, -102.364, -794.183, 28.1582, 5.24881, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48846, 3450, 36, -39.7752, -732.164, 8.98158, 1.46539, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48848, 3450, 36, -85.1472, -727.104, 8.99578, 5.86077, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
INSERT INTO `creature_spawns` (`id`, `entry`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `movetype`, `displayid`, `faction`, `flags`, `bytes`, `bytes2`, `emote_state`, `npc_respawn_link`, `channel_spell`, `channel_target_sqlid`, `channel_target_sqlid_creature`, `standstate`) VALUES (48849, 3450, 36, -46.2383, -724.391, 8.84136, 5.88009, 0, 5207, 17, 0, 16777472, 1, 0, 0, 0, 0, 0, 0);
DELETE FROM `creature_spawns` WHERE `id`=3600077;
DELETE FROM `creature_spawns` WHERE `id`=3600078;
DELETE FROM `creature_spawns` WHERE `id`=3600079;

-- Ascent's event gating is quest-level, not giver-level, so the giver `id`
-- column is intentionally discarded and duplicate (quest, event) pairs are collapsed.

CREATE TABLE IF NOT EXISTS `game_event_quest` (
  `quest` int(10) unsigned NOT NULL,
  `event` smallint(6) NOT NULL DEFAULT 0,
  PRIMARY KEY (`quest`,`event`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci COMMENT='Game event system';

DELETE FROM `game_event_quest`;

INSERT INTO `game_event_quest` (`quest`, `event`) VALUES
	(8353, 12),
	(8354, 12),
	(8355, 12),
	(8356, 12),
	(8357, 12),
	(8358, 12),
	(8359, 12),
	(8360, 12),
	(8980, 8),
	(8983, 8),
	(9025, 8),
	(9027, 8),
	(11356, 12),
	(11357, 12),
	(11441, 26),
	(11446, 26),
	(11496, 35),
	(11513, 39),
	(11514, 40),
	(11517, 39),
	(11520, 45),
	(11521, 46),
	(11523, 36),
	(11524, 35),
	(11525, 36),
	(11532, 37),
	(11533, 38),
	(11534, 40),
	(11535, 43),
	(11536, 44),
	(11537, 38),
	(11538, 37),
	(11539, 41),
	(11540, 42),
	(11541, 42),
	(11542, 41),
	(11543, 42),
	(11544, 44),
	(11545, 47),
	(11546, 46),
	(11547, 40),
	(11548, 48),
	(11549, 47),
	(11549, 48);


-- Backfill suggested player counts for quests
UPDATE `quests`
SET `SuggestedPlayers` = CASE `entry`
  WHEN 19 THEN 2
  WHEN 155 THEN 2
  WHEN 208 THEN 3
  WHEN 656 THEN 2
  WHEN 731 THEN 2
  WHEN 1173 THEN 2
  WHEN 3628 THEN 2
  WHEN 4021 THEN 3
  WHEN 5342 THEN 3
  WHEN 5344 THEN 3
  WHEN 6641 THEN 2
  WHEN 8315 THEN 2
  WHEN 8348 THEN 2
  WHEN 8363 THEN 2
  WHEN 8538 THEN 2
  WHEN 9156 THEN 3
  WHEN 9167 THEN 5
  WHEN 9215 THEN 2
  WHEN 9315 THEN 2
  WHEN 9375 THEN 2
  WHEN 9446 THEN 2
  WHEN 9466 THEN 2
  WHEN 9490 THEN 2
  WHEN 9528 THEN 2
  WHEN 9689 THEN 2
  WHEN 9711 THEN 2
  WHEN 9729 THEN 3
  WHEN 9730 THEN 2
  WHEN 9753 THEN 2
  WHEN 9756 THEN 2
  WHEN 9759 THEN 2
  WHEN 9760 THEN 2
  WHEN 9761 THEN 2
  WHEN 9817 THEN 2
  WHEN 9851 THEN 2
  WHEN 9852 THEN 2
  WHEN 9853 THEN 3
  WHEN 9856 THEN 2
  WHEN 9859 THEN 2
  WHEN 9868 THEN 2
  WHEN 9879 THEN 2
  WHEN 9894 THEN 2
  WHEN 9895 THEN 2
  WHEN 9937 THEN 5
  WHEN 9938 THEN 5
  WHEN 9946 THEN 3
  WHEN 9954 THEN 3
  WHEN 9955 THEN 3
  WHEN 9962 THEN 5
  WHEN 9967 THEN 5
  WHEN 9970 THEN 5
  WHEN 9972 THEN 5
  WHEN 9973 THEN 5
  WHEN 9977 THEN 5
  WHEN 9980 THEN 2
  WHEN 9981 THEN 2
  WHEN 9991 THEN 3
  WHEN 9999 THEN 3
  WHEN 10001 THEN 3
  WHEN 10004 THEN 3
  WHEN 10009 THEN 3
  WHEN 10010 THEN 3
  WHEN 10011 THEN 3
  WHEN 10020 THEN 2
  WHEN 10035 THEN 2
  WHEN 10036 THEN 2
  WHEN 10051 THEN 2
  WHEN 10052 THEN 2
  WHEN 10111 THEN 2
  WHEN 10116 THEN 2
  WHEN 10117 THEN 2
  WHEN 10132 THEN 2
  WHEN 10134 THEN 2
  WHEN 10136 THEN 3
  WHEN 10138 THEN 2
  WHEN 10139 THEN 3
  WHEN 10168 THEN 3
  WHEN 10191 THEN 2
  WHEN 10231 THEN 3
  WHEN 10247 THEN 3
  WHEN 10248 THEN 3
  WHEN 10252 THEN 3
  WHEN 10253 THEN 3
  WHEN 10256 THEN 2
  WHEN 10261 THEN 2
  WHEN 10274 THEN 3
  WHEN 10276 THEN 2
  WHEN 10290 THEN 2
  WHEN 10293 THEN 2
  WHEN 10309 THEN 3
  WHEN 10310 THEN 3
  WHEN 10320 THEN 3
  WHEN 10323 THEN 3
  WHEN 10337 THEN 2
  WHEN 10349 THEN 2
  WHEN 10351 THEN 2
  WHEN 10365 THEN 3
  WHEN 10400 THEN 3
  WHEN 10407 THEN 2
  WHEN 10408 THEN 5
  WHEN 10409 THEN 5
  WHEN 10439 THEN 5
  WHEN 10451 THEN 2
  WHEN 10507 THEN 5
  WHEN 10508 THEN 2
  WHEN 10518 THEN 3
  WHEN 10578 THEN 4
  WHEN 10588 THEN 5
  WHEN 10626 THEN 3
  WHEN 10627 THEN 3
  WHEN 10634 THEN 5
  WHEN 10636 THEN 2
  WHEN 10647 THEN 4
  WHEN 10648 THEN 4
  WHEN 10651 THEN 5
  WHEN 10692 THEN 5
  WHEN 10701 THEN 2
  WHEN 10707 THEN 5
  WHEN 10724 THEN 3
  WHEN 10742 THEN 3
  WHEN 10750 THEN 3
  WHEN 10751 THEN 3
  WHEN 10758 THEN 3
  WHEN 10764 THEN 3
  WHEN 10765 THEN 3
  WHEN 10768 THEN 3
  WHEN 10769 THEN 3
  WHEN 10772 THEN 3
  WHEN 10773 THEN 3
  WHEN 10774 THEN 3
  WHEN 10775 THEN 3
  WHEN 10776 THEN 3
  WHEN 10781 THEN 5
  WHEN 10793 THEN 4
  WHEN 10805 THEN 3
  WHEN 10806 THEN 3
  WHEN 10815 THEN 5
  WHEN 10821 THEN 2
  WHEN 10834 THEN 2
  WHEN 10838 THEN 2
  WHEN 10841 THEN 3
  WHEN 10842 THEN 3
  WHEN 10858 THEN 5
  WHEN 10866 THEN 5
  WHEN 10872 THEN 5
  WHEN 10876 THEN 3
  WHEN 10879 THEN 2
  WHEN 10898 THEN 2
  WHEN 10921 THEN 3
  WHEN 10922 THEN 2
  WHEN 10923 THEN 3
  WHEN 10925 THEN 3
  WHEN 10929 THEN 2
  WHEN 10930 THEN 3
  WHEN 10937 THEN 2
  WHEN 10974 THEN 5
  WHEN 10975 THEN 5
  WHEN 10976 THEN 5
  WHEN 10995 THEN 5
  WHEN 10996 THEN 5
  WHEN 10997 THEN 5
  WHEN 10998 THEN 5
  WHEN 11000 THEN 5
  WHEN 11041 THEN 2
  WHEN 11059 THEN 5
  WHEN 11072 THEN 3
  WHEN 11073 THEN 5
  WHEN 11078 THEN 5
  WHEN 11079 THEN 5
  WHEN 11097 THEN 3
  WHEN 11101 THEN 3
  WHEN 11401 THEN 5
  WHEN 11404 THEN 5
  WHEN 11405 THEN 5
  WHEN 11551 THEN 25
  WHEN 11552 THEN 25
  WHEN 11553 THEN 25
  WHEN 11691 THEN 5
  WHEN 11885 THEN 3
  WHEN 12062 THEN 5
  ELSE `SuggestedPlayers`
END
WHERE `SuggestedPlayers` = 0
  AND `entry` IN (
    19,155,208,656,731,1173,3628,4021,5342,5344,6641,8315,8348,8363,8538,
    9156,9167,9215,9315,9375,9446,9466,9490,9528,9689,9711,9729,9730,9753,
    9756,9759,9760,9761,9817,9851,9852,9853,9856,9859,9868,9879,9894,9895,
    9937,9938,9946,9954,9955,9962,9967,9970,9972,9973,9977,9980,9981,9991,
    9999,10001,10004,10009,10010,10011,10020,10035,10036,10051,10052,10111,
    10116,10117,10132,10134,10136,10138,10139,10168,10191,10231,10247,10248,
    10252,10253,10256,10261,10274,10276,10290,10293,10309,10310,10320,10323,
    10337,10349,10351,10365,10400,10407,10408,10409,10439,10451,10507,10508,
    10518,10578,10588,10626,10627,10634,10636,10647,10648,10651,10692,10701,
    10707,10724,10742,10750,10751,10758,10764,10765,10768,10769,10772,10773,
    10774,10775,10776,10781,10793,10805,10806,10815,10821,10834,10838,10841,
    10842,10858,10866,10872,10876,10879,10898,10921,10922,10923,10925,10929,
    10930,10937,10974,10975,10976,10995,10996,10997,10998,11000,11041,11059,
    11072,11073,11078,11079,11097,11101,11401,11404,11405,11551,11552,11553,
    11691,11885,12062
  );

-- IsRepeatable: 0 = normal, 1 = repeatable, 2 = daily.
UPDATE `quests`
SET `IsRepeatable` = CASE `entry`
  WHEN 2358 THEN 1 -- OLD Horns of Nez'ra
  WHEN 3785 THEN 1 -- Morrowgrain Research
  WHEN 5058 THEN 1 -- Mrs. Dalson's Diary
  WHEN 5405 THEN 1 -- Argent Dawn Commission
  WHEN 5503 THEN 1 -- Argent Dawn Commission
  WHEN 5517 THEN 1 -- Chromatic Mantle of the Dawn
  WHEN 5521 THEN 1 -- Chromatic Mantle of the Dawn
  WHEN 5524 THEN 1 -- Chromatic Mantle of the Dawn
  WHEN 5887 THEN 1 -- Salve via Hunting
  WHEN 5888 THEN 1 -- Salve via Mining
  WHEN 5889 THEN 1 -- Salve via Gathering
  WHEN 5890 THEN 1 -- Salve via Skinning
  WHEN 5891 THEN 1 -- Salve via Disenchanting
  WHEN 6962 THEN 1 -- Treats for Great-father Winter
  WHEN 7164 THEN 1 -- Honored Amongst the Clan
  WHEN 7166 THEN 1 -- Legendary Heroes
  WHEN 7167 THEN 1 -- The Eye of Command
  WHEN 7170 THEN 1 -- Earned Reverence
  WHEN 7171 THEN 1 -- Legendary Heroes
  WHEN 7172 THEN 1 -- The Eye of Command
  WHEN 7830 THEN 1 -- Avenging the Fallen
  WHEN 7846 THEN 1 -- Recover the Key!
  WHEN 8314 THEN 1 -- Unraveling the Mystery
  WHEN 8552 THEN 1 -- The Monogrammed Sash
  WHEN 8579 THEN 1 -- Mortal Champions
  WHEN 8763 THEN 1 -- The Hero of the Day
  WHEN 8799 THEN 1 -- The Hero of the Day
  ELSE `IsRepeatable`
END
WHERE `entry` IN (
  2358, 3785, 5058, 5405, 5503, 5517, 5521, 5524, 5887,
  5888, 5889, 5890, 5891, 6962, 7164, 7166, 7167, 7170,
  7171, 7172, 7830, 7846, 8314, 8552, 8579, 8763, 8799
)
AND `IsRepeatable` = 0;

-- Positive groups block another active/completed quest in the group.
-- Negative groups only block another active quest in the same group.
UPDATE `quests`
SET `ExclusiveGroup` = CASE `entry`
  WHEN 2 THEN -2
  WHEN 23 THEN -2
  WHEN 24 THEN -2
  WHEN 96 THEN 96
  WHEN 188 THEN -193
  WHEN 193 THEN -193
  WHEN 197 THEN -193
  WHEN 203 THEN -203
  WHEN 204 THEN -203
  WHEN 235 THEN 235
  WHEN 585 THEN -585
  WHEN 586 THEN -585
  WHEN 621 THEN -621
  WHEN 648 THEN -648
  WHEN 712 THEN -734
  WHEN 714 THEN -734
  WHEN 742 THEN 235
  WHEN 836 THEN -648
  WHEN 936 THEN 936
  WHEN 972 THEN 96
  WHEN 990 THEN 990
  WHEN 994 THEN 994
  WHEN 995 THEN 994
  WHEN 1000 THEN 1000
  WHEN 1004 THEN 1000
  WHEN 1015 THEN 1015
  WHEN 1018 THEN 1000
  WHEN 1019 THEN 1015
  WHEN 1047 THEN 1015
  WHEN 1079 THEN -1079
  WHEN 1080 THEN -1079
  WHEN 1083 THEN -1083
  WHEN 1084 THEN -1083
  WHEN 1118 THEN -621
  WHEN 1120 THEN 1120
  WHEN 1121 THEN 1120
  WHEN 1222 THEN -1222
  WHEN 1258 THEN -1222
  WHEN 1282 THEN 1282
  WHEN 1302 THEN 1282
  WHEN 1472 THEN 1472
  WHEN 1478 THEN 1478
  WHEN 1505 THEN 1505
  WHEN 1506 THEN 1506
  WHEN 1507 THEN 1472
  WHEN 1522 THEN 1522
  WHEN 1523 THEN 1522
  WHEN 1528 THEN 1528
  WHEN 1529 THEN 1528
  WHEN 1531 THEN 1531
  WHEN 1532 THEN 1531
  WHEN 1638 THEN 1638
  WHEN 1679 THEN 1638
  WHEN 1684 THEN 1638
  WHEN 1685 THEN 1715
  WHEN 1715 THEN 1715
  WHEN 1793 THEN 1793
  WHEN 1794 THEN 1793
  WHEN 1818 THEN 1505
  WHEN 1859 THEN 1859
  WHEN 1860 THEN 1860
  WHEN 1879 THEN 1860
  WHEN 1881 THEN 1881
  WHEN 1883 THEN 1881
  WHEN 1885 THEN 1859
  WHEN 2205 THEN 2205
  WHEN 2218 THEN 2205
  WHEN 2241 THEN 2205
  WHEN 2378 THEN 2378
  WHEN 2380 THEN 2378
  WHEN 2761 THEN -2761
  WHEN 2762 THEN -2761
  WHEN 2763 THEN -2761
  WHEN 2767 THEN -648
  WHEN 2771 THEN -2771
  WHEN 2772 THEN -2771
  WHEN 2773 THEN -2771
  WHEN 2848 THEN -2848
  WHEN 2849 THEN -2848
  WHEN 2850 THEN -2848
  WHEN 2851 THEN -2848
  WHEN 2852 THEN -2848
  WHEN 2855 THEN -2855
  WHEN 2856 THEN -2855
  WHEN 2857 THEN -2855
  WHEN 2858 THEN -2855
  WHEN 2859 THEN -2855
  WHEN 2983 THEN 1522
  WHEN 2984 THEN 1522
  WHEN 2985 THEN 1528
  WHEN 2996 THEN 2996
  WHEN 2997 THEN 2997
  WHEN 2998 THEN 2998
  WHEN 2999 THEN 2997
  WHEN 3000 THEN 2997
  WHEN 3001 THEN 2996
  WHEN 3127 THEN -3129
  WHEN 3128 THEN -3129
  WHEN 3526 THEN 3526
  WHEN 3629 THEN 3526
  WHEN 3630 THEN 3526
  WHEN 3631 THEN 4487
  WHEN 3632 THEN 3526
  WHEN 3633 THEN 3526
  WHEN 3634 THEN 3526
  WHEN 3635 THEN 3526
  WHEN 3637 THEN 3526
  WHEN 3638 THEN 3638
  WHEN 3640 THEN 3638
  WHEN 3642 THEN 3638
  WHEN 3681 THEN 2998
  WHEN 3762 THEN 936
  WHEN 3763 THEN 3763
  WHEN 3784 THEN 936
  WHEN 3787 THEN 3797
  WHEN 3788 THEN 3797
  WHEN 3789 THEN 3763
  WHEN 3790 THEN 3763
  WHEN 4022 THEN 4022
  WHEN 4023 THEN 4022
  WHEN 4181 THEN 3526
  WHEN 4285 THEN -4285
  WHEN 4287 THEN -4285
  WHEN 4288 THEN -4285
  WHEN 4293 THEN -4293
  WHEN 4294 THEN -4293
  WHEN 4485 THEN 4485
  WHEN 4486 THEN 4485
  WHEN 4487 THEN 4487
  WHEN 4488 THEN 4487
  WHEN 4489 THEN 4487
  WHEN 4737 THEN 4737
  WHEN 4738 THEN 4737
  WHEN 4962 THEN 4962
  WHEN 4963 THEN 4962
  WHEN 4964 THEN 4964
  WHEN 4965 THEN 4965
  WHEN 4967 THEN 4965
  WHEN 4968 THEN 4965
  WHEN 4969 THEN 4965
  WHEN 4975 THEN 4964
  WHEN 5066 THEN 5066
  WHEN 5090 THEN 5066
  WHEN 5091 THEN 5066
  WHEN 5093 THEN 5093
  WHEN 5094 THEN 5093
  WHEN 5095 THEN 5093
  WHEN 5141 THEN 5141
  WHEN 5142 THEN 5142
  WHEN 5143 THEN 5141
  WHEN 5144 THEN 5141
  WHEN 5145 THEN 5145
  WHEN 5146 THEN 5145
  WHEN 5148 THEN 5145
  WHEN 5154 THEN -5153
  WHEN 5168 THEN -5153
  WHEN 5249 THEN 5249
  WHEN 5250 THEN 5249
  WHEN 5283 THEN 5283
  WHEN 5284 THEN 5283
  WHEN 5301 THEN 5301
  WHEN 5302 THEN 5301
  WHEN 5542 THEN -5542
  WHEN 5543 THEN -5542
  WHEN 5544 THEN -5542
  WHEN 5601 THEN 5142
  WHEN 5628 THEN 5631
  WHEN 5629 THEN 5631
  WHEN 5631 THEN 5631
  WHEN 5634 THEN 5634
  WHEN 5635 THEN 5634
  WHEN 5636 THEN 5634
  WHEN 5637 THEN 5634
  WHEN 5638 THEN 5634
  WHEN 5639 THEN 5634
  WHEN 5641 THEN 5641
  WHEN 5642 THEN 5642
  WHEN 5643 THEN 5642
  WHEN 5644 THEN 5644
  WHEN 5645 THEN 5641
  WHEN 5646 THEN 5644
  WHEN 5647 THEN 5641
  WHEN 5652 THEN 5652
  WHEN 5654 THEN 5652
  WHEN 5655 THEN 5652
  WHEN 5656 THEN 5652
  WHEN 5657 THEN 5652
  WHEN 5658 THEN 5658
  WHEN 5660 THEN 5658
  WHEN 5661 THEN 5658
  WHEN 5662 THEN 5658
  WHEN 5663 THEN 5658
  WHEN 5672 THEN 5672
  WHEN 5673 THEN 5672
  WHEN 5674 THEN 5672
  WHEN 5675 THEN 5672
  WHEN 5676 THEN 5676
  WHEN 5677 THEN 5676
  WHEN 5679 THEN 5644
  WHEN 5680 THEN 5642
  WHEN 5923 THEN 5923
  WHEN 5924 THEN 5923
  WHEN 5925 THEN 5923
  WHEN 5926 THEN 5926
  WHEN 5927 THEN 5926
  WHEN 5928 THEN 5926
  WHEN 6065 THEN 6065
  WHEN 6066 THEN 6065
  WHEN 6067 THEN 6065
  WHEN 6068 THEN 6068
  WHEN 6069 THEN 6068
  WHEN 6070 THEN 6068
  WHEN 6071 THEN 6071
  WHEN 6072 THEN 6071
  WHEN 6073 THEN 6071
  WHEN 6074 THEN 6074
  WHEN 6075 THEN 6074
  WHEN 6076 THEN 6074
  WHEN 6382 THEN 235
  WHEN 6541 THEN 6541
  WHEN 6542 THEN 6541
  WHEN 6582 THEN -6582
  WHEN 6583 THEN -6582
  WHEN 6584 THEN -6582
  WHEN 6721 THEN 6071
  WHEN 6722 THEN 6071
  WHEN 6804 THEN -6804
  WHEN 6805 THEN -6804
  WHEN 7021 THEN 7021
  WHEN 7022 THEN 7022
  WHEN 7023 THEN 7022
  WHEN 7024 THEN 7021
  WHEN 7625 THEN -7625
  WHEN 7626 THEN -7626
  WHEN 7627 THEN -7626
  WHEN 7628 THEN -7626
  WHEN 7630 THEN -7625
  WHEN 7638 THEN 7638
  WHEN 7670 THEN 7638
  WHEN 7730 THEN -7730
  WHEN 7731 THEN -7730
  WHEN 7788 THEN 7788
  WHEN 7789 THEN 7922
  WHEN 7863 THEN 7863
  WHEN 7864 THEN 7863
  WHEN 7865 THEN 7863
  WHEN 7866 THEN 7866
  WHEN 7867 THEN 7866
  WHEN 7868 THEN 7866
  WHEN 7871 THEN 7788
  WHEN 7872 THEN 7788
  WHEN 7873 THEN 7788
  WHEN 7874 THEN 7922
  WHEN 7875 THEN 7922
  WHEN 7876 THEN 7922
  WHEN 7886 THEN 7788
  WHEN 7887 THEN 7788
  WHEN 7888 THEN 7788
  WHEN 7921 THEN 7788
  WHEN 7922 THEN 7922
  WHEN 7923 THEN 7922
  WHEN 7924 THEN 7922
  WHEN 7925 THEN 7922
  WHEN 8080 THEN 8080
  WHEN 8081 THEN 8080
  WHEN 8105 THEN 8105
  WHEN 8120 THEN 8120
  WHEN 8123 THEN 8123
  WHEN 8124 THEN 8123
  WHEN 8154 THEN 8080
  WHEN 8155 THEN 8080
  WHEN 8156 THEN 8080
  WHEN 8157 THEN 8080
  WHEN 8158 THEN 8080
  WHEN 8159 THEN 8080
  WHEN 8160 THEN 8123
  WHEN 8161 THEN 8123
  WHEN 8162 THEN 8123
  WHEN 8163 THEN 8123
  WHEN 8164 THEN 8123
  WHEN 8165 THEN 8123
  WHEN 8166 THEN 8105
  WHEN 8167 THEN 8105
  WHEN 8168 THEN 8105
  WHEN 8169 THEN 8120
  WHEN 8170 THEN 8120
  WHEN 8171 THEN 8120
  WHEN 8260 THEN 8260
  WHEN 8261 THEN 8260
  WHEN 8262 THEN 8260
  WHEN 8263 THEN 8263
  WHEN 8264 THEN 8263
  WHEN 8265 THEN 8263
  WHEN 8266 THEN 8266
  WHEN 8267 THEN 8266
  WHEN 8268 THEN 8268
  WHEN 8269 THEN 8268
  WHEN 8275 THEN 8275
  WHEN 8276 THEN 8275
  WHEN 8291 THEN 7788
  WHEN 8292 THEN 7788
  WHEN 8293 THEN 7922
  WHEN 8294 THEN 7922
  WHEN 8297 THEN 8080
  WHEN 8298 THEN 8080
  WHEN 8299 THEN 8123
  WHEN 8300 THEN 8123
  WHEN 8309 THEN -8309
  WHEN 8310 THEN -8309
  WHEN 8368 THEN 8368
  WHEN 8370 THEN 8370
  WHEN 8372 THEN 8372
  WHEN 8374 THEN 8374
  WHEN 8384 THEN 8374
  WHEN 8386 THEN 8372
  WHEN 8389 THEN 8368
  WHEN 8390 THEN 8370
  WHEN 8391 THEN 8374
  WHEN 8392 THEN 8374
  WHEN 8393 THEN 8374
  WHEN 8394 THEN 8374
  WHEN 8395 THEN 8374
  WHEN 8396 THEN 8374
  WHEN 8397 THEN 8374
  WHEN 8398 THEN 8374
  WHEN 8399 THEN 8372
  WHEN 8400 THEN 8372
  WHEN 8401 THEN 8372
  WHEN 8402 THEN 8372
  WHEN 8403 THEN 8372
  WHEN 8404 THEN 8372
  WHEN 8405 THEN 8372
  WHEN 8406 THEN 8372
  WHEN 8407 THEN 8372
  WHEN 8408 THEN 8372
  WHEN 8419 THEN 8419
  WHEN 8420 THEN 8419
  WHEN 8426 THEN 8368
  WHEN 8427 THEN 8368
  WHEN 8428 THEN 8368
  WHEN 8429 THEN 8368
  WHEN 8430 THEN 8368
  WHEN 8431 THEN 8368
  WHEN 8432 THEN 8368
  WHEN 8433 THEN 8368
  WHEN 8434 THEN 8368
  WHEN 8435 THEN 8368
  WHEN 8436 THEN 8370
  WHEN 8437 THEN 8370
  WHEN 8438 THEN 8370
  WHEN 8439 THEN 8370
  WHEN 8440 THEN 8370
  WHEN 8441 THEN 8370
  WHEN 8442 THEN 8370
  WHEN 8443 THEN 8370
  WHEN 8578 THEN -8578
  WHEN 8587 THEN -8578
  WHEN 8620 THEN -8578
  WHEN 8729 THEN -8729
  WHEN 8730 THEN -8729
  WHEN 8741 THEN -8729
  WHEN 8792 THEN 8792
  WHEN 8793 THEN 8792
  WHEN 8794 THEN 8792
  WHEN 8870 THEN 8870
  WHEN 8871 THEN 8870
  WHEN 8872 THEN 8870
  WHEN 8873 THEN 8870
  WHEN 8874 THEN 8870
  WHEN 8875 THEN 8870
  WHEN 8962 THEN 8962
  WHEN 8963 THEN 8962
  WHEN 8964 THEN 8962
  WHEN 8965 THEN 8962
  WHEN 9121 THEN 9121
  WHEN 9122 THEN 9121
  WHEN 9123 THEN 9121
  WHEN 9257 THEN 9257
  WHEN 9269 THEN 9257
  WHEN 9270 THEN 9257
  WHEN 9271 THEN 9257
  WHEN 9324 THEN -9324
  WHEN 9325 THEN -9324
  WHEN 9326 THEN -9324
  WHEN 9330 THEN -9330
  WHEN 9331 THEN -9330
  WHEN 9332 THEN -9330
  WHEN 9500 THEN 9500
  WHEN 9547 THEN 9547
  WHEN 9551 THEN 9547
  WHEN 9617 THEN 9617
  WHEN 9672 THEN 9672
  WHEN 9751 THEN 9672
  WHEN 9793 THEN 9793
  WHEN 9824 THEN -9824
  WHEN 9825 THEN -9824
  WHEN 9851 THEN -9851
  WHEN 9856 THEN -9851
  WHEN 9859 THEN -9851
  WHEN 9868 THEN -9868
  WHEN 9927 THEN -9927
  WHEN 9928 THEN -9927
  WHEN 9931 THEN -9931
  WHEN 9932 THEN -9931
  WHEN 9934 THEN -9868
  WHEN 9935 THEN -9935
  WHEN 9936 THEN -9936
  WHEN 9939 THEN -9935
  WHEN 9940 THEN -9936
  WHEN 9957 THEN 9957
  WHEN 9960 THEN 9957
  WHEN 9961 THEN 9957
  WHEN 10104 THEN 9793
  WHEN 10183 THEN 10183
  WHEN 10263 THEN 10263
  WHEN 10264 THEN 10263
  WHEN 10313 THEN -10313
  WHEN 10321 THEN -10313
  WHEN 10373 THEN 5066
  WHEN 10374 THEN 5093
  WHEN 10460 THEN 10460
  WHEN 10461 THEN 10460
  WHEN 10462 THEN 10460
  WHEN 10463 THEN 10460
  WHEN 10490 THEN 9500
  WHEN 10491 THEN 9547
  WHEN 10500 THEN 8792
  WHEN 10520 THEN 3763
  WHEN 10523 THEN -10523
  WHEN 10530 THEN 9617
  WHEN 10541 THEN -10523
  WHEN 10551 THEN 10551
  WHEN 10552 THEN 10551
  WHEN 10568 THEN 10568
  WHEN 10579 THEN -10523
  WHEN 10583 THEN -10583
  WHEN 10585 THEN -10583
  WHEN 10601 THEN -10601
  WHEN 10602 THEN -10601
  WHEN 10619 THEN 10619
  WHEN 10634 THEN -10634
  WHEN 10635 THEN -10634
  WHEN 10636 THEN -10634
  WHEN 10641 THEN -10641
  WHEN 10665 THEN -10665
  WHEN 10666 THEN -10665
  WHEN 10667 THEN -10667
  WHEN 10668 THEN -10641
  WHEN 10669 THEN -10641
  WHEN 10670 THEN -10667
  WHEN 10683 THEN 10568
  WHEN 10729 THEN 10729
  WHEN 10730 THEN 10729
  WHEN 10731 THEN 10729
  WHEN 10732 THEN 10729
  WHEN 10752 THEN 990
  WHEN 10789 THEN 1478
  WHEN 10790 THEN 1506
  WHEN 10807 THEN 10619
  WHEN 10831 THEN 10831
  WHEN 10832 THEN 10831
  WHEN 10833 THEN 10831
  WHEN 10862 THEN 10862
  WHEN 10863 THEN 10862
  WHEN 10884 THEN -10884
  WHEN 10885 THEN -10884
  WHEN 10886 THEN -10884
  WHEN 10897 THEN 10897
  WHEN 10899 THEN 10897
  WHEN 10902 THEN 10897
  WHEN 10905 THEN 10905
  WHEN 10906 THEN 10905
  WHEN 10907 THEN 10905
  WHEN 10908 THEN 10862
  WHEN 10995 THEN -10995
  WHEN 10996 THEN -10995
  WHEN 10997 THEN -10995
  WHEN 11016 THEN 11016
  WHEN 11017 THEN 11016
  WHEN 11018 THEN 11016
  WHEN 11030 THEN -11010
  WHEN 11031 THEN 11031
  WHEN 11032 THEN 11031
  WHEN 11033 THEN 11031
  WHEN 11034 THEN 11031
  WHEN 11036 THEN 10183
  WHEN 11037 THEN 10183
  WHEN 11040 THEN 10183
  WHEN 11042 THEN 10183
  WHEN 11058 THEN -11010
  WHEN 11109 THEN 11109
  WHEN 11110 THEN 11109
  WHEN 11111 THEN 11109
  WHEN 11112 THEN 11109
  WHEN 11113 THEN 11109
  WHEN 11114 THEN 11109
  WHEN 11135 THEN 11135
  WHEN 11211 THEN 11211
  WHEN 11214 THEN 11211
  WHEN 11215 THEN 11211
  WHEN 11220 THEN 11135
  WHEN 11293 THEN 11293
  WHEN 11294 THEN 11293
  WHEN 11354 THEN 11354
  WHEN 11362 THEN 11354
  WHEN 11363 THEN 11354
  WHEN 11364 THEN 11364
  WHEN 11368 THEN 11354
  WHEN 11369 THEN 11354
  WHEN 11370 THEN 11354
  WHEN 11371 THEN 11364
  WHEN 11372 THEN 11354
  WHEN 11373 THEN 11354
  WHEN 11374 THEN 11354
  WHEN 11375 THEN 11354
  WHEN 11376 THEN 11364
  WHEN 11377 THEN 11377
  WHEN 11378 THEN 11354
  WHEN 11379 THEN 11377
  WHEN 11380 THEN 11377
  WHEN 11381 THEN 11377
  WHEN 11382 THEN 11354
  WHEN 11383 THEN 11364
  WHEN 11384 THEN 11354
  WHEN 11385 THEN 11364
  WHEN 11386 THEN 11354
  WHEN 11387 THEN 11364
  WHEN 11388 THEN 11354
  WHEN 11389 THEN 11364
  WHEN 11407 THEN 11293
  WHEN 11408 THEN 11293
  WHEN 11441 THEN 11441
  WHEN 11442 THEN 11441
  WHEN 11446 THEN 11446
  WHEN 11447 THEN 11446
  WHEN 11499 THEN 11354
  WHEN 11500 THEN 11364
  WHEN 11665 THEN 11665
  WHEN 11666 THEN 11665
  WHEN 11667 THEN 11665
  WHEN 11668 THEN 11665
  WHEN 11669 THEN 11665
  WHEN 11933 THEN -9330
  WHEN 11935 THEN -9324
  ELSE `ExclusiveGroup`
END
WHERE `ExclusiveGroup` = 0
  AND `entry` IN (
    2,23,24,96,188,193,197,203,204,235,585,586,621,648,712,714,
    742,836,936,972,990,994,995,1000,1004,1015,1018,1019,1047,
    1079,1080,1083,1084,1118,1120,1121,1222,1258,1282,1302,
    1472,1478,1505,1506,1507,1522,1523,1528,1529,1531,1532,
    1638,1679,1684,1685,1715,1793,1794,1818,1859,1860,1879,
    1881,1883,1885,2205,2218,2241,2378,2380,2761,2762,2763,
    2767,2771,2772,2773,2848,2849,2850,2851,2852,2855,2856,
    2857,2858,2859,2983,2984,2985,2996,2997,2998,2999,3000,
    3001,3127,3128,3526,3629,3630,3631,3632,3633,3634,3635,
    3637,3638,3640,3642,3681,3762,3763,3784,3787,3788,3789,
    3790,4022,4023,4181,4285,4287,4288,4293,4294,4485,4486,
    4487,4488,4489,4737,4738,4962,4963,4964,4965,4967,4968,
    4969,4975,5066,5090,5091,5093,5094,5095,5141,5142,5143,
    5144,5145,5146,5148,5154,5168,5249,5250,5283,5284,5301,
    5302,5542,5543,5544,5601,5628,5629,5631,5634,5635,5636,
    5637,5638,5639,5641,5642,5643,5644,5645,5646,5647,5652,
    5654,5655,5656,5657,5658,5660,5661,5662,5663,5672,5673,
    5674,5675,5676,5677,5679,5680,5923,5924,5925,5926,5927,
    5928,6065,6066,6067,6068,6069,6070,6071,6072,6073,6074,
    6075,6076,6382,6541,6542,6582,6583,6584,6721,6722,6804,
    6805,7021,7022,7023,7024,7625,7626,7627,7628,7630,7638,
    7670,7730,7731,7788,7789,7863,7864,7865,7866,7867,7868,
    7871,7872,7873,7874,7875,7876,7886,7887,7888,7921,7922,
    7923,7924,7925,8080,8081,8105,8120,8123,8124,8154,8155,
    8156,8157,8158,8159,8160,8161,8162,8163,8164,8165,8166,
    8167,8168,8169,8170,8171,8260,8261,8262,8263,8264,8265,
    8266,8267,8268,8269,8275,8276,8291,8292,8293,8294,8297,
    8298,8299,8300,8309,8310,8368,8370,8372,8374,8384,8386,
    8389,8390,8391,8392,8393,8394,8395,8396,8397,8398,8399,
    8400,8401,8402,8403,8404,8405,8406,8407,8408,8419,8420,
    8426,8427,8428,8429,8430,8431,8432,8433,8434,8435,8436,
    8437,8438,8439,8440,8441,8442,8443,8578,8587,8620,8729,
    8730,8741,8792,8793,8794,8870,8871,8872,8873,8874,8875,
    8962,8963,8964,8965,9121,9122,9123,9257,9269,9270,9271,
    9324,9325,9326,9330,9331,9332,9500,9547,9551,9617,9672,
    9751,9793,9824,9825,9851,9856,9859,9868,9927,9928,9931,
    9932,9934,9935,9936,9939,9940,9957,9960,9961,10104,10183,
    10263,10264,10313,10321,10373,10374,10460,10461,10462,10463,
    10490,10491,10500,10520,10523,10530,10541,10551,10552,10568,
    10579,10583,10585,10601,10602,10619,10634,10635,10636,10641,
    10665,10666,10667,10668,10669,10670,10683,10729,10730,10731,
    10732,10752,10789,10790,10807,10831,10832,10833,10862,10863,
    10884,10885,10886,10897,10899,10902,10905,10906,10907,10908,
    10995,10996,10997,11016,11017,11018,11030,11031,11032,11033,
    11034,11036,11037,11040,11042,11058,11109,11110,11111,11112,
    11113,11114,11135,11211,11214,11215,11220,11293,11294,11354,
    11362,11363,11364,11368,11369,11370,11371,11372,11373,11374,
    11375,11376,11377,11378,11379,11380,11381,11382,11383,11384,
    11385,11386,11387,11388,11389,11407,11408,11441,11442,11446,
    11447,11499,11500,11665,11666,11667,11668,11669,11933,11935
  );
