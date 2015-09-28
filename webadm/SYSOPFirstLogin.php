<?
require_once("vpopadm.inc.php");
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>初始化<?php echo SYSOPID; ?>@<?php echo DOMAIN; ?>密码</TITLE>
</HEAD>
<BODY>
<DIV align="center">
<?

function changeUserPasswd() {

$passwd_file = VPOPMAILHOME . 'domains/' . DOMAIN . '/vpasswd';

$user_profile = VPOPMAILHOME . 'domains/' . DOMAIN . '/' . USERPROFILE;

if ((!isset($_SESSION['SYSOPFirstLogin'])) && ($_SESSION['SYSOPFirstLogin']=='TRUE')){
?>
	该网页只用于系统初始化!
<?
	return false;
} 

	if ( isset($_REQUEST["changeUserPasswd"])){ //实际修改用户信息

	$passwd1 = $_REQUEST['passwd1'];						
	$passwd2 = $_REQUEST['passwd2'];
    if ( $passwd1 != $passwd2 ) {
    	echo "错误：两次输入的密码不匹配<br>";
    	return false;
    }

	$result=addAdmin(SYSOPID,$passwd1,PERM_ADMIN_MAX,'系统最高管理员');
	
	if ($result==ERR_FORMAT_PASSWORD) {
		errorReturn("密码含有非法字符,请重试",$_SERVER['PHP_SELF']);
	}
	if ($result==OK){
		$_SESSION['Privilidge']=PERM_ADMIN_MAX;
		$_SESSION['AdminID']=SYSOPID;
		unset($_SESSION['SYSOPFirstLogin']);
		errorReturn(SYSOPID . "密码初始化成功","frames.php");
	} else {
?>
	系统错误……请重试，如果错误重复出现请联络系统管理员
<?
		}		
		return true;
	} 
	

?>

<FORM action="<?php echo $_SERVER['PHP_SELF']; ?>" method="post">
<INPUT type="hidden" name="changeUserPasswd">
<table>
<tbody>
<tr>
<td>系统最高管理员账号</td>
<td><?php echo SYSOPID; ?>@<?php echo DOMAIN; ?></td>
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
<?
}


changeUserPasswd();

?>
</div>
</BODY>
</HTML>
