<?php
include_once('config.php');

$path = $HOME;

$w = basename((string)@$_REQUEST['w']);
if (!$w) $w = 'out.wav';
$eq = basename((string)@$_REQUEST['eq']);
$channel = @$_REQUEST['channel'];
$sample = @$_REQUEST['sample'];

$wavs = array();
$wavdir = dir($path.'/wav/');
$eqs = array();
$eqsdir = dir($path.'/config/');
while (($file = $wavdir->read()) !== false){
	if (strstr($file, '.wav') || strstr($file, '.raw')) $wavs[] = $file;
}
while (($file = $eqsdir->read()) !== false){
	if (strstr($file, '_l.conf')) $eqs[] = str_replace('_l.conf', '', $file);
}

$wavdir->close();
$eqsdir->close();

if ($eq) {
	// Only allow writing to EQ configs that already exist (no new paths)
	if (!in_array($eq, $eqs, true)) {
		die('invalid eq config name');
	}
	$eq_l = @$_REQUEST['eq_l'];
	$eq_r = @$_REQUEST['eq_r'];
	$eq_l = trim($eq_l);
	$eq_r = trim($eq_r);
	if ($eq_l) file_put_contents("${HOME}/config/${eq}_l.conf", $eq_l) or die('err');
	if ($eq_r) file_put_contents("${HOME}/config/${eq}_r.conf", $eq_r) or die('err1');

	$eq_l = @file_get_contents("${HOME}/config/${eq}_l.conf");
	$eq_r = @file_get_contents("${HOME}/config/${eq}_r.conf");
}

?>
<html>
<head>
<title>eq</title>
</head>
<body>
<div style="background-color:#DDD; padding:5px;">
<form action="action.php" target="_blank">
hw:<input type="text" name="hw" value="hw:0" style="width:50px;" />
<p />
file name:<input type="text" name="w_name" value=".raw" />
duration:<input type="text" name="duration" value="10" style="width:50px;" />s
<input type="submit" name="action" value="record" />
<p />
wave file:
<select name="wave">
<?php foreach ($wavs as $file) {?>
<option value="<?php echo htmlspecialchars($file); ?>"><?php echo htmlspecialchars($file); ?></option>
<?php } ?>
</select>
eq:<select name="eq">
<option value=""></option>
<?php foreach ($eqs as $file) { ?>
<option value="<?php echo htmlspecialchars($file); ?>"><?php echo htmlspecialchars($file); ?></option>
<?php }?>
</select>
<input type="submit" name="action" value="play" />&nbsp;<input type="submit" name="action" value="measure" />
</form>
</div>
<p />
wave:&nbsp;
<?php foreach ($wavs as $file) { ?>
<a href="index.php?w=<?php echo urlencode($file); ?>&eq=<?php echo urlencode($eq); ?>&channel=<?php echo urlencode($channel); ?>&sample=<?php echo urlencode($sample); ?>"><?php echo htmlspecialchars($file); ?></a>&nbsp;
<?php }?>
<p />
eq:&nbsp;
<?php foreach ($eqs as $file) { ?>
<a href="index.php?w=<?php echo urlencode($w); ?>&eq=<?php echo urlencode($file); ?>&channel=<?php echo urlencode($channel); ?>&sample=<?php echo urlencode($sample); ?>"><?php echo htmlspecialchars($file); ?></a>&nbsp;
<?php }?>
<p />
channel: <a href="index.php?w=<?php echo urlencode($w); ?>&eq=<?php echo urlencode($eq); ?>&channel=l&sample=<?php echo urlencode($sample); ?>">l</a>&nbsp;<a href="index.php?w=<?php echo urlencode($w); ?>&eq=<?php echo urlencode($eq); ?>&channel=r&sample=<?php echo urlencode($sample); ?>">r</a>
<p />
<image src="line.php?w=<?php echo urlencode($w); ?>&eq=<?php echo urlencode($eq); ?>&channel=<?php echo urlencode($channel); ?>&sample=<?php echo urlencode($sample); ?>" />
<p />
<form method="post" action="index.php">
<input type="hidden" name="w" value="<?php echo htmlspecialchars($w);?>" />
<input type="hidden" name="eq" value="<?php echo htmlspecialchars($eq); ?>" />
<input type="hidden" name="channel" value="<?php echo htmlspecialchars($channel); ?>" />
<input type="hidden" name="sample" value="<?php echo htmlspecialchars($sample); ?>" />
<textarea name="eq_l" style="width:300px; height:200px"><?php echo htmlspecialchars($eq_l); ?></textarea>
<textarea name="eq_r" style="width:300px; height:200px"><?php echo htmlspecialchars($eq_r); ?></textarea>
<input type="submit" name="save" value="save" />
</form>
</body>
</html>
