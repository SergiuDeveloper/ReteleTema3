USE `sys`;
DROP procedure IF EXISTS `sp_GetLocalServerPath`;

DELIMITER $$
USE `sys`$$
CREATE PROCEDURE `sp_GetLocalServerPath` ()
BEGIN
	SELECT Value FROM EnvironmentVariables WHERE Variable = "LocalServerPath";
END$$

DELIMITER ;
