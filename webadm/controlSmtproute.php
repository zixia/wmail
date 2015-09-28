<?
require_once("vpopadm.inc.php");
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>smtp转发设置</TITLE>
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
	$handle=fopen("/var/qmail/control/smtproutes","w");	
	if (!$handle) {
		echo "错误，无法保存设置#1！<br>";
		return false;
	}
	$content = str_replace("\r","",$_POST['content']);
	fputs($handle, $content);
	fclose($handle);

	$lines = split ( "\n", $content );

	$route_domains = array();
	foreach ( $lines as $line ){
		list($domain, $ip, $port) = explode(":", $line);
		//echo "$domain:$ip:$port<br>";
		if ( strlen($domain) ){
			array_push( $route_domains, $domain );
		}
	}

	// modify rcpthosts
	if ( ! file_exists("/var/qmail/control/rcpthosts") ){
		fclose( fopen("/var/qmail/control/rcpthosts","w") );
	}
	
	$handle=fopen("/var/qmail/control/rcpthosts","r");	
	if (!$handle) {
		echo "错误，无法保存设置#2！<br>";
		return false;
	}

	//FIXME: only can add new domain, but never delete old domains.
	$rcpt_domains = array();
	while ( !feof($handle) ){
		$domain = fgets ( $handle );
		$domain = rtrim($domain);
		if ( strlen($domain) ){
			array_push( $rcpt_domains, $domain );
		}
	}
	fclose($handle);

	$add_route_domains = array_diff ( $route_domains, $rcpt_domains) ;

	foreach ( $add_route_domains as $add_domain ){
		array_push ( $rcpt_domains, $add_domain );
	}

	$rcpthost_content = "";
	foreach ( $rcpt_domains as $domain ){
		$rcpthost_content .= $domain ;
		$rcpthost_content .= "\n" ;
	}

	$handle=fopen("/var/qmail/control/rcpthosts","w");	
	if (!$handle) {
		echo "错误，无法保存设置#2！<br>";
		return false;
	}
	fputs($handle,$rcpthost_content);
	fclose($handle);

	return true;
}

if ( (isset($_REQUEST['doConfig']) && doConfig()) ){
	echo "设置保存成功！<br>";
} else {
	if ( ! file_exists("/var/qmail/control/smtproutes") ){
		fclose( fopen("/var/qmail/control/smtproutes","w") );
	}
	$handle = fopen ("/var/qmail/control/smtproutes", "r");

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
		<td colspan="2" class=title><b>设置SMTP转发列表</b></td>
		</tr>
		<tr>
		<td colspan="2">SMTP转发列表：
		</td>
		</tr>
		<tr>
		<td colspan="2"><textarea name="content" rows=20 cols=20><? echo $list ?></textarea>
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
