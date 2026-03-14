-- Timed world-state spawn examples for Ascent 2.4.3.
--
-- These bindings are meant for server-wide aftermath displays such as the
-- Onyxia and Nefarian heads. They are separate from the recurring holiday
-- game_event tables because they are triggered by gameplay, not the calendar.
--
-- This script uses the currently selected database. In HeidiSQL or the MySQL
-- client, select your Ascent world database before running it.
--
-- Binding mode:
--   1 = show this spawn only while the state is active
--   2 = hide this spawn while the state is active
--
-- Typical GM flow after binding a head display:
--   .spawnstate set stormwind_onyxia_head 6h
--   .spawnstate info stormwind_onyxia_head
--   .spawnstate clear stormwind_onyxia_head

CREATE TABLE IF NOT EXISTS `spawn_state` (
  `state_key` varchar(64) NOT NULL,
  `active_until` int(10) unsigned NOT NULL default '0',
  `description` varchar(255) NOT NULL default '',
  PRIMARY KEY (`state_key`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='Timed world-state spawns';

CREATE TABLE IF NOT EXISTS `spawn_state_creature` (
  `guid` int(10) unsigned NOT NULL,
  `state_key` varchar(64) NOT NULL,
  `mode` tinyint(3) unsigned NOT NULL default '1',
  PRIMARY KEY (`guid`,`state_key`),
  KEY `state_key` (`state_key`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='Timed world-state creature bindings';

CREATE TABLE IF NOT EXISTS `spawn_state_gameobject` (
  `guid` int(10) unsigned NOT NULL,
  `state_key` varchar(64) NOT NULL,
  `mode` tinyint(3) unsigned NOT NULL default '1',
  PRIMARY KEY (`guid`,`state_key`),
  KEY `state_key` (`state_key`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='Timed world-state gameobject bindings';

-- Example placeholder state rows.
REPLACE INTO `spawn_state` (`state_key`, `active_until`, `description`) VALUES
('stormwind_onyxia_head', 0, 'Stormwind Onyxia head display'),
('orgrimmar_onyxia_head', 0, 'Orgrimmar Onyxia head display'),
('stormwind_nefarian_head', 0, 'Stormwind Nefarian head display'),
('orgrimmar_nefarian_head', 0, 'Orgrimmar Nefarian head display');

-- Replace guid values below with the creature or gameobject spawn ids from your world DB.
-- Positive binding: the display only exists while the state is active.
-- INSERT INTO `spawn_state_creature` (`guid`, `state_key`, `mode`) VALUES
-- (12345, 'stormwind_onyxia_head', 1),
-- (12346, 'orgrimmar_onyxia_head', 1);

-- Known head display entries from the converted TBC data:
--   Orgrimmar:
--     179556 = Severed Head of Onyxia
--     179881 = Severed Head of Nefarian
--   Stormwind:
--     179558 = Severed Head of Onyxia
--     179882 = Severed Head of Nefarian
--
-- Bind every matching spawn by entry. This keeps the binding tied to the
-- actual SQL spawn ids that the runtime refresh path uses.
INSERT INTO `spawn_state_gameobject` (`guid`, `state_key`, `mode`)
SELECT `id`, 'orgrimmar_onyxia_head', 1
FROM `gameobject_spawns`
WHERE `Entry` = 179556;

INSERT INTO `spawn_state_gameobject` (`guid`, `state_key`, `mode`)
SELECT `id`, 'orgrimmar_nefarian_head', 1
FROM `gameobject_spawns`
WHERE `Entry` = 179881;

INSERT INTO `spawn_state_gameobject` (`guid`, `state_key`, `mode`)
SELECT `id`, 'stormwind_onyxia_head', 1
FROM `gameobject_spawns`
WHERE `Entry` = 179558;

INSERT INTO `spawn_state_gameobject` (`guid`, `state_key`, `mode`)
SELECT `id`, 'stormwind_nefarian_head', 1
FROM `gameobject_spawns`
WHERE `Entry` = 179882;

-- Negative binding example:
-- Hide a normal spawn while a special state is active.
-- INSERT INTO `spawn_state_gameobject` (`guid`, `state_key`, `mode`) VALUES
-- (30001, 'stormwind_onyxia_head', 2);
