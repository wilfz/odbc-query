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

if exists (select 1 from sysobjects where name = 'testproc' and type = 'P')
begin
	drop procedure testproc
	print 'testproc deleted ...'
end
go
--

create procedure testproc ( 
	@intvalin int,
	@intvalio int output,
	@strin char(255),
	@strio char(255) output)
as
begin
	set nocount on
	
	set @intvalio = @intvalin + @intvalio
	set @strio = rtrim(@strin) + rtrim(@strio)
end
go
grant exec on testproc to public
go

/*
declare @intvalio int
set @intvalio = 7
declare @strio varchar(32)
set @strio = 'JKL'
exec testproc 2, @intvalio output, 'GHI', @strio output 
select @intvalio, @strio
*/
