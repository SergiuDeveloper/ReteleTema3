USE `sys`;

DELIMITER $$
USE `sys`$$
CREATE DEFINER=`root`@`localhost` PROCEDURE `sp_LogLocalServerPath`(localServerPath VARCHAR(256))
BEGIN
	IF (SELECT COUNT(*) FROM EnvironmentVariables WHERE Variable = "LocalServerPath") > 0 THEN
		UPDATE EnvironmentVariables SET Value = localServerPath WHERE Variable = "LocalServerPath";
	ELSE
		INSERT INTO EnvironmentVariables (Variable, Value) VALUES ("LocalServerPath", localServerPath);
	END IF;
END$$

DELIMITER ;
;
