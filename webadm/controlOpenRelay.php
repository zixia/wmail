<?
require_once("vpopadm.inc.php");
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>Open Relay Deny功能设置</TITLE>
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
	if (!isset($_POST['content'])){ 
		return false;
	}
	$handle=fopen("/var/qmail/control/openrelaylist","w");	
	if (!$handle) {
		echo "错误，无法保存设置！<br>";
		return false;
	}
	fputs($handle,str_replace("\r","",$_POST['content']));
	fclose($handle);
	$handle=fopen("/var/qmail/control/openrelaycontrol","w");	
	if (!$handle) {
		echo "错误，无法保存设置！<br>";
		return false;
	}
	fputs($handle,($_POST['OpenRelay']=='true')?'1':'0');
	fclose($handle);
	return true;
}

if ( (isset($_REQUEST['doConfig']) && doConfig()) ){
	echo "设置保存成功！<br>";
} else {
	if ( ! file_exists("/var/qmail/control/openrelaycontrol") ){
		fclose( fopen("/var/qmail/control/openrelaycontrol","w") );
	}

	$handle = fopen ("/var/qmail/control/openrelaycontrol", "r");
	if ($handle) {
		$info=fscanf($handle,"%d");
		list($config)=$info;
		$config=intval($config);
		if ( ($config!=1) ) {
			$config=0;
		}
		fclose($handle);
	} else {
		$config=0;
	}

	if ( ! file_exists("/var/qmail/control/openrelaylist") ){
		fclose( fopen("/var/qmail/control/openrelaylist","w") );
	}

	$handle = fopen ("/var/qmail/control/openrelaylist", "r");
	$list="";
	while (!feof ($handle)) {
		$buffer= fgets($handle, 4096);
		$list.=$buffer;
	}
	fclose ($handle);

	?>
		<center>
		<form action="<? echo $_SERVER['PHP_SELF']; ?>" method=post>
		<INPUT type="hidden" name="doConfig">
		<table border=0>
		<tr align="center" bgcolor=#6fa6e6>
		<td colspan="2" class=title><b>设置Open Relay Deny功能</b></td>
		</tr>
		<td colspan="2"><input type="checkbox" name="OpenRelay" value="true" <?php if ($config) echo "checked" ;?> >打开Open Relay Deny功能
		</td>
		<tr>
		<td colspan="2">Open Relay列表服务器地址列表
		</td>
		</tr>
		<tr>
		<td colspan="2"><textarea name="content" rows=20 cols=50><? echo $list ?></textarea>
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
