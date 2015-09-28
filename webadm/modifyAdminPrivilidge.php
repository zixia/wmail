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

	if (!adminPerm(PERM_ADMIN_ADMINCONTROL) ){
?>
		<br>
		您没有访问该网页的权限。<br>
<?php
		return false;
	}



	if ( isset($_REQUEST["changeUserPasswd"])){ //实际修改用户信息

	$selfID=getAdminID();

	$id=$_REQUEST['adminID'];

	if ($selfID==$id) {
		errorReturn("错误：您不能修改自己的权限", "showadminlist.php");
	}
	


	if ($id=='') {
		errorReturn("错误：未输入管理员账号",$_SERVER['PHP_SELF']);
	}

	if ($id==SYSOPID) {
		errorReturn("错误：您无权修改最高管理员". SYSOPID ."的权限","showadminlist.php");
	}

       $privilidge=PERM_ADMIN_BASIC;

        if (isset($_POST['userControl'])) {
                $privilidge |=PERM_ADMIN_USERCONTROL;
        }

        if (isset($_POST['adminControl'])) {
                $privilidge |=PERM_ADMIN_ADMINCONTROL;
        }

	$result=modifyAdminPrivilidge($id,$privilidge);	
	

	if ( ($result==ERR_FORMAT_PRIVILIDGE) ){
		errorReturn("程序错误，请联络系统管理员",$_SERVER['PHP_SELF']);
	}

	if ( ($result==ERR_FORMAT_ID) || ($result==ERR_NOSUCHID) ){
		session_unset();
		session_destroy();
		errorReturn("无此管理员账号,请重新输入",$_SERVER['PHP_SELF']);
	} 
	
	if ($result!=OK) {
		errorReturn('其他错误，叫系统管理员来debug', $_SERVER['PHP_SELF']);
	}
		echo "权限修改成功！";
		return true;
	} 
	
?>
<FORM action="<? echo $_SERVER['PHP_SELF']; ?>" method="post">
<INPUT type="hidden" name="changeUserPasswd">
<table>
<tbody>
<tr>
<td>管理员账号：</td>
<td><input type=hidden name="adminID" value="<?php echo $_REQUEST['id']; ?>"><?php echo $_REQUEST['id']; ?></td>
</tr>
<tr>
        <td align="right" >管理权限：</td><td>
                <input type=checkbox name=userControl >用户管理<br>
                <input type=checkbox name=adminControl >管理员管理<br>
        </td>
<tr>
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
