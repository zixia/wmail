<?
require_once('vpopadm.inc.php');

	if (!adminPerm(PERM_ADMIN_BASIC) ){
?>
		<br>
		您尚未登陆。<br>
<?php
		return false;
	}
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>邮件账户管理系统</TITLE>
</HEAD>
<frameset framespacing="0" border="0" rows="81,*,50" frameborder="0">
<FRAME SRC="<? echo siteFile('top.html'); ?>"> 
<FRAMESET COLS="170, *" FRAMEBORDER="0">
<FRAME SRC="menu.php" name="menu" >
<FRAME SRC="http://<?php echo $_SERVER['SERVER_NAME']; ?>/mailadm/showuserlist.php" name="xbody">
</FRAMESET>
<FRAME SRC="<? echo siteFile('bottom.html'); ?>">
</FRAMESET>
</HTML>
