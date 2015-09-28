<?php
require_once("vpopadm.inc.php");

	if (!adminPerm(PERM_ADMIN_USERCONTROL) ){
?>
		<br>
		您没有访问该网页的权限。<br>
<?php
		exit(0);
	}

$passwd_file = VPOPMAILHOME . 'domains/' . DOMAIN . '/vpasswd';

$user_profile = VPOPMAILHOME . 'domains/' . DOMAIN . '/' . USERPROFILE;

    $h_user_profile = fopen ($user_profile,"a+");
   
    if ($h_user_profile == NULL ){
        echo "错误：用户数据文件无法打开。<br>";
		exit(-1);
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
										"group"=> str_replace(',', '，', $group))
	    );
	}

   	flock($h_user_profile, LOCK_UN);
   	
   	fclose($h_user_profile);
    

	$user_list = file( $passwd_file );
 	
    $mail_count=count($user_list);

?>
account,name,unit,department,station,idCode,createTime,quota,isPublic,note,groups
<?
		for( $i = 0 ; $i < $mail_count ; $i++)
		{
			list( $user_account, $xxx, $xxx, $xxx, $user_name, $xxx, $user_quota )  = explode( ':', $user_list[$i] );
			if (strpos($user_quota,'M')===false){
				$user_quota_num=intval($user_quota/(1024*1024));
			} else {
			 	$user_quota_num=substr($user_quota,0,strpos($user_quota,'M'));
			}
			list($unit, $department, $station, $id_code, $create_time,$is_public,$note,$group)=array("","","","","","N","","");
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
			 echo "{$user_account},{$user_name},{$unit},{$department},{$station},{$id_code},{$create_time},{$user_quota_num},{$is_public}, {$note}, {$group}";
			if ($i<$mail_count-1) 
				echo "\n";
 
		}
?> 
