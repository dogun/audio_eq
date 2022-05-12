<?php
include_once('config.php');

$pfile = @$_REQUEST['wave'];
$hw = @$_REQUEST['hw'];
$eq = @$_REQUEST['eq'];
$duration = @$_REQUEST['duration'];
$rfile = @$_REQUEST['w_name'];

$action = @$_REQUEST['action'];

$pwave = $HOME.$pfile;
$rwave = $HOME.$rfile;

$play_cmd = 'cat';
if (strstr($pwave, 'wav')) $play_cmd = $HOME.'/bin/wave';
$play_cmd = "$play_cmd $pwave | $HOME/bin/play $hw";
if ($eq)
	$play_cmd = "$play_cmd $pwave | $HOME/bin/eq config/$eq 80 | $HOME/bin/play $hw";

$record_cmd = "$HOME/bin/record $hw $duration > $rwave";

if ($action == 'play') {
	$cmd = $play_cmd;
} elseif ($action == 'measure') {
	$cmd = "$record_cmd & sleep 2; $play_cmd";
} elseif ($action == 'record') {
	$cmd = $record_cmd;
}

ob_start();
passthru($cmd);
$ret = ob_get_clean();

echo $cmd;
echo '<br />';
echo $ret;
echo "OK";
