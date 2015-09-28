<?
require_once("vpopadm.inc.php");
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>账户删除</TITLE>
</HEAD>
<BODY>
<DIV align="center">
<?


function deleteUser() {

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
   
    flock($h_user_profile, LOCK_EX);
    
    fseek($h_user_profile,0, SEEK_SET);

	$userinfo_list=array();

   	while (!feof($h_user_profile)){
	    $userinfo = fgets($h_user_profile,1024); 
		list ($id, $unit, $department, $station, $id_code, $create_time, $is_public, $note) = explode( ':', $userinfo);
		array_push($userinfo_list,array("id" => rtrim($id),
	    								"unit" => rtrim($unit),
	    								"department" => rtrim($department),
	    								"station" => rtrim($station),
	    								"id_code" => rtrim($id_code),
	    								"create_time" => rtrim($create_time),
										"is_public" => rtrim($is_public),
										"note" => rtrim($note))
		);
	}

    

	$user_list = file( $passwd_file );
 	
    $mail_count=count($user_list);
	list($unit, $department, $station, $id_code, $create_time,$is_public,$note)=array("","","","","","","");
	for( $i = 0 ; $i < $mail_count ; $i++)
	{
		list( $user_account, $xxx, $xxx, $xxx, $user_name, $xxx, $user_quota )  = explode( ':', $user_list[$i] );
		$user_quota_num=$user_quota/1000000+0.5;
		if (!strcmp($user_account, $_REQUEST['id'])){
			ftruncate($h_user_profile,0);
			for ($t=0; $t<count($userinfo_list); $t++){
				if ( (strcmp($user_account, $userinfo_list[$t]['id'])) 
				  && (strcmp("", $userinfo_list[$t]['id'])) ) {
   					$userinfo=implode(":",$userinfo_list[$t]);
   					$userinfo .= "\n";					
   					fputs($h_user_profile,$userinfo);
				} 
			}
			system( VPOPMAILHOME . "bin/vdeluser " . $user_account . '@' . DOMAIN . ' > /dev/null' , $del_result );
			break;
		}
	}
	
   	flock($h_user_profile, LOCK_UN);
   	
   	fclose($h_user_profile);
	
	if ($i<$mail_count){
?>
	账号已成功删除！<br>
<?
	} else {
?>
	错误：未找到账号为<? echo $_REQUEST['id'] ;?>的用户信息<br>
<?
	}
}


deleteUser();
?>
</div>
</BODY>
</HTML>
