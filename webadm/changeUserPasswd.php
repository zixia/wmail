<?
require_once("vpopadm.inc.php");
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>修改账户信息</TITLE>
</HEAD>
<BODY>
<DIV align="center">
<?

function changeUserPasswd() {

	if (!adminPerm(PERM_ADMIN_USERCONTROL) ){
?>
		<br>
		您没有访问该网页的权限。<br>
<?php
		return false;
	}

$passwd_file = VPOPMAILHOME . 'domains/' . DOMAIN . '/vpasswd';

$user_profile = VPOPMAILHOME . 'domains/' . DOMAIN . '/' . USERPROFILE;



if (!isset($_REQUEST['id'])){
?>
	错误:未指定用户账号！
<?
	return false;
} 

	if ( isset($_REQUEST["changeUserPasswd"])){ //实际修改用户信息

	$passwd1 = $_REQUEST['passwd1'];						
	$passwd2 = $_REQUEST['passwd2'];
    if (!eregi( "^[0-9a-z]+$", $passwd1 )) {
    	echo  "错误：密码中含有非法字符<br>";
    	return false;
    }
    if ( $passwd1 != $passwd2 ) {
    	echo "错误：两次输入的密码不匹配<br>";
    	return false;
    }

		$user_list = file( $passwd_file );
	 	
	    $mail_count=count($user_list);
		for( $i = 0 ; $i < $mail_count ; $i++)
		{
			list( $user_account, $xxx, $xxx, $xxx, $xxx, $xxx, $xxx )  = explode( ':', $user_list[$i] );
			if (!strcmp($user_account, $_REQUEST['id'])){
				   	system( VPOPMAILHOME . 'bin/vpasswd ' . $user_account . "@" . DOMAIN . ' ' . $passwd1 .' > /dev/null', $add_result );
				break;
			}
		}
	
		if ($i<$mail_count){
?>
	账号密码修改成功！<br>
<?
		} else {
?>
	错误：未找到账号为<? echo $_REQUEST['id'] ;?>的用户信息<br>
<?
		}		
		return true;
	} 
	
	//显示用户信息


	$user_list = file( $passwd_file );
 	
    $mail_count=count($user_list);
	for( $i = 0 ; $i < $mail_count ; $i++)
	{
		list( $user_account, $xxx, $xxx, $xxx, $user_name, $xxx, $user_quota )  = explode( ':', $user_list[$i] );
		if (!strcmp($user_account, $_REQUEST['id'])){
			break;
		}
	}
	if ($i<$mail_count){
?>

<FORM action="<? echo $_SERVER['PHP_SELF']; ?>" method="post">
<INPUT type="hidden" name="changeUserPasswd">
<table>
<tbody>
<tr>
<td>账号</td>
<INPUT type="hidden" name="id" value="<? echo $user_account; ?>">
<td><? echo $user_account; ?></td>
</tr>
<tr>
<td>姓名</td>
<td><? echo $user_name; ?></td>
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
	} else {
?>
	错误：未找到账号为<? echo $_REQUEST['id'] ;?>的用户信息
<?
	}
}


changeUserPasswd();

?>
</div>
</BODY>
</HTML>