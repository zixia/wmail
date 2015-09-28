<?php


define("ADMINFILE","ADMINFILE"); //用于保存管理员信息的数据文件

define("SYSOPID", "SYSOP"); //最高管理员ID

//返回值定义
define("OK",0);
define("ERR_NOSUCHID",-100);

define("ERR_FORMAT_ID",-201);
define("ERR_FORMAT_PASSWORD",-202);
define("ERR_FORMAT_PRIVILIDGE",-203);
define("ERR_FORMAT_NOTE",-204);

define("ERR_IDEXIST",-300);
define("ERR_WRONGPASSWORD",-400);

define("PERM_ADMIN_BASIC",0x0001);
define("PERM_ADMIN_USERCONTROL",0x0002);
define("PERM_ADMIN_ADMINCONTROL",0x0004);

define("PERM_ADMIN_MAX",0xffffffff);


$AdminDataFile= VPOPMAILHOME . 'domains/' . DOMAIN . '/' . ADMINFILE;


function havePerm($privilidge,$PermVal){
	return (intval($privilidge) & $PermVal);
}

function adminPerm($PermVal){
	if (isset($_SESSION['Privilidge'])) {
		return havePerm($_SESSION['Privilidge'],$PermVal);
	} else {
		return false;
	}
}

function getAdminID(){
	return $_SESSION['AdminID'];
}

//判断ID是否合法
function isIDFormatOK($id){
	return ereg( "^[A-Za-z][_0-9A-Za-z\.]*$", $id );
}

//判断Password是否合法
function isPasswordFormatOK($passwd){
	return eregi( "^[0-9a-z]+$", $passwd );
}

//判断权限是否合法
function isPrivilidgeFormatOK($privilidge){
	return intval($privilidge);
}

//判断备注是否合法
function isNoteFormatOK($note){
	return ereg("^[^:]*$", $note);
}



//添加管理员
function addAdmin($id, $passwd, $privilidge, $note){
	global $AdminDataFile;

	if (!isIDFormatOK($id)){
		return ERR_FORMAT_ID;
	}

	if (!isPasswordFormatOK($passwd)){
		return ERR_FORMAT_PASSWORD;
	}

	if (!isPrivilidgeFormatOK($privilidge)){
		return ERR_FORMAT_PRIVILIDGE;
	}

	if (!isNoteFormatOK($id)){
		return ERR_FORMAT_NOTE;
	}


    $hAdminProfile = fopen ($AdminDataFile,"a+");
   
    if ($hAdminProfile == NULL ){
        echo "错误：管理员数据文件无法打开。<br>";
		exit(-1);
    }

	flock($hAdminProfile, LOCK_EX);

	fseek($hAdminProfile,0, SEEK_SET);

	while (!feof($hAdminProfile)){
	    $adminInfo = fgets($hAdminProfile,1024); 
	    list ($adminId, $adminEncodedPasswd,$Privilidge,$Note) = explode( ':', $adminInfo);
	    if (!strcmp($adminId,$id)){
	    	echo "错误：用户id已存在！<br>";
		   	flock($hAdminProfile, LOCK_UN);
	    	return ERR_IDEXIST;
	    }
	}

	$encodedPasswd=crypt($passwd);

	$adminInfo=implode(":",array($id, $encodedPasswd, $privilidge, $note));
   	
	$adminInfo .= "\n";
	
   	fseek($hAdminProfile, 0, SEEK_END );
	
	fputs($hAdminProfile,$adminInfo);
   	
   	flock($hAdminProfile, LOCK_UN);
   	
   	fclose($hAdminProfile);

	return OK;
   
}


//判断输入密码是否正确
function isPasswordRight($id, $passwd){
		global $AdminDataFile;

	if (!isIDFormatOK($id)){
		return ERR_FORMAT_ID;
	}




    $hAdminProfile = fopen ($AdminDataFile,"r");
   
    if ($hAdminProfile == NULL ){
        echo "错误：管理员数据文件无法打开。<br>";
		exit(-1);
    }

	flock($hAdminProfile, LOCK_SH);

	fseek($hAdminProfile,0, SEEK_SET);

	$isFind=false;

	while (!feof($hAdminProfile)){
	    $adminInfo = fgets($hAdminProfile,1024); 
	    list ($adminId, $encodedPasswd,$Privilidge,$Note) = explode( ':', $adminInfo);
	    if (!strcmp($adminId,$id)){
			$isFind=true;
			break;
	    }
	}

   	flock($hAdminProfile, LOCK_UN);
   	
   	fclose($hAdminProfile);

	if (!$isFind) {
		return ERR_NOSUCHID;
	}

	if (!isPasswordFormatOK($passwd)){
		return ERR_FORMAT_PASSWORD;
	}

	if (crypt($passwd, $encodedPasswd)!=$encodedPasswd) {
		return ERR_WRONGPASSWORD;
	}

	return OK;

}

//读取管理员信息
function getAdminInfo($id){
	global $AdminDataFile;

    $hAdminProfile = fopen ($AdminDataFile,"r");
   
    if ($hAdminProfile == NULL ){
        echo "错误：管理员数据文件无法打开。<br>";
		exit(-1);
    }

	flock($hAdminProfile, LOCK_SH);

	fseek($hAdminProfile,0, SEEK_SET);

	$isFind=false;

	while (!feof($hAdminProfile)){
	    $adminInfo = fgets($hAdminProfile,1024); 
	    list ($adminId, $adminEncodedPasswd,$Privilidge,$Note) = explode( ':', $adminInfo);
	    if (!strcmp($adminId,$id)){
			$isFind=true;
			break;
	    }
	}

   	flock($hAdminProfile, LOCK_UN);
   	
   	fclose($hAdminProfile);

	if (!$isFind) {
		return array("returnCode"=>ERR_NOSUCHID);
	}

	return array("returnCode"=>OK, "privilidge"=>$Privilidge, "note"=>$Note);

}

function setPassword($id, $passwd){
		global $AdminDataFile;

	if (!isIDFormatOK($id)){
		return ERR_FORMAT_ID;
	}




    $hAdminProfile = fopen ($AdminDataFile,"a+");
   
    if ($hAdminProfile == NULL ){
        echo "错误：管理员数据文件无法打开。<br>";
		exit(-1);
    }

	flock($hAdminProfile, LOCK_EX);

	fseek($hAdminProfile,0, SEEK_SET);

	$isFind=false;

	$userinfo_list=array();
	
	while (!feof($hAdminProfile)){
	    $adminInfo = fgets($hAdminProfile,1024); 
	    list ($adminId, $encodedPasswd,$Privilidge,$Note) = explode( ':', $adminInfo);
	    if (!strcmp($adminId,$id)){
			$isFind=true;
	    }
		array_push($userinfo_list,array("id" => rtrim($adminId),
	    									"passwd" => rtrim($encodedPasswd),
	    									"privilidge" => rtrim($Privilidge),
	    									"note" => rtrim($Note)
									)
													
		);
	}


	if (!$isFind) {
	   	flock($hAdminProfile, LOCK_UN);
   	
   		fclose($hAdminProfile);
		return ERR_NOSUCHID;
	}

	if (!isPasswordFormatOK($passwd)){
	   	flock($hAdminProfile, LOCK_UN);
   	
   		fclose($hAdminProfile);
		return ERR_FORMAT_PASSWORD;
	}

	ftruncate($hAdminProfile,0);	


	for ($t=0; $t<count($userinfo_list); $t++){
		if (!strcmp("", $userinfo_list[$t]['id'])) {
				continue;
		}
		if (!strcmp($id, $userinfo_list[$t]['id'])){
			$encodedPasswd=crypt($passwd);
		} else {
			$encodedPasswd=$userinfo_list[$t]['passwd'];
		}
		$adminID=$userinfo_list[$t]['id'];
		$privilidge=$userinfo_list[$t]['privilidge'];
		$note=$userinfo_list[$t]['note'];
		$userinfo=implode(":",array($adminID, $encodedPasswd, $privilidge, $note));
		$userinfo .= "\n";
		fputs($hAdminProfile , $userinfo);
	}

	   	flock($hAdminProfile, LOCK_UN);
   	
   		fclose($hAdminProfile);

	return OK;

}

function getAdminList(){
	global $AdminDataFile;

    $hAdminProfile = fopen ($AdminDataFile,"r");
   
    if ($hAdminProfile == NULL ){
        echo "错误：管理员数据文件无法打开。<br>";
		exit(-1);
    }

	flock($hAdminProfile, LOCK_SH);

	fseek($hAdminProfile,0, SEEK_SET);


	$admin_list=array();
	while (!feof($hAdminProfile)){
	    $adminInfo = fgets($hAdminProfile,1024); 
	    list ($adminId, $adminEncodedPasswd,$Privilidge,$Note) = explode( ':', $adminInfo);
		if (rtrim($adminId)=='') continue;
		array_push($admin_list,array("id" => rtrim($adminId),
	    									"passwd" => rtrim($encodedPasswd),
	    									"privilidge" => rtrim($Privilidge),
	    									"note" => rtrim($Note)
									)
		);
	}

   	flock($hAdminProfile, LOCK_UN);
   	
   	fclose($hAdminProfile);

	return $admin_list;
}

function modifyAdminPrivilidge($id,$Perm){
		global $AdminDataFile;

	if (!isIDFormatOK($id)){
		return ERR_FORMAT_ID;
	}

    $hAdminProfile = fopen ($AdminDataFile,"a+");
   
    if ($hAdminProfile == NULL ){
        echo "错误：管理员数据文件无法打开。<br>";
		exit(-1);
    }

	flock($hAdminProfile, LOCK_EX);

	fseek($hAdminProfile,0, SEEK_SET);

	$isFind=false;

	$userinfo_list=array();
	
	while (!feof($hAdminProfile)){
	    $adminInfo = fgets($hAdminProfile,1024); 
	    list ($adminId, $encodedPasswd,$Privilidge,$Note) = explode( ':', $adminInfo);
	    if (!strcmp($adminId,$id)){
			$isFind=true;
	    }
		array_push($userinfo_list,array("id" => rtrim($adminId),
	    									"passwd" => rtrim($encodedPasswd),
	    									"privilidge" => rtrim($Privilidge),
	    									"note" => rtrim($Note)
									)
													
		);
	}


	if (!$isFind) {
	   	flock($hAdminProfile, LOCK_UN);
   	
   		fclose($hAdminProfile);
		return ERR_NOSUCHID;
	}

	if (!isPrivilidgeFormatOK($Perm)){
	   	flock($hAdminProfile, LOCK_UN);
   	
   		fclose($hAdminProfile);
		return ERR_FORMAT_PRIVILIDGE;
	}

	ftruncate($hAdminProfile,0);	


	for ($t=0; $t<count($userinfo_list); $t++){
		if (!strcmp("", $userinfo_list[$t]['id'])) {
				continue;
		}
		if (!strcmp($id, $userinfo_list[$t]['id'])){
			$privilidge=$Perm;
		} else {
			$privilidge=$userinfo_list[$t]['privilidge'];
		}
		$adminID=$userinfo_list[$t]['id'];
                $encodedPasswd=$userinfo_list[$t]['passwd'];
		$note=$userinfo_list[$t]['note'];
		$userinfo=implode(":",array($adminID, $encodedPasswd, $privilidge, $note));
		$userinfo .= "\n";
		fputs($hAdminProfile , $userinfo);
	}

	   	flock($hAdminProfile, LOCK_UN);
   	
   		fclose($hAdminProfile);

	return OK;

}

function deleteAdmin($id){

		global $AdminDataFile;

	if (!isIDFormatOK($id)){
		return ERR_FORMAT_ID;
	}

    $hAdminProfile = fopen ($AdminDataFile,"a+");
   
    if ($hAdminProfile == NULL ){
        echo "错误：管理员数据文件无法打开。<br>";
		exit(-1);
    }

	flock($hAdminProfile, LOCK_EX);

	fseek($hAdminProfile,0, SEEK_SET);

	$isFind=false;

	$userinfo_list=array();
	
	while (!feof($hAdminProfile)){
	    $adminInfo = fgets($hAdminProfile,1024); 
	    list ($adminId, $encodedPasswd,$Privilidge,$Note) = explode( ':', $adminInfo);
	    if (!strcmp($adminId,$id)){
			$isFind=true;
			continue;
	    }
		array_push($userinfo_list,array("id" => rtrim($adminId),
	    									"passwd" => rtrim($encodedPasswd),
	    									"privilidge" => rtrim($Privilidge),
	    									"note" => rtrim($Note)
									)
													
		);
	}

	if (!$isFind) {
		flock($hAdminProfile, LOCK_UN);
		fclose($hAdminProfile);
		return ERR_NOSUCHID;
	}



	ftruncate($hAdminProfile,0);	


	for ($t=0; $t<count($userinfo_list); $t++){
		if (!strcmp("", $userinfo_list[$t]['id'])) {
				continue;
		}
		$privilidge=$userinfo_list[$t]['privilidge'];
		$adminID=$userinfo_list[$t]['id'];
                $encodedPasswd=$userinfo_list[$t]['passwd'];
		$note=$userinfo_list[$t]['note'];
		$userinfo=implode(":",array($adminID, $encodedPasswd, $privilidge, $note));
		$userinfo .= "\n";
		fputs($hAdminProfile , $userinfo);
	}

	   	flock($hAdminProfile, LOCK_UN);
   	
   		fclose($hAdminProfile);

	return OK;

}




?>
