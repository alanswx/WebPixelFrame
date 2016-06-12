cd ../
rm -rf data/po
rm data/p/js/lib/piskel-packageda.js
php tools/convertdir.php
cd data
gzip po/71	
gzip po/73
gzip po/75
gzip po/8
for file in `ls -A1 po`; do curl -F "file=@$PWD/po/$file;filename=/po/$file" 10.0.2.19/edit; done
