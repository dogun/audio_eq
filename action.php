<?php
include_once('config.php');

// Strip path components to prevent directory traversal
$pfile = basename((string)@$_REQUEST['wave']);
$hw = (string)@$_REQUEST['hw'];
$eq = basename((string)@$_REQUEST['eq']);
$duration = (int)@$_REQUEST['duration'];
$rfile = basename((string)@$_REQUEST['w_name']);

$action = (string)@$_REQUEST['action'];

// Validate hw: only allow hw:N or hw:N,N format
if (!preg_match('/^hw:[0-9]+(,[0-9]+)?$/', $hw)) {
	die('invalid hw parameter');
}

// Validate duration: must be a positive integer
if ($duration <= 0 || $duration > 3600) {
	die('invalid duration');
}

// Validate rfile: only allow safe filename characters
if ($rfile === '' || !preg_match('/^[a-zA-Z0-9._-]+$/', $rfile)) {
	die('invalid filename');
}

// Validate pfile exists within the wav directory
$pwave = $HOME.'/wav/'.$pfile;
if ($pfile && !file_exists($pwave)) {
	die('wave file not found');
}

$rwave = $HOME.'/wav/'.$rfile;

// Validate eq config exists within the config directory
if ($eq && !file_exists($HOME.'/config/'.$eq.'_l.conf')) {
	die('eq config not found');
}

$bin_wave   = escapeshellarg($HOME.'/bin/wave');
$bin_play   = escapeshellarg($HOME.'/bin/play');
$bin_eq     = escapeshellarg($HOME.'/bin/eq');
$bin_record = escapeshellarg($HOME.'/bin/record');

$src_cmd = pathinfo($pfile, PATHINFO_EXTENSION) === 'wav' ? "$bin_wave ".escapeshellarg($pwave) : "cat ".escapeshellarg($pwave);

if ($eq) {
	$play_cmd = "$src_cmd | $bin_eq ".escapeshellarg('config/'.$eq)." 80 | $bin_play ".escapeshellarg($hw);
} else {
	$play_cmd = "$src_cmd | $bin_play ".escapeshellarg($hw);
}

$record_cmd = "$bin_record ".escapeshellarg($hw)." ".escapeshellarg((string)$duration)." > ".escapeshellarg($rwave);

if ($action == 'play') {
	$cmd = $play_cmd;
} elseif ($action == 'measure') {
	$cmd = "$record_cmd & sleep 2; $play_cmd";
} elseif ($action == 'record') {
	$cmd = $record_cmd;
} else {
	die('invalid action');
}

ob_start();
passthru($cmd);
$ret = ob_get_clean();

echo htmlspecialchars($cmd);
echo '<br />';
echo htmlspecialchars($ret);
echo "OK";
