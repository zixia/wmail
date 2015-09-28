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

function modifyUserInfo() {

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

	if ( isset($_REQUEST["modifyUserInfo"])){ //实际修改用户信息
	    $h_user_profile = fopen ($user_profile,"a+");
	   
	    if ($h_user_profile == NULL ){
	        echo "错误：用户数据文件无法打开。<br>";
			return false;
	    }
   
	    flock($h_user_profile, LOCK_EX);
	    
	    fseek($h_user_profile,0, SEEK_SET);
	
		$userinfo_list=array();
	
	   	while (!feof($h_user_profile)){
		    $userinfo = fgets($h_user_profile,1024); 
		    list ($id, $unit, $department, $station, $id_code, $create_time, $is_public, $note, $group) = explode( ':', $userinfo);
			array_push($userinfo_list,array("id" => rtrim($id),
	    									"unit" => rtrim($unit),
	    									"department" => rtrim($department),
	    									"station" => rtrim($station),
	    									"id_code" => rtrim($id_code),
	    									"create_time" => rtrim($create_time),
											"is_public" => rtrim($is_public),
											"note" => rtrim($note),
											"group" => rtrim($group))
			);
		}
	
	    
	
		$user_list = file( $passwd_file );
	 	
	    $mail_count=count($user_list);
		for( $i = 0 ; $i < $mail_count ; $i++)
		{
			list( $user_account, $xxx, $xxx, $xxx, $xxx, $xxx, $xxx )  = explode( ':', $user_list[$i] );
			if (!strcmp($user_account, $_REQUEST['id'])){
				ftruncate($h_user_profile,0);
				$isInfoed=false;
				$user_box_size = 1024 * 1024 * ($_REQUEST['user_box_size'] );
				if ($user_box_size<0) {
					$user_box_size=0;
				}
			   	system( VPOPMAILHOME . 'bin/vmoduser -c "' . $_REQUEST['user_name'] . '" -q ' . $user_box_size . ' ' . $user_account . "@" . DOMAIN . ' > /dev/null', $add_result );
				for ($t=0; $t<count($userinfo_list); $t++){
					if (!strcmp("", $userinfo_list[$t]['id'])) {
						continue;
					}
					if (!strcmp($user_account, $userinfo_list[$t]['id'])){
						$isInfoed=true;
						$user_id=$user_account;
						$user_unit=$_REQUEST["user_unit"];
						$user_department=$_REQUEST["user_department"];
						$user_station=$_REQUEST["user_station"];
						$user_id_code=$_REQUEST["user_id_code"];
						$user_is_public = isset($_REQUEST['user_is_public']);
						$user_note = $_REQUEST['user_note'];
						$user_create_time=$userinfo_list[$t]["create_time"];							
						$user_group= $_POST['groups'];

					} else {
						$user_id=$userinfo_list[$t]['id'];
						$user_unit=rtrim($userinfo_list[$t]['unit']);
						$user_department=rtrim($userinfo_list[$t]['department']);
						$user_station=rtrim($userinfo_list[$t]['station']);
						$user_id_code=rtrim($userinfo_list[$t]['id_code']);
						$user_create_time=rtrim($userinfo_list[$t]['create_time']);
						$user_is_public=rtrim($userinfo_list[$t]['is_public']);
						$user_note=rtrim($userinfo_list[$t]['note']);
						$user_group=rtrim($userinfo_list[$t]['group']);
					}
					$userinfo=implode(":",array($user_id, $user_unit, $user_department, $user_station, $user_id_code, $user_create_time,$user_is_public, $user_note, $user_group));
	   				$userinfo .= "\n";
	   				fputs($h_user_profile,$userinfo);
				}
				if (!$isInfoed){
					$user_id=$user_account;
					$user_unit=$_REQUEST["user_unit"];
					$user_department=$_REQUEST["user_department"];
					$user_station=$_REQUEST["user_station"];
					$user_id_code=$_REQUEST["user_id_code"];
					$user_create_time=date("Y-m-d");
					$create_time=$userinfo_list[$t]['create_time'];
					$user_is_public = isset($_REQUEST['user_is_public']);
					$user_note = $_REQUEST['user_note'];
					$user_group = $_POST['groups'];

					$userinfo=implode(":",array($user_id, $user_unit, $user_department, $user_station, $user_id_code, $user_create_time,$user_is_public, $user_note, $user_group));
		   			$userinfo .= "\n";
		   			fputs($h_user_profile,$userinfo);
				}
				break;
			}
		}
	
   		flock($h_user_profile, LOCK_UN);
   	
   		fclose($h_user_profile);
	
		if ($i<$mail_count){
?>
	账号信息修改成功！<br>
<?
		} else {
?>
	错误：未找到账号为<? echo $_REQUEST['id'] ;?>的用户信息<br>
<?
		}		
		return true;
	} 
	
	//显示用户信息
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
	    array_push($userinfo_list,array("id" => $id,
	    								"unit" => $unit,
	    								"department" => $department,
	    								"station" => $station,
	    								"id_code" => $id_code,
	    								"create_time" => $create_time,
										"is_public" => $is_public,
										"note" => $note,
										"group" => $group)
	    );
	}

   	flock($h_user_profile, LOCK_UN);
   	
   	fclose($h_user_profile);
    

	$user_list = file( $passwd_file );
 	
    $mail_count=count($user_list);
	list($unit, $department, $station, $id_code, $create_time,$is_public,$note , $group) = array("","","","","","","");
	for( $i = 0 ; $i < $mail_count ; $i++)
	{
		list( $user_account, $xxx, $xxx, $xxx, $user_name, $xxx, $user_quota )  = explode( ':', $user_list[$i] );
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

<FORM id=oForm action="<? echo $_SERVER['PHP_SELF'] ?>" method="post">
<INPUT type="hidden" name="modifyUserInfo">
<table>
<tbody>
<tr>
<td>账号</td>
<INPUT type="hidden" name="id" value="<? echo $user_account; ?>">
<td><? echo $user_account; ?></td>
</tr>
<tr>
<td>姓名</td>
<td><input type="text" name="user_name" value="<? echo $user_name; ?>"></td>
</tr>
<tr>
<td>单位</td>
<td><input type="text" name="user_unit" value="<? echo $unit; ?>"></td>
</tr>
<tr>
<td>部门</td>
<td><input type="text" name="user_department" value="<? echo $department; ?>"></td>
</tr>
<tr>
<td>岗位</td>
<td><input type="text" name="user_station" value="<? echo $station; ?>"></td>
</tr>
<tr>
<td>证件号码</td>
<td><input type="text" name="user_id_code" value="<? echo $id_code; ?>"></td>
</tr>
<tr>
<td>账号建立时间</td>
<td><? echo $create_time; ?></td>
</tr>
<tr>
<td>邮箱容量</td>
<td><input type="text" name="user_box_size" value="<? echo $user_quota_num; ?> ">MB</td>
</tr>
<tr>
	<td colspan=2><input type=checkbox name="user_is_public" <? echo ($is_public?'checked':''); ?>>对外显示</td>
</tr>
<tr>
<td>用户所属组</td>
<td><select id="oGroupList" size=5 multiple><? 
	$groupdefine_profile = VPOPMAILHOME . 'domains/' . DOMAIN . '/' . GROUPFILE;
   
    $h_groupdefine_profile = fopen ($groupdefine_profile,"r");
   
    if ($h_groupdefine_profile == NULL ){
        echo "错误：用户组数据文件无法打开。<br>";
		exit(-1);
    }
   
    flock($h_groupdefine_profile, LOCK_SH);
    

	$group_list=array();
	$group_count=0;

   	while (!feof($h_groupdefine_profile)){
	    $tmp = fgets($h_groupdefine_profile,1024); 
	    $groupinfo= explode( ',', $tmp);
		if (trim($groupinfo[1])!=''){
			$group_list[]=trim($groupinfo[1]);
			$group_count++;
		}
	}

   	flock($h_groupdefine_profile, LOCK_UN);
   	
   	fclose($h_groupdefine_profile);

	$groups=explode(',',$group);

	for ($i=0;$i<$group_count;$i++){
?>
	<option <?php echo in_array($group_list[$i], $groups)?'selected':''; ?>><?php echo $group_list[$i] ?></option>
<?php
	}

	?></select></td>
</tr>
<tr>
	<td>备注</td>
	<td><input type=text name="user_note" value="<? echo $note ?>"></td>
</tr>
</tbody>
</table>
<script>
function doModify(){
	var dot=false;
	for (i=0;i<document.all.oGroupList.length;i++){
		if (document.all.oGroupList.options(i).selected) {
			if (dot) {
					document.all.oGroups.value+=',';
			} else {
				dot=true;
			}
			document.all.oGroups.value+=document.all.oGroupList.options(i).text;
		}
	}
	oForm.submit();
}
</script>
<input type="hidden" name="groups" id="oGroups" >
<INPUT type="button" value="提交修改信息" onclick="doModify();">
</form>
<?
	} else {
?>
	错误：未找到账号为<? echo $_REQUEST['id'] ;?>的用户信息
<?
	}
}


modifyUserInfo();

?>
</div>
</BODY>
</HTML>
