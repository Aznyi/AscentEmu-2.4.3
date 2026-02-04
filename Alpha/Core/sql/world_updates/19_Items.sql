ALTER TABLE items CHANGE RequiredSkillSubRank `requiredspell` mediumint(8) unsigned NOT NULL DEFAULT '0';
ALTER TABLE items CHANGE RequiredPlayerRank1 `requiredhonorrank` mediumint(8) unsigned NOT NULL DEFAULT '0';
ALTER TABLE items CHANGE RequiredPlayerRank2 `RequiredCityRank` mediumint(8) unsigned NOT NULL DEFAULT '0';
ALTER TABLE items CHANGE RequiredFaction `RequiredReputationFaction` smallint(5) unsigned NOT NULL DEFAULT '0';
ALTER TABLE items CHANGE RequiredFactionStanding `RequiredReputationRank` smallint(5) unsigned NOT NULL DEFAULT '0';