<?
require_once("vpopadm.inc.php");
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>修改密码信息</TITLE>
</HEAD>
<BODY>
<DIV align="center">
<?

function changeUserPasswd() {

	$id=getAdminID();

	if (!adminPerm(PERM_ADMIN_BASIC) ){
?>
		<br>
		您没有访问该网页的权限。<br>
<?php
		return false;
	}



	if ( isset($_REQUEST["changeUserPasswd"])){ //实际修改用户信息

	$passwd= $_REQUEST['passwd'];
	$passwd1 = $_REQUEST['passwd1'];						
	$passwd2 = $_REQUEST['passwd2'];

    if ( $passwd1 != $passwd2 ) {
    	echo "错误：两次输入的密码不匹配<br>";
    	return false;
    }

	$result=isPasswordRight($id,$passwd);

	if ( ($result==ERR_FORMAT_PASSWORD) || ($result==ERR_WRONGPASSWORD) ){
		errorReturn("密码错误,请重新输入",$_SERVER['PHP_SELF']);
	}

	if ( ($result==ERR_FORMAT_ID) || ($result==ERR_NOSUCHID) ){
		session_unset();
		session_destroy();
		errorReturn("无此管理员账号,请重新输入",$_SERVER['PHP_SELF']);
	} 

	if ($result!=OK) {
		errorReturn('其他错误，叫系统管理员来debug', $_SERVER['PHP_SELF']);
	}

	$result=setPassword($id,$passwd1);	

	if ( ($result==ERR_FORMAT_PASSWORD) ){
		errorReturn("密码格式错误,请重新输入",$_SERVER['PHP_SELF']);
	}

	if ( ($result==ERR_FORMAT_ID) || ($result==ERR_NOSUCHID) ){
		session_unset();
		session_destroy();
		errorReturn("无此管理员账号,请重新输入",$_SERVER['PHP_SELF']);
	} 

	if ($result!=OK) {
		errorReturn('其他错误，叫系统管理员来debug', $_SERVER['PHP_SELF']);
	}
		echo "密码修改成功！";
		return true;
	} 
	
?>
<FORM action="<? echo $_SERVER['PHP_SELF']; ?>" method="post">
<INPUT type="hidden" name="changeUserPasswd">
<table>
<tbody>
<tr>
<td>管理员账号</td>
<td><? echo $id; ?></td>
</tr>
<tr>
	<td>输入旧密码</td>
	<td><input type=password name="passwd"></td>
</tr>
<tr>
	<td>输入新密码</td>
	<td><input type=password name="passwd1"></td>
</tr>
<tr>
	<td>确认新密码</td>
	<td><input type=password name="passwd2"></td>
</tr>

</tbody>
</table>
<INPUT type="submit" value="提交修改信息">
</form>
<?php

}


changeUserPasswd();

?>
</div>
</BODY>
</HTML>