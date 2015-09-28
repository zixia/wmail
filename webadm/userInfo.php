<?
require_once("vpopadm.inc.php");


?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>账户信息</TITLE>
</HEAD>
<style>
table {font-size:x-small;}

.title {color:#ffffff;background-color:#6fa6e6;font-size:medium;}
</style>
<BODY>

<DIV align="center">

<?


function userInfo() {

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

    $h_user_profile = fopen ($user_profile,"a+"); 
   
    if ($h_user_profile == NULL ){
        echo "错误：用户数据文件无法打开。<br>";
		return false;
    }
   
    flock($h_user_profile, LOCK_SH);
    
    fseek($h_user_profile,0, SEEK_SET);

	$userinfo_list=array();

   	while (!feof($h_user_profile)){
	    $userinfo = fgets($h_user_profile,1024); 
	    list ($id, $unit, $department, $station, $id_code, $create_time, $is_public, $note, $group) = explode( ':', $userinfo);
		$isPublic=$is_public?'Y':'N';

	    array_push($userinfo_list,array("id" => $id,
	    								"unit" => $unit,
	    								"department" => $department,
	    								"station" => $station,
	    								"id_code" => $id_code,
	    								"create_time" => $create_time,
										"is_public" => $isPublic,
										"note" => $note,
										"group" => $group)
	    );
	}

   	flock($h_user_profile, LOCK_UN);
   	
   	fclose($h_user_profile);
    

	$user_list = file( $passwd_file );
 	
    $mail_count=count($user_list);
	list($unit, $department, $station, $id_code, $create_time)=array("","","","","");
	for( $i = 0 ; $i < $mail_count ; $i++)
	{
		list( $user_account, $xxx, $xxx, $xxx, $user_name, $xxx, $user_quota )  = explode( ':', $user_list[$i] );
		list($unit, $department, $station, $id_code, $create_time,$is_public,$note,$group)=array("","","","","","N","","");
                        if (strpos($user_quota,'M')===false){
                                $user_quota_num=intval($user_quota/(1024*1024));
                        } else {
                                $user_quota_num=substr($user_quota,0,strpos($user_quota,'M'));
                        }
		if (!strcmp($user_account, $_REQUEST['id'])){
			for ($t=0; $t<count($userinfo_list); $t++){
				if (!strcmp($user_account, $userinfo_list[$t]['id'])) {
					break;
				} 
			}
			if ($t<count($userinfo_list)){
				$unit=rtrim($userinfo_list[$t]['unit']);
				$department=rtrim($userinfo_list[$t]['department']);
				$station=rtrim($userinfo_list[$t]['station']);
				$id_code=rtrim($userinfo_list[$t]['id_code']);
				$create_time=rtrim($userinfo_list[$t]['create_time']);
				$is_public=rtrim($userinfo_list[$t]['is_public']);
				$note=rtrim($userinfo_list[$t]['note']);
				$group=rtrim($userinfo_list[$t]['group']);
			}
			break;
		}
	}
	if ($i<$mail_count){
?>

<table border="0" >
<tbody>
<tr class="title" align="center">
<td colspan="2">
用户信息
</td>
</tr>
<tr>
<td >账号</td>
<td><? echo $user_account; ?></td>
</tr>
<tr>
<td>姓名</td>
<td><? echo $user_name; ?></td>
</tr>
<tr>
<td>单位</td>
<td><? echo $unit; ?></td>
</tr>
<tr>
<td>部门</td>
<td><? echo $department; ?></td>
</tr>
<tr>
<td>岗位</td>
<td><? echo $station; ?></td>
</tr>
<tr>
<td>证件号码</td>
<td><? echo $id_code; ?></td>
</tr>
<tr>
<td>账号建立时间</td>
<td><? echo $create_time; ?></td>
</tr>
<tr>
<td>邮箱容量</td>
<td><? echo $user_quota_num; ?> MB</td>
</tr>
<tr>
<td>对外公开？</td>
<td><? echo $is_public; ?></td>
</tr><tr>
<td valign="top">所属用户组</td>
<td align="left"><?php
	if ($group=='') {
		echo '无';
	} else {
		$groups=explode(',', $group);
		foreach( $groups as $groupi) {
			echo $groupi."<br>";
		}
	}
	?></td>
</tr>
<td>备注</td>
<td><? echo $note; ?></td>
</tr>
<SCRIPT language="JScript">
function modifyUserInfo(){
	document.location.href="modifyUserInfo.php?id=<? echo $_REQUEST['id'] ?>";
}
function changeUserPasswd(){
	document.location.href="changeUserPasswd.php?id=<? echo $_REQUEST['id'] ?>";
}
function deleteUser(){
	var reply=confirm("真的要删除用户<? echo $user_account; ?>吗？");
	if (reply) {
		document.location.href="deleteUser.php?id=<? echo $_REQUEST['id'] ?>";
	}
}

</script>
<tr>
<td colspan="2">
<table width="450">
<tr>
<td align="center"><INPUT type="button" value="修改此用户信息" onclick="return modifyUserInfo();"></td>
<td align="center"><input type="button" value="修改此用户密码" onclick="return changeUserPasswd();"></td>
<td align="center"><INPUT type="button" value="删除此用户" onclick="deleteUser();"></td>
</tr>
</table>
</td></tr>
</tbody></table>
<?
	} else {
?>
	错误：未找到账号为<? echo $_REQUEST['id'] ;?>的用户信息
<?
	}
}


userInfo();
?>
</div>
</BODY>
</HTML>
