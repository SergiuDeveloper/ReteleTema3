USE `sys`;
DROP procedure IF EXISTS `sp_IsWhitelistedClient`;

DELIMITER $$
USE `sys`$$
CREATE PROCEDURE `sp_IsWhitelistedClient` (clientIP VARCHAR(16), clientMAC VARCHAR(19))
BEGIN
	SELECT (COUNT(*) > 0) FROM ClientIPsWhitelist
		JOIN ClientMACs ON ClientMACs.ClientIPID = ClientIPsWhitelist.ID
		WHERE ClientIPsWhitelist.ClientIP = clientIP AND ClientMACs.ClientMAC = clientMAC;
END$$

DELIMITER ;
