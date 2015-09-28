<?php
require_once("vpopadm.inc.php");
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>群发信件</TITLE>
</HEAD>
<BODY>
<DIV align="center">
<h2>群发信件</h2>
<?php

if (!adminPerm(PERM_ADMIN_USERCONTROL) ){
?>
<br>
您没有访问该网页的权限。<br>
<?php
} else {
if ($_POST['action']=='send') {
	sendGroup($_POST['groups'],$_POST['sendAll']=='true', $_POST['mailbox'], $_POST['title'],$_POST['content']);
} else {
	showMenu();
}
}



?>
</div>
</BODY>
</HTML>
<?php

function get_mimetype($name)
{
	$dot = strrchr($name, '.');
	if ($dot == $name)
		return "text/plain; charset=gb2312";
	if (strcasecmp($dot, ".html") == 0 || strcasecmp($dot, ".htm") == 0)
		return "text/html; charset=gb2312";
	if (strcasecmp($dot, ".jpg") == 0 || strcasecmp($dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcasecmp($dot, ".gif") == 0)
		return "image/gif";
	if (strcasecmp($dot, ".png") == 0)
		return "image/png";
	if (strcasecmp($dot, ".pcx") == 0)
		return "image/pcx";
	if (strcasecmp($dot, ".css") == 0)
		return "text/css";
	if (strcasecmp($dot, ".au") == 0)
		return "audio/basic";
	if (strcasecmp($dot, ".wav") == 0)
		return "audio/wav";
	if (strcasecmp($dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcasecmp($dot, ".mov") == 0 || strcasecmp($dot, ".qt") == 0)
		return "video/quicktime";
	if (strcasecmp($dot, ".mpeg") == 0 || strcasecmp($dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcasecmp($dot, ".vrml") == 0 || strcasecmp($dot, ".wrl") == 0)
		return "model/vrml";
	if (strcasecmp($dot, ".midi") == 0 || strcasecmp($dot, ".mid") == 0)
		return "audio/midi";
	if (strcasecmp($dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcasecmp($dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";
	if (strcasecmp($dot, ".txt") == 0)
		return "text/plain; charset=gb2312";
	if (strcasecmp($dot, ".xht") == 0 || strcasecmp($dot, ".xhtml") == 0)
		return "application/xhtml+xml";
	if (strcasecmp($dot, ".xml") == 0)
		return "text/xml";
	return "application/octet-stream";
}

function sendGroup($sendGroups, $isSendAll, $mailbox,$title, $content) {
	$passwd_file = VPOPMAILHOME . 'domains/' . DOMAIN . '/vpasswd';

	$user_profile = VPOPMAILHOME . 'domains/' . DOMAIN . '/' . USERPROFILE;

    $h_user_profile = fopen ($user_profile,"a+");
   
    if ($h_user_profile == NULL ){
        echo "错误：用户数据文件无法打开。<br>";
		exit(-1);
    }

	if (!$isSendAll){
   
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
											"group"=> trim($group))
			);
		}
		flock($h_user_profile, LOCK_UN);
		fclose($h_user_profile);
		$sendgrouplist=explode(',', $sendGroups);
	}
    

	$user_list = file( $passwd_file );
    $mail_count=count($user_list);

define('MAIL_MIME_CRLF',"\n",TRUE);

require 'Mail.php';
require 'Mail/mime.php';

$headers['From'] = $mailbox;
$headers['To'] = 'alluser@'.DOMAIN;
$headers['Subject'] = $title;

$mime = new Mail_mime;

$mime->setTXTBody(str_replace("\r","",$content));
	
$attachdir="/tmp/wmail/".$_SESSION['AdminID'];
@mkdir("/tmp/wmail");
@mkdir($attachdir);

$fp1=@fopen($attachdir . "/.index","r");
if ($fp1!=FALSE) {
	while (!feof($fp1)) {
		$buf=fgets($fp1);
		$file=substr($buf,0,strpos($buf,' '));
		if ($file=="")
			continue;
		$name=strstr($buf,' ');
		$name=substr($name,1);		
		@unlink($attachdir . "/" . $file);
		$mime->addAttachment($file, get_mimetype($file),$name);
	}
	fclose($fp1);
	@unlink($attachdir . "/.index");
}
$fp2=fopen($attachdir . "/.mail","w");
if ($fp2==FALSE) {
	echo "无法建立信件！";
	exit(0);
}
// get MIME formatted message headers and body
$body = $mime->get(array('text_charset'=>'GB2312'));
$header = $mime->headers($headers);
fwrite($fp2,"Return-Path: <".$header['From'].">\n");
fwrite($fp2,"Date: ".date("r")."\n");

foreach ($header as $name => $value) {
	fwrite($fp2, $name.': '.$value."\n");
}
fwrite($fp2, "\n");
fwrite($fp2, $body);

fclose($fp2);

$mail_name=time().".12345.".DOMAIN;
$iscopy=true;
$linkpath='';
	for( $i = 0 ; $i < $mail_count ; $i++)	{
			list( $user_account, $xxx, $xxx, $xxx, $user_name, $user_maildir, $user_quota )  = explode( ':', $user_list[$i] );
			$user_maildir= $user_maildir. "/Maildir/new/";
			$user_mailname= $user_maildir.$mail_name; 
			if ($isSendAll) {	
				echo "发送给".$user_account;
				if ($iscopy) {
					@copy($attachdir . "/.mail", $user_mailname);
					$linkpath=$user_mailname;
					$iscopy=false;
				} else {
					@link($linkpath, $user_mailname);
				}
				continue;
			}
			for ($t=0; $t<count($userinfo_list); $t++){
				if (!strcmp($user_account, $userinfo_list[$t]['id'])) {
					break;
				} 
			}
			if ($t<count($userinfo_list)){
				$groups=explode(',',trim($userinfo_list[$t]['group']));
				foreach ($sendgrouplist as $sendgroup) {
					if (in_array($sendgroup,$groups)) {
						if ($iscopy) {
							@copy($attachdir . "/.mail", $user_mailname);
							$linkpath=$user_mailname;
							$iscopy=false;
						} else {
							@link($linkpath, $user_mailname);
						}
						break;
					}
				}
			}
 
	}
@unlink($attachdir . "/.mail");
	echo "发送成功！";
	return false;
}

function showMenu(){
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
请指定待发送的用户组,若不选择则信件将发给全部用户：
<select id="oGroupList" size=10 multiple>
<?php
	for ($i=0;$i<$group_count;$i++){
?>
	<option ><?php echo $group_list[$i] ?></option>
<?php
	}
?>
</select>
<?php
	}
?>
<hr width="97%">
<script language="JavaScript">
	function doSend(){
        var dot=false;
		if (oForm.oContent.value=='') {
			alert('群发信件内容为空!');
			return false;
		}
		if (oForm.oTitle.value=='') {
			alert('群发信件标题为空!');
			return false;
		}
		if (oForm.oMailbox.value=='') {
			alert('管理员地址为空!');
			return false;
		}
		if (typeof(oGroupList) != "undefined") {
			for (i=0;i<oGroupList.length;i++){
			if (oGroupList.options(i).selected) {
				if (dot) {
					oForm.oGroups.value+=',';
				} else {
					oForm.oSendAll.value="false";
					dot=true;
				}
				oForm.oGroups.value+=oGroupList.options(i).text;
			}
			}
		}
		return oForm.submit();
	}
		
</script>
<form action="<?php echo $_SERVER['PHP_SELF'] ; ?>" method="POST" id="oForm">
<input type="hidden" name="groups" id="oGroups" >
<input type="hidden" name="action" value="send">
<input type="hidden" name="sendAll" id="oSendAll" value="true">
<table>
<tr>
<td>管理员邮箱</td><td><input type="text" name="mailbox" id="oMailbox"></td>
</tr>
<tr>
<td>群发邮件标题</td><td><input type="text" name="title" id="oTitle"></td>
</tr>
<tr>
<td>群发信件内容</td><td><textarea name="content" id="oContent" style="width:250; height:400"></textarea></td>
</tr>
</table>
<input type="button" value="发送群体信件" onclick="doSend();">
<script language="JavaScript">
<!--
   function GoAttachWindow(){     
	
   	var hWnd = window.open("uploadAttach.php","_blank","width=600,height=300,scrollbars=yes");  

	if ((document.window != null) && (!hWnd.opener))  

		   hWnd.opener = document.window;  

	hWnd.focus();  

   	return false;  

   }  
-->
</script>
<input type="button" value="添加附件" onclick="GoAttachWindow()");
</form>
<?php
}
?>
