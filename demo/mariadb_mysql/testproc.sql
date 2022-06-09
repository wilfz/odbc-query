/* -----------------------------------------------------------------------------
 *  (c) copyright 2018 by Wilfried Zimmermann
 * -----------------------------------------------------------------------------
 * procedure:       testproc()
 * -----------------------------------------------------------------------------
 * function :	
 * -----------------------------------------------------------------------------
 * remarks:	
 * -----------------------------------------------------------------------------
 */
 
DROP PROCEDURE IF EXISTS testproc;
DELIMITER //
CREATE PROCEDURE testproc(
	IN intvalin INT, 
	INOUT intvalio INT,
	IN strin CHAR(255),
	INOUT strio CHAR(255))
LANGUAGE SQL
SQL SECURITY DEFINER
BEGIN
     SET intvalio = intvalin + intvalio;
     SET strio = concat(strin, strio);
END; //
DELIMITER ;
/*
set @a = 5;
set @b = 'DEF';
call testproc( 3, @a, 'ABC', @b);
select @a, @b;
*/
