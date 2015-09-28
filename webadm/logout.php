<?php

require_once('vpopadm.inc.php');

unset($_SESSION['AdminID']);
unset($_SESSION['Privilidge']);

session_unset();
session_destroy();

?>
<html>
<head>
<META content="text/html; charset=gb2312" http-equiv=Content-Type>
</head>
<body>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<div align="center">
已成功退出。请关闭此窗口
</div>
</body>
</html>
