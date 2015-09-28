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
global $DEFAULTMAILBOXSIZE;
		if (!adminPerm(PERM_ADMIN_USERCONTROL) )
	{
?>
		<br>
		您没有访问该网页的权限。<br>
<?php
		return false;
	}

//判断用户信息填写是否完整：
	if (!isset($_REQUEST['data'])){ 
		echo "错误：未输入批量用户数据<br>";
		return false;
	}
	$tmp_userlist=split("\n",$_REQUEST['data']);
	$userlist=array();
	foreach ($tmp_userlist as $user){
		if (trim($user)=='') {
			continue;
		}
		$usercontent=split(',',$user);
		$passwd=rand();
		$userlist[]=array('id'=>$usercontent[0],'name'=>$usercontent[1],'department'=>$usercontent[2],'unit'=>$usercontent[3],'station'=>$usercontent[4],'code'=>$usercontent[5],'passwd'=>$passwd,'boxsize'=>$DEFAULTMAILBOXSIZE);
	}



    

	foreach ($userlist as $user){
		$user_id = $user['id'];		//用户ID
		$passwd = $user['passwd'];						
	    $user_name = $user['name'];	//用户姓名
		$user_unit = $user['unit'];		//用户单位
		$user_department = $user['department']; 	//用户部门
		$user_station = $user['station']; 		//用户岗位
		$user_id_code = $user['code'];		//用户证件号码
		$user_note = '';		//用户证件号码
		$user_is_public = 'N';
		$user_box_size = 1024 * 1024 * intval($user['boxsize']);

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
		if (!ereg( "^[A-Za-z][_0-9A-Za-z\.]*$", $user_id )) { //
			echo "错误：用户ID {$user_id} 中含有非法字符<br>";
			return false;
		}

		
		if ( !ereg("^[^:]+$", $user_name)){
			echo "错误：用户姓名 {$user_name} 中含有非法字符“:”<br>";
			return false;
		}

		if ( !ereg("^[^:]*$", $user_unit)){
			echo "错误：用户单位 {$user_unit} 中含有非法字符“:”<br>";
			return false;
		}
		
		if ( !ereg("^[^:]*$", $user_department)){
			echo "错误：用户部门 {$user_department}中含有非法字符“:”<br>";
			return false;
		}
		
		if ( !ereg("^[^:]*$", $user_station)){
			echo "错误：用户岗位 {$user_station} 中含有非法字符“:”<br>";
			return false;
		}
		
		if ( !ereg("^[^:]*$", $user_id_code)){
			echo "错误：用户证件号码 {$user_id_code} 中含有非法字符“：”<br>";
			return false;
		}
	}

   	$user_profile = VPOPMAILHOME . 'domains/' . DOMAIN . '/' . USERPROFILE;
   	
    $h_user_profile = fopen ($user_profile,"a+");
   
    if ($h_user_profile == NULL ){
        echo "错误：用户数据文件无法打开。<br>";
		exit(-1);
    }
   
    flock($h_user_profile, LOCK_EX);

	foreach ($userlist as $user){
		$user_id = $user['id'];		//用户ID
		$passwd = $user['passwd'];						
	    $user_name = $user['name'];	//用户姓名
		$user_unit = $user['unit'];		//用户单位
		$user_department = $user['department']; 	//用户部门
		$user_station = $user['station']; 		//用户岗位
		$user_id_code = $user['code'];		//用户证件号码
		$user_note = '';		//用户证件号码
		$user_is_public = 'N';
		$user_box_size = 1024 * 1024 * intval($user['boxsize']);
		
		fseek($h_user_profile,0, SEEK_SET);
		$is_exist=false;
		while (!feof($h_user_profile)){
			$userinfo = fgets($h_user_profile,1024); 
			list ($id, $unit, $department, $station, $id_code, $create_time , $is_public, $note) = explode( ':', $userinfo);
			if (!strcmp($id,$user_id)){
				$is_exist=true;
				break;
			}
		}

		if ($is_exist){
			echo $user_name."的账号".$user_id."已存在<br>\n";
			continue;
		}

		system( VPOPMAILHOME . 'bin/vadduser -c "' . $user_name . '" -q ' . $user_box_size . ' ' . $user_id . "@" . DOMAIN . ' "'  . $passwd . '" > /dev/null', $add_result );
		
		$user_create_time=date("Y-m-d");
		
		$userinfo=implode(":",array($user_id, $user_unit, $user_department, $user_station, $user_id_code, $user_create_time,$user_is_public, $user_note));
		
		$userinfo .= "\n";
		
		fseek($h_user_profile, 0, SEEK_END );
		
		fputs($h_user_profile,$userinfo);
	}
   	flock($h_user_profile, LOCK_UN);
   	
   	fclose($h_user_profile);

?>
<table>
<thead>
<th>用户ID</th>
<th>用户姓名</th>
<th>用户单位</th>
<th>用户部门</th>
<th>用户岗位</th>
<th>用户证件号码</th>
<th>用户密码</th>
</thead>
<tbody>

<?php
	foreach ($userlist as $user){
		echo '<tr>';
		echo "<td>{$user['id']} </td>";
		echo "<td>{$user['name']} </td>";
		echo "<td>{$user['unit']} </td>";
		echo "<td>{$user['department']} </td>";
		echo "<td>{$user['station']} </td>";
		echo "<td>{$user['code']} </td>";
		echo "<td>{$user['passwd']} </td>";
		echo '</tr>';
	}
?>
</tbody>
</table>
<?php
  	
	return true;
}

if ( (isset($_REQUEST['addUser']) && addUser()) ){
	echo "批量用户数据已经成功导入！<br>";
} else {
	
?>
<center>
<form action="<? echo $_SERVER['PHP_SELF']; ?>" method=post>
<INPUT type="hidden" name="addUser">
<table border=0>
<tr align="center" bgcolor=#6fa6e6>
<td colspan="2" class=title><b>批量添加新用户</b></td>
</tr>
<tr align="center" >
	<td colspan=2><textarea name="data" ></textarea>
	</td>
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
