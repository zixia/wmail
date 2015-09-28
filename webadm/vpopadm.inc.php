<?php
session_start(); 

function getDomain(){
	return "localhost";
	return substr($_SERVER['SERVER_NAME'],5);
}

define( 'DOMAIN', getDomain() );
define( 'VPOPMAILHOME', '/home/vpopmail/' );
#system( VPOPMAILHOME . "bin/chmod 644 " .VPOPMAILHOME . "domains/" . DOMAIN . "/vpasswd" );
define( "USERPROFILE", "USERDATA");
define("GROUPFILE", "GROUPS");

$DEFAULTMAILBOXSIZE=50;

function siteFile($filename){
	return  'domains/' . DOMAIN . '/' . $filename;
	
}

function errorReturn($msg,$url){ //提示错误信息并返回$url页面
?>
<script>
	alert("<?php echo $msg; ?>");
	document.location.href="<?php echo $url; ?>";
</script>
<?php
	exit();
}


require_once("admin.inc.php");
?>
