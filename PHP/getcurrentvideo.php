<?php

$STRNoActiveVideo = 'Nothing.';

$stdInFilePath = '/home/pi/projects/RaspberryPiPlayer/bin/ARM/Debug/currentvideo.txt'; //'/proc/' . $pidTxt . '/fd/1';

$STRFile = file_get_contents($stdInFilePath);

if ($STRFile)
{
	echo $STRFile;
}
else
{
	echo $STRNoActiveVideo;
}

?>