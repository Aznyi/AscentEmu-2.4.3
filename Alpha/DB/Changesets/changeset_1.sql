-- Update Shrine of Dath Remars Object Type to match retail
UPDATE gameobject_names SET TYPE = 10 WHERE entry = 180516;

-- Fix Invalid loot for non existent item in issue 7
DELETE cl
FROM creatureloot cl
LEFT JOIN items i ON i.entry = cl.itemid
WHERE cl.entryid IN (24560, 25169)
  AND i.entry IS NULL;

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
