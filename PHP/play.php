<?php

$outputFile = fopen("output.txt", "w");
file_put_contents('ls2.txt', 'WTF');

$postData = file_get_contents('php://input');
file_put_contents('post.txt', $postData);

//$last_line = system('/usr/bin/sudo /usr/bin/omxplayer /opt/vc/src/hello_pi/hello_video/test.h264 -o hdmi', $retval);

file_put_contents('ls.txt', $last_line);


$stdInFilePath = '/home/pi/projects/RaspberryPiPlayer/bin/ARM/Debug/thefifofile'; //'/proc/' . $pidTxt . '/fd/1';

fwrite($outputFile, $stdInFilePath . "\n");

$pStdInFile = fopen($stdInFilePath, 'w');

if ($pStdInFile)
{
	fwrite($pStdInFile, "p " . $postData);
	fwrite($outputFile, "Writing to the std in\n"); 
	fclose($pStdInFile);
}

//system('sudo reboot');

?>