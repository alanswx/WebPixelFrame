IP=10.0.2.30
cd ../
rm -rf p
cp -r ../piskel/dest/prod p
rm -rf data/po
rm -f ../p/js/piskel-packaged-2*js
php tools/convertdir.php
cd data
curl -F "file=@$PWD/index.htm;filename=/index.htm" http://admin:admin@$IP/edit
curl -F "file=@$PWD/gallery.html;filename=/gallery.html" http://admin:admin@$IP/edit
curl -F "file=@$PWD/gallery.js.gz;filename=/gallery.js.gz" http://admin:admin@$IP/edit
curl -F "file=@$PWD/gallery.css.gz;filename=/gallery.css.gz" http://admin:admin@$IP/edit
curl -F "file=@$PWD/jscolor.min.js.gz;filename=/jscolor.min.js.gz" http://admin:admin@$IP/edit
curl -F "file=@$PWD/checkback.png;filename=/checkback.png" http://admin:admin@$IP/edit
curl -F "file=@$PWD/iconmonstr-lock-3-icon.svg;filename=/iconmonstr-lock-3-icon.svg" http://admin:admin@$IP/edit
#
# to delete a file:
# curl -X  "DELETE" -F "path=/gallery.js" http://admin:admin@$IP/edit
#curl -X  "DELETE" -F "path=/piskeldata/5.json" http://admin:admin@$IP/edit
#gzip po/71	
#gzip po/73
#gzip po/75
#gzip po/8
for file in `ls -A1 po`; do gzip po/$file ; done
curl -F "file=@$PWD/pindex.txt;filename=/pindex.txt" http://admin:admin@$IP/edit
for file in `ls -A1 po`; do echo curl -F "file=@$PWD/po/$file;filename=/po/$file" http://admin:admin@$IP/edit; done
