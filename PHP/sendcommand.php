<?php

$paramAdded = $_GET['cmd'];

$stdInFilePath = '/home/pi/projects/RaspberryPiPlayer/bin/ARM/Debug/thefifofile'; //'/proc/' . $pidTxt . '/fd/1';

file_put_contents('cmd.txt', $paramAdded);

$pStdInFile = fopen($stdInFilePath, 'w');

if ($pStdInFile && $paramAdded)
{
	fwrite($pStdInFile, $paramAdded);
	fwrite($outputFile, "Writing to the std in\n"); 
	fclose($pStdInFile);
}


?>