cd ../
rm -rf p
cp -r ../piskel/dest/prod p
rm -rf data/po
rm -f p/js/piskel-packageda.js
php tools/convertdir.php
cd data
curl -F "file=@$PWD/gallery.html;filename=/gallery.html" 10.0.2.29/edit
gzip po/71	
gzip po/73
gzip po/75
gzip po/8
#for file in `ls -A1 po`; do curl -F "file=@$PWD/po/$file;filename=/po/$file" 10.0.2.29/edit; done
