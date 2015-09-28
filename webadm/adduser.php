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
		if (!adminPerm(PERM_ADMIN_USERCONTROL) ){
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
	if (!isset($_REQUEST['user_name'])){ 
		echo "错误：未输入用户姓名<br>";
		return false;
	}
	if (!isset($_REQUEST['user_unit'])){ 
		echo "错误：未输入用户所在单位<br>";
		return false;
	}
	if (!isset($_REQUEST['user_department'])){ 
		echo "错误：未输入用户所在部门<br>";
		return false;
	}
	if (!isset($_REQUEST['user_station'])){ 
		echo "错误：未输入用户岗位<br>";
		return false;
	}
	if (!isset($_REQUEST['user_id_code'])){ 
		echo "错误：未输入用户证件号码<br>";
		return false;
	}

	if (!isset($_REQUEST['user_box_size'])){ 
		echo "错误：未输入邮箱大小<br>";
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
	if ($_REQUEST['user_name']==''){ 
		echo "错误：未输入用户姓名<br>";
		return false;
	}
    if ( !ereg("^[0-9]+$", $_REQUEST['user_box_size'])){
    	echo "错误：用户邮箱大小未指定或其不是有效数字“:”<br>";
    	return false;
    }

	
	$user_id = $_REQUEST['user_id'];		//用户ID
	$passwd1 = $_REQUEST['passwd1'];						
	$passwd2 = $_REQUEST['passwd2'];
	$user_name = $_REQUEST['user_name'];	//用户姓名
	$user_unit = $_REQUEST['user_unit'];		//用户单位
	$user_department = $_REQUEST['user_department']; 	//用户部门
	$user_station = $_REQUEST['user_station']; 		//用户岗位
	$user_id_code = $_REQUEST['user_id_code'];		//用户证件号码
	$user_note = $_REQUEST['user_note'];		//用户证件号码
	$user_is_public = isset($_REQUEST['user_is_public']);
	$user_box_size = 1024 * 1024 * (intval($_REQUEST['user_box_size']) );

	if ($user_box_size<0) {
		$user_box_size=0;
	}

	if ($user_unit==''){
		$user_unit=' ';
	}
	if ($user_department==''){
		$user_department=' ';
	}
	if ($user_station==''){
		$user_station=' ';
	}
	if ($user_id_code==''){
		$user_id_code=' ';
	}
    if (!ereg( "^[a-z][_0-9a-z\.]*$", $user_id )) { //
        echo "错误：用户ID中含有非法字符<br>";
        return false;
    }
    if (!eregi( "^[0-9a-z]+$", $passwd1 )) {
    	echo  "错误：密码中含有非法字符<br>";
    	return false;
    }
    if ( $passwd1 != $passwd2 ) {
    	echo "错误：两次输入的密码不匹配<br>";
    	return false;
    }
    
    if ( !ereg("^[^:]+$", $user_name)){
    	echo "错误：用户姓名中含有非法字符“:”<br>";
    	return false;
    }

    if ( !ereg("^[^:]*$", $user_unit)){
    	echo "错误：用户单位中含有非法字符“:”<br>";
    	return false;
    }
    
    if ( !ereg("^[^:]*$", $user_department)){
    	echo "错误：用户部门中含有非法字符“:”<br>";
    	return false;
    }
    
    if ( !ereg("^[^:]*$", $user_station)){
    	echo "错误：用户岗位中含有非法字符“:”<br>";
    	return false;
    }
    
    if ( !ereg("^[^:]*$", $user_id_code)){
    	echo "错误：用户证件号码中含有非法字符“：”<br>";
    	return false;
    }
    if ( !ereg("^[^:]*$", $user_note)){
    	echo "错误：备注中含有非法字符“：”<br>";
    	return false;
    }

   	$user_profile = VPOPMAILHOME . 'domains/' . DOMAIN . '/' . USERPROFILE;
   	
    $h_user_profile = fopen ($user_profile,"a+");
   
    if ($h_user_profile == NULL ){
        echo "错误：用户数据文件无法打开。<br>";
		exit(-1);
    }
   
    flock($h_user_profile, LOCK_EX);
    
//向vpopmail系统中添加新用户
   	system( VPOPMAILHOME . 'bin/vadduser -c "' . $user_name . '" -q ' . $user_box_size . ' ' . $user_id . "@" . DOMAIN . ' "'  . $passwd1 . '" > /dev/null', $add_result );
/* 检查添加是否成功
   	if ($add_result!=0) {
   		echo "错误：用户id已存在<br>";
	   	flock($h_user_profile, LOCK_UN);
   		return false;
   	}
*/
	
	fseek($h_user_profile,0, SEEK_SET);
 	
   	while (!feof($h_user_profile)){
	    $userinfo = fgets($h_user_profile,1024); 
	    list ($id, $unit, $department, $station, $id_code, $create_time , $is_public, $note) = explode( ':', $userinfo);
	    if (!strcmp($id,$user_id)){
	    	echo "错误：用户id已存在！<br>";
		   	flock($h_user_profile, LOCK_UN);
	    	return false;
	    }
	}


	
	$user_create_time=date("Y-m-d");
   	
   	$userinfo=implode(":",array($user_id, $user_unit, $user_department, $user_station, $user_id_code, $user_create_time,$user_is_public, $user_note));
   	
	$userinfo .= "\n";
	
   	fseek($h_user_profile, 0, SEEK_END );
	
	fputs($h_user_profile,$userinfo);
   	
   	flock($h_user_profile, LOCK_UN);
   	
   	fclose($h_user_profile);
   	
	return true;
}

if ( (isset($_REQUEST['addUser']) && addUser()) ){
	echo "用户 ${_REQUEST['user_id'] }已经成功加入！<br>";
} else {


?>
<center>
<form action="<? echo $_SERVER['PHP_SELF']; ?>" method=post>
<INPUT type="hidden" name="addUser">
<table border=0>
<tr align="center" bgcolor=#6fa6e6>
<td colspan="2" class=title><b>添加新用户</b></td>
</tr>
<tr>
	<td>新帐号名：<input type=text name="user_id" value="<? echo $_REQUEST['user_id'] ?>">
	</td>
	<td><font color=#ff0000>请使用小写英文字母及数字起帐号名，必须使用英文字母开头。</font></td>
</tr>
<tr>
	<td>输入密码：<input type=password name="passwd1"></td>
	<td><font color=#ff0000>请使用大小写英文字母和数字作为邮件密码。</font></td>
</tr>
<tr>
	<td>确认密码：<input type=password name="passwd2"></td>
	<td><font color=#ff0000>请再次输入密码，两次密码必须相同。</font></td>
</tr>
<tr>
	<td>用户姓名：<input type=text name="user_name" value="<? echo $_REQUEST['user_name'] ?>"></td>
	<td><font color=#ff0000>用户的姓名。</font></td>
</tr>
<tr>
	<td>用户单位：<input type=text name="user_unit" value="<? echo $_REQUEST['user_unit'] ?>"></td>
	<td><font color=#ff0000>用户所在的单位。</font></td>
</tr>
<tr>
	<td>用户部门：<input type=text name="user_department" value="<? echo $_REQUEST['user_department'] ?>"></td>
	<td><font color=#ff0000>用户所在的部门。</font></td>
</tr>
<tr>
	<td>用户岗位：<input type=text name="user_station" value="<? echo $_REQUEST['user_station'] ?>"></td>
	<td><font color=#ff0000>用户的岗位。</font></td>
</tr>
<tr>
	<td>用户证件号码：<input type=text name="user_id_code" value="<? echo $_REQUEST['user_id_code'] ?>"></td>
	<td><font color=#ff0000>用户的证件号码。</font></td>
</tr>
<tr>
<?
	if (isset($_REQUEST['user_box_size'])) {
		$user_box_size=$_REQUEST['user_box_size'];
	} else {
		$user_box_size=$DEFAULTMAILBOXSIZE;
	}
?>
	<td>邮箱大小（M）：<input type=text name="user_box_size" value="<? echo $user_box_size ?>"></td>
	<td><font color=#ff0000>用户的邮箱大小。</font></td>
</tr>
<tr>
	<td><input type=checkbox name="user_is_public" <? echo ($user_is_public?'checked':''); ?>>对外显示</td>
	<td><font color=#ff0000>是否公开对外显示在公共邮件列表中</font></td>
</tr>
<tr>
	<td>备注：<input type=text name="user_note" value="<? echo $user_note ?>"></td>
	<td><font color=#ff0000></font></td>
</tr>
<tr align="center" >
	<td colspan=2><input type=submit name="adduser" value="  确  认  ">
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
