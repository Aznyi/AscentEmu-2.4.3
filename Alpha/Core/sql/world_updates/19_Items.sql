ALTER TABLE items CHANGE RequiredSkillSubRank `requiredspell` mediumint unsigned NOT NULL DEFAULT '0';
ALTER TABLE items CHANGE RequiredPlayerRank1 `requiredhonorrank` mediumint unsigned NOT NULL DEFAULT '0';
ALTER TABLE items CHANGE RequiredPlayerRank2 `RequiredCityRank` mediumint unsigned NOT NULL DEFAULT '0';
ALTER TABLE items CHANGE RequiredFaction `RequiredReputationFaction` smallint unsigned NOT NULL DEFAULT '0';
ALTER TABLE items CHANGE RequiredFactionStanding `RequiredReputationRank` smallint unsigned NOT NULL DEFAULT '0';