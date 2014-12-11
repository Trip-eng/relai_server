<!DOCTYPE HTML>
<?php
$headline = "PI Relai";
$relais = array(
    0 => "Relai 1",
    1 => "Relai 2",
    2 => "Relai 3",
    3 => "Relai 4",
    4 => "Relai 5",
    5 => "Relai 6",
    6 => "Relai 7",
    7 => "Relai 8",
);
?>
<html>
<head>
<title><?php echo $headline ?></title>

<meta http-equiv="content-type" content="text/html; charset=utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">

<!--
<link rel="stylesheet" href="css/bootstrap.min.css">
-->

<link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.1/css/bootstrap.min.css">

<script type="text/javascript">

var localTest = false;
var defaultServer = "raspberrypi";
var ws;

window.onbeforeunload = onWindowColse;

function connect(server)
{
	if(server != null && server != "")
		defaultServer = server;
	
	if(ws != null)
	{
		ws.close();
	}
	ws = new WebSocket("ws://" + defaultServer +":45600");
	
	ws.onopen = onConnect;
	ws.onmessage = onReseve;
	ws.onclose = onDisonnect;

	ws.onerror = function(evt) { }
	
	if(ws.readyState == 0)
	{
		setAlert("Connecting ... ", "info");
		disablAll();
	}
}

function onWindowColse()
{
	ws.close();
}

function onConnect()
{
	resetAlert();
	//setAlert("Connected", "success", true);
	//window.setTimeout(resetAlert, 5000);
}

function sendMessage(relai, state)
{
    var frame = '{ "relai":"'+relai+'","state":"'+state+'" }';
    //console.log("send:" + frame);
    ws.send(frame);
    
}

function onReseve(evt)
{
    var received_msg = evt.data;
    var jsonmsg = JSON.parse(received_msg);
    setButState(jsonmsg.relai,jsonmsg.state);
}

function onDisonnect()
{
	setAlert("<strong>Not Conectet !<strong> <a href=\"javascript:connect()\" class=\"alert-link\">try to connect<a>","danger");
	disablAll();
}

function setAlert(message,type,closeable)
{
    var htmlText = "";
	htmlText += "<div class=\"alert alert-" + type + " alert-dismissible \" role=\"alert\">";
	if(closeable)
		htmlText += "<button type=\"button\" class=\"close\" data-dismiss=\"alert\" onclick=\"resetAlert();\" >&times</button>";
	htmlText += message;
	htmlText += "</div>";
	document.getElementById("alert").innerHTML = htmlText;
	
}
function resetAlert()
{
	document.getElementById("alert").innerHTML ="";
}

function setButState(butNumber, state)
{
	console.log("but" + butNumber);
	var but = document.getElementById("but" + butNumber);
	console.log(but);
	if(state == "on")
	{
		but.state = "on";
		but.disabled = false;
		but.className = "btn btn-success";
	} 
	else if(state == "off")
	{
		but.state = "off";
		but.disabled = false;
		but.className = "btn btn-danger";
	}
	else 
	{
		but.state = "disabled";
		but.disabled = true;
		but.className = "btn btn-default";
	}
}

function onButClick(butNumber)
{
	var but = document.getElementById("but" + butNumber);
	if(but.state == "on")
	{
		sendMessage(butNumber,"off");
	}
	else if(but.state == "off")
	{
		sendMessage(butNumber,"on");
	}
}

function disablAll()
{
	for (var i = 0; i < 8; i++) {
		setButState(i);
	};
}

</script>
</head>
<body>
<center>
<div id="page" style="max-width: 600px; text-align:center;">
<div id="pageheader" style="text-align:center;">
	<h1><?php echo $headline ?></h1><br>
</div>
<div id="pagebody" style="text-align:left;">
<div id="alert"></div>
<div id="main" style="max-width:100%; width: 600px; text-align:center;" >
<div class="btn-group-vertical btn-group-lg" role="group">

<?php
foreach ($relais as $i => $value) { 
	echo '<button id="but' . $i . '" type="button" class="btn btn-default" onclick="onButClick(' . $i . ');" disabled >' . $value . ' </button>';
}
?>
</div>
</div>
</div>
</div>
</center>
<script type="text/javascript">
if(localTest)
	onConnect();
else
	connect();
</script>
</body>
</html>
