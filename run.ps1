$files = Get-ChildItem "result\" -Recurse -File -Filter *.png

$destdir = "result-webp"
foreach($f in $files) {
    $char_destdir = $destdir + "/" + $f.Directory.BaseName
    New-Item -ItemType Directory -Force -Path $char_destdir

    $char_destpath = $char_destdir + "/" + $f.Basename + ".webp"
    cwebp -m 6 -q 80 -af -mt $f.Fullname -o $char_destpath
}