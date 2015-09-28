<?php
require_once("vpopadm.inc.php");
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>用户组管理</TITLE>
</HEAD>
<BODY>
<DIV align="center">
<h2>用户组管理</h2>
<?php

if (!adminPerm(PERM_ADMIN_USERCONTROL) ){
?>
<br>
您没有访问该网页的权限。<br>
<?php
} else {
if ($_POST['action']=='delete') {
	deleteGroup($_POST['group']);
} else if ($_POST['action']=='add') {
	addGroup($_POST['newGroup']);
} else if ($_POST['action']=='modify') {
	modifyGroup($_POST['group'],$_POST['newGroup']);
} 
showGroups();
}



?>
</div>
</BODY>
</HTML>
<?php

function modifyGroup($oldgroupname,$newgroupname){
	if (trim($newgroupname)=='') {
		return true;
	}
		$groupdefine_profile = VPOPMAILHOME . 'domains/' . DOMAIN . '/' . GROUPFILE;
   
    $h_groupdefine_profile = fopen ($groupdefine_profile,"a+");
   
    if ($h_groupdefine_profile == NULL ){
        echo "错误：用户组数据文件无法打开。<br>";
		exit(-1);
    }
   
    flock($h_groupdefine_profile, LOCK_EX);
    
    fseek($h_groupdefine_profile,0, SEEK_SET);

	$group_list=array();
	$group_count=0;
	$founded=false;

   	while (!feof($h_groupdefine_profile)){
	    $tmp = fgets($h_groupdefine_profile,1024); 
	    $groupinfo= explode( ',', $tmp);
		$groupname=trim($groupinfo[1]);
		if ($groupname!=''){
			if (!strcmp($groupname,$newgroupname)) {
?>
用户组名：<?php echo $newgroupname ;?>已存在！<br>
修改用户组失败。<br>
<?php
			   	flock($h_groupdefine_profile, LOCK_UN);
   	
			   	fclose($h_groupdefine_profile);
				return false;
			}
			if (!strcmp($groupname,$oldgroupname)) {
				$founded=true;
				$groupname=$newgroupname;
			}
			$index=intval($groupinfo[0]);
			$group_list[]=array($index,$groupname);
			$group_count++;
		}
	}

	if ($founded) {

		ftruncate($h_groupdefine_profile,0);

		for ($i=0;$i<$group_count;$i++){
			fputs($h_groupdefine_profile,$group_list[$i][0].', '.$group_list[$i][1]."\n");
		}
	}

   	flock($h_groupdefine_profile, LOCK_UN);
   	
   	fclose($h_groupdefine_profile);

	return true;
}

function deleteGroup($oldgroupname){
		$groupdefine_profile = VPOPMAILHOME . 'domains/' . DOMAIN . '/' . GROUPFILE;
   
    $h_groupdefine_profile = fopen ($groupdefine_profile,"a+");
   
    if ($h_groupdefine_profile == NULL ){
        echo "错误：用户组数据文件无法打开。<br>";
		exit(-1);
    }
   
    flock($h_groupdefine_profile, LOCK_EX);
    
    fseek($h_groupdefine_profile,0, SEEK_SET);

	$group_list=array();
	$group_count=0;
	$founded=false;

   	while (!feof($h_groupdefine_profile)){
	    $tmp = fgets($h_groupdefine_profile,1024); 
	    $groupinfo= explode( ',', $tmp);
		$groupname=trim($groupinfo[1]);
		if ($groupname!=''){
			if (!strcmp($groupname,$oldgroupname)) {
				$founded=true;
				continue;
			}
			$index=intval($groupinfo[0]);
			$group_list[]=array($index,$groupname);
			$group_count++;
		}
	}

	if ($founded) {

		ftruncate($h_groupdefine_profile,0);

		for ($i=0;$i<$group_count;$i++){
			fputs($h_groupdefine_profile,$group_list[$i][0].', '.$group_list[$i][1]."\n");
		}
	}

   	flock($h_groupdefine_profile, LOCK_UN);
   	
   	fclose($h_groupdefine_profile);

	return true;
}

function addGroup($newgroupname){
	if (trim($newgroupname)=='') {
		return true;
	}
	$groupdefine_profile = VPOPMAILHOME . 'domains/' . DOMAIN . '/' . GROUPFILE;
   
    $h_groupdefine_profile = fopen ($groupdefine_profile,"a+");
   
    if ($h_groupdefine_profile == NULL ){
        echo "错误：用户组数据文件无法打开。<br>";
		exit(-1);
    }
   
    flock($h_groupdefine_profile, LOCK_EX);
    
    fseek($h_groupdefine_profile,0, SEEK_SET);

//	$group_list=array();
//	$group_count=0;
	$max_count=0;

   	while (!feof($h_groupdefine_profile)){
	    $tmp = fgets($h_groupdefine_profile,1024); 
	    $groupinfo= explode( ',', $tmp);
		$groupname=trim($groupinfo[1]);
		if ($groupname!=''){
			if (!strcmp($groupname,$newgroupname)) {
?>
用户组名：<?php echo $newgroupname ;?>已存在！<br>
添加新用户组失败。<br>
<?php
			   	flock($h_groupdefine_profile, LOCK_UN);
   	
			   	fclose($h_groupdefine_profile);
				return false;
			}
			$index=intval($groupinfo[0]);
			if ($index>$max_count) {
				$max_count=$index;
			}
//			$group_list[]=array($index,$groupname);
//			$group_count++;
		}
	}
	$index=$max_count+1;

    fseek($h_groupdefine_profile,0, SEEK_END);

	fputs($h_groupdefine_profile,$index.', '.trim($newgroupname)."\n");

   	flock($h_groupdefine_profile, LOCK_UN);
   	
   	fclose($h_groupdefine_profile);

	return true;
}

function showGroups(){
	$groupdefine_profile = VPOPMAILHOME . 'domains/' . DOMAIN . '/' . GROUPFILE;
   
    if ( ! file_exists($groupdefine_profile) ){
	fclose ( fopen ( $groupdefine_profile, "w" ) );
    }

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
			$group_list[]=$groupinfo[1];
			$group_count++;
		}
	}

   	flock($h_groupdefine_profile, LOCK_UN);
   	
   	fclose($h_groupdefine_profile);
    
	if ($group_count<=0) {
?>
	目前尚无用户组定义！
<?php
	} else {
?>
现有用户组列表：
<select id="oGroupList" size=10 >
<?php
	for ($i=0;$i<$group_count;$i++){
?>
	<option <?php echo $i?'':'selected'; ?>><?php echo $group_list[$i] ?></option>
<?php
	}
?>
</select>
<?php
	}
?>
<hr width="97%">
<script language="JavaScript">
	function doIt(type){
		if (typeof(oGroupList) != "undefined") {
			document.all.oGroup.value=oGroupList.options(oGroupList.selectedIndex).text
		}
		document.all.oAction.value=type;
		return oForm.submit();
	}
		
</script>
<form action="<?php echo $_SERVER['PHP_SELF'] ; ?>" method="POST" id="oForm">
<input type="hidden" name="group" id="oGroup">
<input type="hidden" name="action" id="oAction">
用户组名：<input type="text" name="newGroup">
<input type="button" value="删除" onclick="doIt('delete');">
<input type="button" value="添加" onclick="doIt('add');">
<input type="button" value="修改" onclick="doIt('modify');">
</form>
<?php
}
?>
