<?php
	require_once('vpopadm.inc.php');
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>邮件账户管理菜单</TITLE>
<link rel="stylesheet" href="newstyle.css" type="text/css">
</HEAD>
<head>
<SCRIPT LANGUAGE="JavaScript">

 
function Folder(folderDescription, hreference)
{ 
  this.desc = folderDescription 
  this.hreference = hreference 
  this.id = -1   
  this.navObj = 0  
  this.iconImg = 0  
  this.nodeImg = 0  
  this.isLastNode = 0 
 
  this.isOpen = true 
  this.iconSrc = "images/ftv2folderopen.gif"   
  this.children = new Array 
  this.nChildren = 0 
 
  this.initialize = initializeFolder 
  this.setState = setStateFolder 
  this.addChild = addChild 
  this.createIndex = createEntryIndex 
  this.hide = hideFolder 
  this.display = display 
  this.renderOb = drawFolder 
  this.totalHeight = totalHeight 
  this.subEntries = folderSubEntries 
  this.outputLink = outputFolderLink 
} 
 
function setStateFolder(isOpen) 
{ 
  var subEntries 
  var totalHeight 
  var fIt = 0 
  var i=0 
 
  if (isOpen == this.isOpen) 
    return 
 
  if (browserVersion == 2)  
  { 
    totalHeight = 0 
    for (i=0; i < this.nChildren; i++) 
      totalHeight = totalHeight + this.children[i].navObj.clip.height 
      subEntries = this.subEntries() 
    if (this.isOpen) 
      totalHeight = 0 - totalHeight 
    for (fIt = this.id + subEntries + 1; fIt < nEntries; fIt++) 
      indexOfEntries[fIt].navObj.moveBy(0, totalHeight) 
  }  
  this.isOpen = isOpen 
  propagateChangesInState(this) 
} 
 
function propagateChangesInState(folder) 
{   
  var i=0 
 
  if (folder.isOpen) 
  { 
    if (folder.nodeImg) 
      if (folder.isLastNode) 
        folder.nodeImg.src = "images/ftv2mlastnode.gif" 
      else 
	  folder.nodeImg.src = "images/ftv2mnode.gif" 
    folder.iconImg.src = "images/ftv2folderopen.gif" 
    for (i=0; i<folder.nChildren; i++) 
      folder.children[i].display() 
  } 
  else 
  { 
    if (folder.nodeImg) 
      if (folder.isLastNode) 
        folder.nodeImg.src = "images/ftv2plastnode.gif" 
      else 
	  folder.nodeImg.src = "images/ftv2pnode.gif" 
    folder.iconImg.src = "images/ftv2folderclosed.gif" 
    for (i=0; i<folder.nChildren; i++) 
      folder.children[i].hide() 
  }  
} 
 
function hideFolder() 
{ 
  if (browserVersion == 1) { 
    if (this.navObj.style.display == "none") 
      return 
    this.navObj.style.display = "none" 
  } else { 
    if (this.navObj.visibility == "hiden") 
      return 
    this.navObj.visibility = "hiden" 
  } 
   
  this.setState(0) 
} 
 
function initializeFolder(level, lastNode, leftSide) 
{ 
var j=0 
var i=0 
var numberOfFolders 
var numberOfDocs 
var nc 
      
  nc = this.nChildren 
   
  this.createIndex() 
 
  var auxEv = "" 
 
  if (browserVersion > 0) 
    auxEv = "<a href='javascript:clickOnNode("+this.id+")'>" 
  else 
    auxEv = "<a>" 
 
  if (level>0) 
    if (lastNode)
    { 
      this.renderOb(leftSide + auxEv + "<img name='nodeIcon" + this.id + "' src='images/ftv2mlastnode.gif' width=16 height=22 border=0></a>") 
      leftSide = leftSide + "<img src='images/ftv2blank.gif' width=16 height=22>"  
      this.isLastNode = 1 
    } 
    else 
    { 
      this.renderOb(leftSide + auxEv + "<img name='nodeIcon" + this.id + "' src='images/ftv2mnode.gif' width=16 height=22 border=0></a>") 
      leftSide = leftSide + "<img src='images/ftv2vertline.gif' width=16 height=22>" 
      this.isLastNode = 0 
    } 
  else 
    this.renderOb("") 
   
  if (nc > 0) 
  { 
    level = level + 1 
    for (i=0 ; i < this.nChildren; i++)  
    { 
      if (i == this.nChildren-1) 
        this.children[i].initialize(level, 1, leftSide) 
      else 
        this.children[i].initialize(level, 0, leftSide) 
      } 
  } 
} 
 
function drawFolder(leftSide) 
{ 
  if (browserVersion == 2) { 
    if (!doc.yPos) 
      doc.yPos=8 
    doc.write("<layer id='folder" + this.id + "' top=" + doc.yPos + " visibility=hiden>") 
  } 
   
  doc.write("<table ") 
  if (browserVersion == 1) 
    doc.write(" id='folder" + this.id + "' style='position:block;' ") 
  doc.write(" border=0 cellspacing=0 cellpadding=0>") 
  doc.write("<tr><td>") 
  doc.write(leftSide) 
  this.outputLink() 
  doc.write("<img name='folderIcon" + this.id + "' ") 
  doc.write("src='" + this.iconSrc+"' border=0></a>") 
  doc.write("</td><td valign=middle nowrap>") 
  if (USETEXTLINKS) 
  { 
    this.outputLink() 
    doc.write(this.desc + "</a>") 
  } 
  else 
    doc.write(this.desc) 
  doc.write("</td>")  
  doc.write("</table>") 
   
  if (browserVersion == 2) { 
    doc.write("</layer>") 
  }  
  if (browserVersion == 1) { 
    this.navObj = doc.all["folder"+this.id] 
    this.iconImg = doc.all["folderIcon"+this.id] 
    this.nodeImg = doc.all["nodeIcon"+this.id] 
  } else if (browserVersion == 2) { 
    this.navObj = doc.layers["folder"+this.id] 
    this.iconImg = this.navObj.document.images["folderIcon"+this.id] 
    this.nodeImg = this.navObj.document.images["nodeIcon"+this.id] 
    doc.yPos=doc.yPos+this.navObj.clip.height 
  } 
} 
 
function outputFolderLink() 
{ 
  if (this.hreference) 
  { 
    doc.write("<a href='" + this.hreference + "' TARGET=\"xbody\" ") 
    if (browserVersion > 0) 
      doc.write("onClick='javascript:clickOnFolder("+this.id+")'") 
    doc.write(">") 
  } 
  else 
    doc.write("<a>") 
} 
 
function addChild(childNode) 
{ 
  this.children[this.nChildren] = childNode 
  this.nChildren++ 
  return childNode 
} 
 
function folderSubEntries() 
{ 
  var i = 0 
  var se = this.nChildren 
 
  for (i=0; i < this.nChildren; i++){ 
    if (this.children[i].children) //is a folder 
      se = se + this.children[i].subEntries() 
  } 
 
  return se 
} 
 
 
 
function Item(itemDescription, itemLink)
{ 
  this.desc = itemDescription 
  this.link = itemLink 
  this.id = -1
  this.navObj = 0
  this.iconImg = 0
  this.iconSrc = "images/ftv2doc.gif" 
 
  this.initialize = initializeItem 
  this.createIndex = createEntryIndex 
  this.hide = hideItem 
  this.display = display 
  this.renderOb = drawItem 
  this.totalHeight = totalHeight 
} 
 
function hideItem() 
{ 
  if (browserVersion == 1) { 
    if (this.navObj.style.display == "none") 
      return 
    this.navObj.style.display = "none" 
  } else { 
    if (this.navObj.visibility == "hiden") 
      return 
    this.navObj.visibility = "hiden" 
  }     
} 
 
function initializeItem(level, lastNode, leftSide) 
{  
  this.createIndex() 
 
  if (level>0) 
    if (lastNode)
    { 
      this.renderOb(leftSide + "<img src='images/ftv2lastnode.gif' width=16 height=22>") 
      leftSide = leftSide + "<img src='images/ftv2blank.gif' width=16 height=22>"  
    } 
    else 
    { 
      this.renderOb(leftSide + "<img src='images/ftv2node.gif' width=16 height=22>") 
      leftSide = leftSide + "<img src='images/ftv2vertline.gif' width=16 height=22>" 
    } 
  else 
    this.renderOb("")   
} 
 
function drawItem(leftSide) 
{ 
  if (browserVersion == 2) 
    doc.write("<layer id='item" + this.id + "' top=" + doc.yPos + " visibility=hiden>") 
     
  doc.write("<table ") 
  if (browserVersion == 1) 
    doc.write(" id='item" + this.id + "' style='position:block;' ") 
  doc.write(" border=0 cellspacing=0 cellpadding=0>") 
  doc.write("<tr><td>") 
  doc.write(leftSide) 
  doc.write("<a href=" + this.link + ">") 
  doc.write("<img id='itemIcon"+this.id+"' ") 
  doc.write("src='"+this.iconSrc+"' border=0>") 
  doc.write("</a>") 
  doc.write("</td><td valign=middle nowrap>") 
  if (USETEXTLINKS) 
    doc.write("<a href=" + this.link + ">" + this.desc + "</a>") 
  else 
    doc.write(this.desc) 
  doc.write("</table>") 
   
  if (browserVersion == 2) 
    doc.write("</layer>") 
 
  if (browserVersion == 1) { 
    this.navObj = doc.all["item"+this.id] 
    this.iconImg = doc.all["itemIcon"+this.id] 
  } else if (browserVersion == 2) { 
    this.navObj = doc.layers["item"+this.id] 
    this.iconImg = this.navObj.document.images["itemIcon"+this.id] 
    doc.yPos=doc.yPos+this.navObj.clip.height 
  } 
} 
 
 
function display() 
{ 
  if (browserVersion == 1) 
    this.navObj.style.display = "block" 
  else 
    this.navObj.visibility = "show" 
} 
 
function createEntryIndex() 
{ 
  this.id = nEntries 
  indexOfEntries[nEntries] = this 
  nEntries++ 
} 
 
function totalHeight()
{ 
  var h = this.navObj.clip.height 
  var i = 0 
   
  if (this.isOpen)
    for (i=0 ; i < this.nChildren; i++)  
      h = h + this.children[i].totalHeight() 
 
  return h 
} 
 
 
function clickOnFolder(folderId) 
{ 
  var clicked = indexOfEntries[folderId] 
 
  if (!clicked.isOpen) 
    clickOnNode(folderId) 
 
  return  
 
  if (clicked.isSelected) 
    return 
} 
 
function clickOnNode(folderId) 
{ 
  var clickedFolder = 0 
  var state = 0 
 
  clickedFolder = indexOfEntries[folderId] 
  
  if(clickedFolder_old.id)
  if(clickedFolder_old.id != clickedFolder.id){
  if (clickedFolder_old.isOpen) 
  clickOnNode(clickedFolder_old.id) 
  }
  
  clickedFolder_old = clickedFolder

  state = clickedFolder.isOpen 
 
  clickedFolder.setState(!state)
} 
 
function initializeDocument() 
{ 
  if (doc.all) 
    browserVersion = 1 //IE4   
  else 
    if (doc.layers) 
      browserVersion = 2 //NS4 
    else 
      browserVersion = 0 //other 
 
  fT.initialize(0, 1, "") 
  fT.display()
  
  if (browserVersion > 0) 
  { 
    doc.write("<layer top="+indexOfEntries[nEntries-1].navObj.top+">&nbsp;</layer>") 
 
    clickOnNode(0) 
    clickOnNode(0) 
  } 
} 
 
 
function gFld(description, hreference) 
{ 
  folder = new Folder(description, hreference) 
  return folder 
} 
 
function gLnk(target, description, linkData) 
{ 
  fullLink = "" 
 
  if (target==0) 
  { 
  	if(description=="退出系统") fullLink = "'"+linkData+"' target=\"_top\""
  	fullLink = "'"+linkData+"' target=\"xbody\"" 
  } 
  else 
  { 
    if (target==1) 
       fullLink = "'"+linkData+"' target=xbody" 
    else 
       fullLink = "'http://"+linkData+"' target=\"xbody\"" 
  } 
 
  linkItem = new Item(description, fullLink)   
  return linkItem 
} 
 
function insFld(parentFolder, childFolder) 
{ 
  return parentFolder.addChild(childFolder) 
} 
 
function insDoc(parentFolder, document) 
{ 
  parentFolder.addChild(document) 
} 

function outMenu(module,url) 
{ 
  insDoc(aux1, gLnk(0,module,url))
} 
 
USETEXTLINKS = 0 
indexOfEntries = new Array 
clickedFolder_old = 0
nEntries = 0 
doc = document 
browserVersion = 0 
selectedFolder=0
</script>

<script>

USETEXTLINKS = 1

  

fT= gFld("<b>邮件系统管理</b>", "");
<?php 
	if (adminPerm(PERM_ADMIN_USERCONTROL) ){
?>
	insDoc(fT, gLnk(0,"用户列表","http://<?php echo $_SERVER['SERVER_NAME']; ?>/mailadm/showuserlist.php"));
<?php
}
?>
<?php 
	if (adminPerm(PERM_ADMIN_USERCONTROL) ){
?>
	insDoc(fT, gLnk(0,"添加新用户","adduser.php"));
<?php
}
?>
<?php 
	if (adminPerm(PERM_ADMIN_USERCONTROL) ){
?>
	insDoc(fT, gLnk(0,"查找用户","searchUser.php"));
<?php
}
?>
<?php 
	if (adminPerm(PERM_ADMIN_USERCONTROL) ){
?>
	insDoc(fT, gLnk(0,"用户组管理","groupsControl.php"));
<?php
}
?>
<?php 
	if (adminPerm(PERM_ADMIN_USERCONTROL) ){
?>
	insDoc(fT, gLnk(0,"群发信件","sendGroupMails.php"));
<?php
}
?>
<?php
        if (adminPerm(PERM_ADMIN_ADMINCONTROL) ){
?>
        insDoc(fT, gLnk(0,"管理员账号列表","http://<?php echo $_SERVER['SERVER_NAME']; ?>/mailadm/showadminlist.php"));
<?php
}
?>
<?php 
	if (adminPerm(PERM_ADMIN_ADMINCONTROL) ){
?>
	insDoc(fT, gLnk(0,"添加管理员账号","addAdmin.php"));
  aux1 = insFld(fT, gFld("反垃圾邮件",""));
		insDoc(aux1,gLnk(0,"SMTP转发设置","controlSmtproute.php"));
		insDoc(aux1,gLnk(0,"AS-SMTP设置","controlMFCheck.php"));
		insDoc(aux1,gLnk(0,"IP黑名单","controlBlackIP.php"));
		insDoc(aux1,gLnk(0,"IP白名单","controlWhiteIP.php"));
		insDoc(aux1,gLnk(0,"域名黑名单","controlBadDomain.php"));
		insDoc(aux1,gLnk(0,"域名白名单","controlGoodDomain.php"));
		insDoc(aux1,gLnk(0,"地址黑名单","controlBadAccount.php"));
		insDoc(aux1,gLnk(0,"Open Relay Deny","controlOpenRelay.php"));
		insDoc(aux1,gLnk(0,"RSS功能","controlRSS.php"));
<?php
}
?>
<?php 
	if (adminPerm(PERM_ADMIN_BASIC) ){
?>	
	insDoc(fT, gLnk(0,"修改密码","changePasswd.php"));
<?php
}
?>
<?php 
	if (adminPerm(PERM_ADMIN_BASIC) ){
?>	
	insDoc(fT, gLnk(2,"<a href=logout.php target=_top>退出系统"));
<?php
}
?>

</script>

<body background="images/bj_01.gif">
<script>
initializeDocument();
</script>
</BODY>
</HTML>
