<?
require_once("vpopadm.inc.php");
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>用户列表</TITLE>
<style>

table { font-size:x-small;}

.title {font-size:medium;color:#ffffff;background-color:#6fa6e6;}
</style>
</HEAD>
<BODY>

<!-- Insert HTML here -->

<?

function addUser(){
	
	if (!adminPerm(PERM_ADMIN_ADMINCONTROL) ){
?>
		<br>
		您没有访问该网页的权限。<br>
<?php
		return false;
	}
//判断用户信息填写是否完整：
	if (!isset($_REQUEST['user_id'])){ 
		echo "错误：未输入用户ID<br>";
		return false;
	}
	if (!isset($_REQUEST['passwd1'])){ 
		echo "错误：未输入用户密码<br>";
		return false;
	}
	if (!isset($_REQUEST['passwd2'])){ 
		echo "错误：未确认输入用户密码<br>";
		return false;
	}
	if (!isset($_REQUEST['user_note'])){ 
		echo "错误：未输入备注<br>";
		return false;
	}

	if ($_REQUEST['user_id']==''){ 
		echo "错误：未输入用户ID<br>";
		return false;
	}
	if ($_REQUEST['passwd1']==''){ 
		echo "错误：未输入用户密码<br>";
		return false;
	}
	if ($_REQUEST['passwd2']==''){ 
		echo "错误：未确认输入用户密码<br>";
		return false;
	}


	
	$user_id = $_REQUEST['user_id'];		//用户ID
	$passwd1 = $_REQUEST['passwd1'];						
	$passwd2 = $_REQUEST['passwd2'];

	$user_note = $_REQUEST['user_note'];		//用户证件号码

    if ( $passwd1 != $passwd2 ) {
    	echo "错误：两次输入的密码不匹配<br>";
    	return false;
    }

	$privilidge=PERM_ADMIN_BASIC;

	if (isset($_POST['userControl'])) {
		$privilidge |=PERM_ADMIN_USERCONTROL;
	}

	if (isset($_POST['adminControl'])) {
		$privilidge |=PERM_ADMIN_ADMINCONTROL;
	}

	$result=addAdmin($user_id,$passwd1,$privilidge,$user_note);
	
	if ($result==ERR_FORMAT_PASSWORD) {
		errorReturn("密码含有非法字符,请重试",$_SERVER['PHP_SELF']);
	}
	if ($result==ERR_FORMAT_ID) {
		errorReturn("账号含有非法字符,请重试",$_SERVER['PHP_SELF']);
	}
	if ($result==ERR_FORMAT_PRIVILIDGE) {
		errorReturn("权限设置错误,请重试",$_SERVER['PHP_SELF']);
	}
	if ($result==ERR_FORMAT_NOTE) {
		errorReturn("备注中含有非法字符“:”,请重试",$_SERVER['PHP_SELF']);
	}
	if ($result==ERR_IDEXIST) {
		errorReturn("用户ID已存在,请重试",$_SERVER['PHP_SELF']);
	}

	if ($result==OK){
		return true;
	} 
   	
	return false;
}

if ( (isset($_REQUEST['addUser']) && addUser()) ){
	echo "管理员 ${_REQUEST['user_id'] }已经成功加入！<br>";
} else {


?>
<center>
<form action="<? echo $_SERVER['PHP_SELF']; ?>" method=post>
<INPUT type="hidden" name="addUser">
<table border=0>
<tr align="center" bgcolor=#6fa6e6>
<td colspan="3" class=title><b>添加新管理员</b></td>
</tr>
<tr>
	<td align="right">新管理员帐号：</td><td><input type=text name="user_id" value="<? echo $_REQUEST['user_id'] ?>">
	</td>
	<td><font color=#ff0000>请使用小写英文字母及数字起帐号名，必须使用英文字母开头。</font></td>
</tr>
<tr>
	<td align="right">输入密码：</td><td><input type=password name="passwd1"></td>
	<td><font color=#ff0000>请使用大小写英文字母和数字作为邮件密码。</font></td>
</tr>
<tr>
	<td align="right">确认密码：</td><td><input type=password name="passwd2"></td>
	<td><font color=#ff0000>请再次输入密码，两次密码必须相同。</font></td>
</tr>
<tr>
	<td align="right" >管理权限：</td><td colspan=2>
		<input type=checkbox name=userControl >用户管理<br>
		<input type=checkbox name=adminControl >管理员管理<br>
	</td>
<tr>
	<td align="right">备注：</td><td><input type=text name="user_note" value="<? echo $user_note ?>"></td>
	<td><font color=#ff0000></font></td>
</tr>
<tr align="center" >
	<td colspan=3><input type=submit name="adduser" value="  确  认  ">
		&nbsp;&nbsp;&nbsp;&nbsp;
	<input type=reset value="  清  除  ">
	</td>
</tr>
</table>

</form>
</br>
<?
}
?>
</BODY>
</HTML>
