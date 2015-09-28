<?
require_once("vpopadm.inc.php");
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>Anti-Spam SMTP设置</TITLE>
<style>

table { font-size:x-small;}

.title {font-size:medium;color:#ffffff;background-color:#6fa6e6;}
</style>
</HEAD>
<BODY>

<!-- Insert HTML here -->

<?

function doConfig(){
		if (!adminPerm(PERM_ADMIN_ADMINCONTROL) ){
?>
		<br>
		您没有访问该网页的权限。<br>
<?php
		return false;
	}
	if (!isset($_POST['type'])){ 
		return false;
	}

	$handle=fopen("/var/qmail/control/mfcheck","w");	
	if (!$handle) {
		echo "错误，无法保存设置！<br>";
		return false;
	}
	fputs($handle,$_POST['type']);
	fclose($handle);

	$handle=fopen("/var/qmail/control/mxcheckrefuse","w");	
	if (!$handle) {
		echo "错误，无法保存设置！<br>";
		return false;
	}
	fputs($handle,isset($_POST['refuse'])?"1":"0");
	fclose($handle);
	return true;
}

if ( (isset($_REQUEST['doConfig']) && doConfig()) ){
	echo "设置修改成功！<br>";
} else {
	if ( ! file_exists("/var/qmail/control/mfcheck") ){
		fclose( fopen("/var/qmail/control/mfcheck","w") );
	}

	$handle=fopen("/var/qmail/control/mfcheck","r");
	if ($handle) {
		$info=fscanf($handle,"%d");
		list($config)=$info;
		$config=intval($config);
		if ( ($config<0) || ($config>4) ) {
			$config=0;
		}
		fclose($handle);
	} else {
		$config=0;
	}

	if ( ! file_exists("/var/qmail/control/mxcheckrefuse") ){
		fclose( fopen("/var/qmail/control/mxcheckrefuse","w") );
	}

	$handle=fopen("/var/qmail/control/mxcheckrefuse","r");
	if ($handle) {
		$info=fscanf($handle,"%d");
		list($config2)=$info;
		$config2=intval($config2);
		if ( ($config2<0) || ($config2>4) ) {
			$config2=0;
		}
		fclose($handle);
	} else {
		$config2=0;
	}
?>
<center>
<form action="<? echo $_SERVER['PHP_SELF']; ?>" method=post>
<INPUT type="hidden" name="doConfig">
<table border=0>
<tr align="center" bgcolor=#6fa6e6>
<td colspan="2" class=title><b>设置Anti-Spam SMTP参数</b></td>
</tr>
<tr>
	<td colspan="2">Anti-Spam级别设定：<?php 
	$note=array("无","低","高(推荐)","很高","最高");
	for ($i=0;$i<5;$i++) {
?>
<input type=radio name="type" value="<?php echo $i ;?>" <?php if ($config==$i) echo "checked"?>><?php echo $note[$i]; ?>
<?php
	}
	?>
	</td>
</tr>
<tr>
	<td colspan=2>
	<input type=checkbox name="refuse" <?php if ($config2) echo "checked"; ?>>直接拒收可疑信件</input>
	</td>
</tr>
<tr align="center" >
	<td colspan=2><input type=submit name="adduser" value="  修  改  ">
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
