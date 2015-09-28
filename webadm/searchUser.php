<?php
require_once('vpopadm.inc.php');

	if (!adminPerm(PERM_ADMIN_USERCONTROL) ){
?>
		<br>
		您没有访问该网页的权限。<br>
<?php
		return false;
	}
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>查找用户</TITLE>
</HEAD>
<BODY>
<DIV align="center">
<FORM action="userInfo.php" method="put">
<table>
<tbody>
<tr>
<td>请输入待查找的用户账号:</td>
<td><INPUT type="text" name="id"></td>
</td>
</tbody>
</table>
<INPUT type="submit" value="开始查找">
</form>
</div>
</BODY>
</HTML>