#pragma once

const char* k_szHeader = R"foo(
<head>
<link rel="stylesheet" type="text/css" href="mystyle.css">
<script src="jquery-3.2.1.js"></script>
</head>
<div>
<div style="width:100%; background-color: lightblue; height:90px; margin-bottom:8px;" >
        <h1 style="text-align:center; position:absolute; width:100%;">Raspberry Pi Player</h1>
</div>

    <div style="width:700px; margin-left:auto; margin-right:auto; border:1px solid #d6d9dc; padding: 12px;">
<h2>Select Videos:</h2>
<div style="width:100%; margin-bottom:8px;">
<div style="text-align:center; width:100%;">

<button onclick="sendcommand(this, 'P')" style=" display: inline-block; width: 110px; height: 122px; background: url(pause.png) 0 0;   background-size: 110px 122px; "></button>
<button onclick="sendcommand(this, 'n')" style=" display: inline-block; width: 110px; height: 122px; background: url(skip.png) 0 0;    background-size: 110px 122px; "></button>
<button onclick="sendcommand(this, 'q')" style=" display: inline-block; width: 110px; height: 122px; background: url(stop.png) 0 0;    background-size: 110px 122px; "></button>
<button onclick="sendcommand(this, '-')" style=" display: inline-block; width: 110px; height: 122px; background: url(voldown.png) 0 0; background-size: 110px 122px; "></button>
<button onclick="sendcommand(this, '=')" style=" display: inline-block; width: 110px; height: 122px; background: url(volup.png) 0 0;   background-size: 110px 122px; "></button>

<!--<button style=" display: inline-block;" type="button" id="playbtn" class="playButtonClass" >Play Selection</button>-->
</div>
<div style="text-align:center; width:100%; margin: 10,10,10,10;">
<button type="button" id="playbtn" class="playButtonClass" style=" display: inline-block;">Play Selection</button>
<p " >Currently Playing:</p>
<p id="CurrentlyPlayingID" >Currently Playing...</p>

</div>
</div>


<script>
function sendcommand(event, param)
{
$.post( "sendcommand.php?cmd=" + param, null, function(result){  });
}
</script>

<script> 
$(document).ready(function(){
$( "#playbtn" ).click(function() {
var myPostData = GatherAllTicked();
$( "#playbtn" ).prop('disabled', true);
$.post( "play.php", JSON.stringify(myPostData), function(result){ $( "#playbtn" ).prop('disabled', false); });

});
});
</script>

)foo";

const char* k_szFooter = R"foo(
      </div>
</div>
		<script>function dropdownfunction(event) 
{ 
var x = event.parentElement.getElementsByTagName("UL"); 
if (x.length > 0) { console.log(x[0].style.display);											   	
if (x[0].style.display == "none")
{	
x[0].style.display = "";
event.src = "right.png";
}
else				
{							   		
x[0].style.display = "none"; 
event.src = "down.png";
}} 
}</script>
	<script src="jscode.js"></script>		
<script src="periodicupdate.js"></script>		
)foo";
