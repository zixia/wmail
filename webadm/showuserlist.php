<?php
require_once('vpopadm.inc.php');

	if (!adminPerm(PERM_ADMIN_USERCONTROL) ){
?>
		<br>
		您没有访问该网页的权限。<br>
<?php
		return false;
	}
?>
<HTML>
<HEAD>
<meta http-equiv="content-type" content="text/html; charset=gb2312">
<TITLE>显示用户列表</TITLE>
<style>
.userAccount {color: #0000FF;text-decoration: underline; font-size: x-small;}
.userAccount_on {color: #0000FF;text-decoration: none; font-size: x-small;}
TD {font-size: x-small;}
BODY {font-size: x-small;}
span {font-size: x-small;}
</style>
</HEAD>
<BODY onclick="hideFilterMenu();">
<OBJECT id="userlist" CLASSID="clsid:333C7BC4-460F-11D0-BC04-0080C7055A83" VIEWASTEXT>
	<PARAM NAME="DataURL" VALUE="userlist.php">
	<PARAM NAME="UseHeader" VALUE="True">
	<PARAM NAME="CHARSET" VALUE="GB2312">
</OBJECT>
<SCRIPT language="JScript">
var filterCol="";


function sortOrder(cols) {  
	var s=new String(userlist.sort);
	var a=s.split(";");
	var i;
	for (i=0;i<a.length;i++){
		if (a[i]==("+"+cols)){
			return 1;
		}
		if (a[i]==("-"+cols)){
			return -1;
		}
	}
	return 0;
}


function clearSortOrder(){
	oSortOn_account.innerHTML="";
	oSortOn_name.innerHTML="";
	oSortOn_unit.innerHTML="";
	oSortOn_department.innerHTML="";
	oSortOn_station.innerHTML="";
	oSortOn_idCode.innerHTML="";
	oSortOn_createTime.innerHTML="";
	oSortOn_quota.innerHTML="";
	oSortOn_isPublic.innerHTML="";
	oSortOn_note.innerHTML="";
	oSortOn_groups.innerHTML="";
}


function showSortOrder(cols){
	var s=sortOrder(cols);
	if (s==1) {
		document.all.item("oSortOn_"+cols).innerHTML="↓";
		return ;
	}
	if (s==-1) {
		document.all.item("oSortOn_"+cols).innerHTML="↑";
		return ;
	}
	document.all.item("oSortOn_"+cols).innerHTML="";
}

function sort(cols){
	var s=sortOrder(cols); 
	var temp=new String(userlist.sort);
	var re;
	if ((window.event!=null) && (window.event.ctrlKey) ){  //按下了ctrl键
		if (s==0) { //原有sortlist里没有该字段
			userlist.sort+="+"+cols+";";
		} else { //原有sortlist里有该字段
			if (s==-1) { 
				re=new RegExp("[-]"+cols+";","i");
				userlist.Sort=temp.replace(re,"");
			} else {
				re=new RegExp("[+]"+cols+";","i");
				userlist.Sort=temp.replace(re,"");
			}
		}

	} else { //未按下ctrl键
		if (s==0) { //原有sortlist里没有该字段
			clearSortOrder();
			userlist.sort="+"+cols+";";
		} else { //原有sortlist里有该字段
			if (s==-1) { 
				re=new RegExp("[-]"+cols,"i");
				userlist.Sort=temp.replace(re,"+"+cols);
			} else {
				re=new RegExp("[+]"+cols,"i");
				userlist.Sort=temp.replace(re,"-"+cols);
			}
		}
	}
	showSortOrder(cols);
	userlist.reset();
}

function filter(cols){
	filterCol=cols;
	
    oFilterMenu.style.display="block";
	oFilterMenu.style.top=window.event.y;
	oFilterMenu.style.left=window.event.x;
	
	window.event.returnValue=false;
}

function hideFilterMenu(){
    oFilterMenu.style.display="none";
}

function chooseUser(){
	document.location.href = "userInfo.php?id=" + event.srcElement.innerText;
}

function doFilter(value){
	if (value=="") {
		userlist.filter=filterCol+" = \"\"";
	} else {
		userlist.filter=filterCol+" = *"+value+"*";
	}
	hideFilterMenu();
	userlist.Reset();
}

function clearFilter() {
	userlist.filter="";
	hideFilterMenu();
	userlist.Reset();
}

</SCRIPT>
<DIV ID="oFilterMenu" style="position:absolute;background-color:#ffffff;border-width:2;border-color:#000000;border-style:solid;padding: 2px; font-size:x-small;display:none" onclick="window.event.cancelBubble = true;">
			<form>
			<table border="0" style="font-size:x-small">
			<thead>
			<th align="center">字段过滤</th>
			</thead>
			<tr>
			<td nowrap>
			本字段应包含：<input type="text" id="oFilterValue" name="filterValue">
			</td>
			</tr>
			<tr>
			<td>
			<table border="0">
			<tr>
			<td>
			<input type="button" onclick="doFilter(oFilterValue.value);" value="应用过滤条件">
			</td>
			<td>
			<input type="button" onclick="clearFilter();" value="清除现有过滤条件">
			</td>
			</tr>
			</table>
			</td>
			</tr>
			</table>
			</form>
</DIV>

<DIV align="center">
<table border="1" datasrc="#userlist" id="oUserInfo" width="99%">
<thead>
<TH><span onclick="return sort('account');" oncontextmenu="filter('account');" class="userAccount" onmouseover="this.className='userAccount_on';" onmouseout="this.className='userAccount';">账号</span><span id="oSortOn_account"></span></th>
<TH><span onclick="return sort('name');"  oncontextmenu="filter('name');" class="userAccount" onmouseover="this.className='userAccount_on';" onmouseout="this.className='userAccount';">用户姓名</span><span id="oSortOn_name"></span></th>
<TH><span onclick="return sort('unit');"  oncontextmenu="filter('unit');" class="userAccount" onmouseover="this.className='userAccount_on';" onmouseout="this.className='userAccount';">用户单位</span><span id="oSortOn_unit"></span></th>
<TH><span onclick="return sort('department');"  oncontextmenu="filter('department');" class="userAccount" onmouseover="this.className='userAccount_on';" onmouseout="this.className='userAccount';">用户部门</span><span id="oSortOn_department"></span></th>
<TH><span onclick="return sort('station');"  oncontextmenu="filter('station');" class="userAccount" onmouseover="this.className='userAccount_on';" onmouseout="this.className='userAccount';">用户岗位</span><span id="oSortOn_station"></span></th>
<TH><span onclick="return sort('idCode');"  oncontextmenu="filter('idCode');" class="userAccount" onmouseover="this.className='userAccount_on';" onmouseout="this.className='userAccount';">用户身份号码</span><span id="oSortOn_idCode"></span></th>
<TH><span onclick="return sort('createTime');"  oncontextmenu="filter('createTime');" class="userAccount" onmouseover="this.className='userAccount_on';" onmouseout="this.className='userAccount';">账号建立时间</span><span id="oSortOn_createTime"></span></th>
<TH><span onclick="return sort('quota');"  oncontextmenu="filter('quota');" class="userAccount" onmouseover="this.className='userAccount_on';" onmouseout="this.className='userAccount';">容量限制</span><span id="oSortOn_quota"></span></th>
<TH><span onclick="return sort('isPublic');"  oncontextmenu="filter('isPublic');" class="userAccount" onmouseover="this.className='userAccount_on';" onmouseout="this.className='userAccount';">对外显示</span><span id="oSortOn_isPublic" ></span></th>
<TH><span onclick="return sort('note');"  oncontextmenu="filter('note');" class="userAccount" onmouseover="this.className='userAccount_on';" onmouseout="this.className='userAccount';">备注</span><span id="oSortOn_note"></span></th>
<TH><span onclick="return sort('groups');"  oncontextmenu="filter('groups');" class="userAccount" onmouseover="this.className='userAccount_on';" onmouseout="this.className='userAccount';">用户组<span><span id="oSortOn_groups"></span></th>
</thead>
<tbody>
<td onclick="return chooseUser()" class="userAccount" onmouseover="this.className='userAccount_on';" onmouseout="this.className='userAccount';"><span datafld="account"></span></td>
<td><span datafld="name"></span></td>
<td><span datafld="unit"></span></td>
<td><span datafld="department"></span></td>
<td><span datafld="station"></span></td>
<td><span datafld="idCode"></span></td>
<td><span datafld="createTime"></span></td>
<td><span datafld="quota"></span> MB</td>
<td><span datafld="isPublic"></span></td>
<td><span datafld="note"></span></td>
<td><span datafld="groups"></span></td>
</tbody>
</table>
</div>
<SCRIPT language="JScript">
sort('account');
</script>
</BODY>
</HTML>
