<?php
include_once('config.php');

$file = @$_REQUEST['w'];
$eq = @$_REQUEST['eq'];
$channel = @$_REQUEST['channel'];
$sample = @$_REQUEST['sample'];
if (!$channel) $channel = 'l';
if (!$sample) $sample = 1000;

$wave = $HOME.$file;
if (!file_exists($wave)) {
	die('file not found');
}

$play_cmd = 'cat';
if (strstr($wave, 'wav')) $play_cmd = $HOME.'/wave';

$cmd = "$play_cmd $wave | $HOME/line $channel $sample";
if ($eq)
	$cmd = "$play_cmd $wave | $HOME/eq $eq 80 | $HOME/line $channel $sample";
ob_start();
passthru($cmd);
$ret = ob_get_clean();

$lines = explode("\n", $ret);

$data = array();

$max = 0;
foreach ($lines as $line) {
	$line = trim($line);
	$nums = explode("\t", $line);
	if (count($nums) != 2) continue;
	//$d = $nums[0] - $nums[1];
	$d = max(abs($nums[0]), abs($nums[1]));
	$data[] = $d;
	if ($max < $d) $max = $d;
}

$info = stat($wave);
$t_time = number_format($info[7] / 8 / 48000, 1);

$w = 1200;
$h = 600;

$image = imagecreatetruecolor($w, $h);
$bgcolor = imagecolorallocate($image, 0, 0, 0);
$red = imagecolorallocate($image, 255, 0, 0);
$white = imagecolorallocate($image, 200, 200, 200);
$blue = imagecolorallocate($image, 0, 255, 100);


imageline($image, 1, $h - 1, $w - 1, $h - 1, $red);
imageline($image, 1, 1, 1, $h - 1, $red);

$c = count($data) > 0 ? (($w - 2) / count($data)) : 0;
$c1 = $max > 0 ? (($h - 2) / $max) : 0;

foreach ($data as $i => $d) {
	$x = intval(1 + $i * $c);
	$y = $h - intval($d * $c1);
	//echo "$i $d $x $y <br />";
	imageline($image, $x, $h - 1, $x, $y, $red);
}

$l_cnt = 5;
for ($i = 1; $i < $l_cnt; ++$i) {
	$l_h = $h / $l_cnt * $i;
	imageline($image, 1, $h - $l_h, $w - 1, $h - $l_h, $white);
	imagettftext($image, 11, 0, $w - 100, $h - $l_h - 8, $blue, $HOME.'/arial.ttf', intval($max / $l_cnt * $i));
}
imagettftext($image, 11, 0, $w - 100, 20, $blue, $HOME.'/arial.ttf', $max);

$l_cnt = 20;
for ($i = 1; $i < $l_cnt; ++$i) {
	$l_w = $w / $l_cnt * $i;
	imageline($image, $l_w, 1, $l_w, $h - 1, $white);
	imagettftext($image, 11, 0, $l_w - 25, $h - 8, $blue, $HOME.'/arial.ttf', number_format($t_time / $l_cnt * $i, 2));
}
imagettftext($image, 11, 0, $w - 30, $h - 8, $blue, $HOME.'/arial.ttf', $t_time);



header('Content-type:image/png');
imagepng($image);
imagedestroy($image);
