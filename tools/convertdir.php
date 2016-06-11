<?php

  $path = 'p';//realpath('p');
/*
$objects = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($path), RecursiveIteratorIterator::SELF_FIRST);
foreach($objects as $name => $object){
    echo "$name\n";
} 
*/

echo "============\n"; 

$directory = new \RecursiveDirectoryIterator($path, \FilesystemIterator::FOLLOW_SYMLINKS);
$filter = new \RecursiveCallbackFilterIterator($directory, function ($current, $key, $iterator) {
  // Skip hidden files and directories.
  if ($current->getFilename()[0] === '.') {
    return FALSE;
  }
  else {
    // Only consume files of interest.
    return  TRUE;
  }

});
$iterator = new \RecursiveIteratorIterator($filter);
$files = array();
foreach ($iterator as $info) {
  $files[] = $info->getPathname();
}
print_r($files); 



@mkdir( "po" );
$outFile = fopen("pindex.txt", 'w') or die("can't open file");

$count=0;
foreach ($files as $key=>$value) {
  copy($value,"po/".$count);
  fwrite($outFile, "$count|$value\n");
  $count++;
}
fclose($outFile);


?>
