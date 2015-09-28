<?
require_once("vpopadm.inc.php");
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>删除管理员</TITLE>
</HEAD>
<BODY>
<DIV align="center">
<?


function deleteUser() {

		if (!adminPerm(PERM_ADMIN_ADMINCONTROL) ){
?>
		<br>
		您没有访问该网页的权限。<br>
<?php
		return false;
	}

   	
        $id=$_REQUEST['id'];
	$selfid=getAdminID();

	if ($selfid==$id) {
		errorReturn("您不能删除自己的账号", "showadminlist.php");
	}

	
	if ($id==SYSOPID) {
		errorReturn("您不能删除最高管理员" . SYSOP . "的账号", "showadminlist.php");
	}

	$result=deleteAdmin($id);
	
	if ($result==OK){
?>
	管理员账号已成功删除！<br>
<?
	} else {
?>
	错误：未找到账号为<? echo $id ;?>的管理员信息<br>
<?
	}
}


deleteUser();
?>
</div>
</BODY>
</HTML>
