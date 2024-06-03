$buildMode = $args[0]

Write-Output "Build Windows $buildMode"

Set-Location ../projects/windows

if (Test-Path build) {
    Remove-Item -Recurse -Force build
}

if (Test-Path src) {
    Remove-Item -Recurse -Force src
}

Copy-Item -Recurse -Force ../../src ./
Copy-Item -Recurse -Force ../../sample ./

$CURRENT_DIR = Get-Location
cmake . -DCMAKE_BUILD_TYPE="$buildMode" -B build -DCURL_LIB_DIR="$CURRENT_DIR/$buildMode" -DCURL_INCLUDE_DIRS="$CURRENT_DIR/Include"
cmake --build build --clean-first --config "$buildMode"

Get-ChildItem "build/lib/$buildMode"

Remove-Item -Recurse -Force src
Remove-Item -Recurse -Force sample
Set-Location ../../scripts

