<?php

require_once('vpopadm.inc.php');


if ((!isset($_REQUEST['adminID'])) || (!isset($_REQUEST['password'])) ){
?>
	<p>请正常<A HREF="index.php">登录</a>！</p>
<?php
	exit();
}


$id=$_REQUEST['adminID'];
$passwd=$_REQUEST['password'];
$result=isPasswordRight($id,$passwd);


if ( ($result==ERR_FORMAT_PASSWORD) || ($result==ERR_WRONGPASSWORD) ){
	errorReturn("密码错误,请重新输入","index.php");
}

$isSYSOPFirstLogin=false;
if ( ($result==ERR_FORMAT_ID) || ($result==ERR_NOSUCHID) ){
	if ($id!=SYSOPID) {
		errorReturn("无此管理员账号,请重新输入","index.php");
	} else {
		$isSYSOPFirstLogin=true;
	}
}


if ($isSYSOPFirstLogin){
	$_SESSION['SYSOPFirstLogin']='TRUE';
	header("Refresh: 0;URL=SYSOPFirstLogin.php");
} else {
	if ($id==SYSOPID) {
		$_SESSION['Privilidge']=PERM_ADMIN_MAX;
	} else {
		$result=getAdminInfo($id);
		if ($result['$returnCode']!=OK){
			errorReturn("无此管理员账号,请重新输入","index.php");
		}else {
			$_SESSION['Privilidge']=$result['privilidge'];
		}
	}

	$_SESSION['AdminID']=$id;
	header("Refresh: 0;URL=frames.php");
}
exit();

?>