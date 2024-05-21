
Set-Location ..
$ROOT_DIR = Get-Location
$OUTPUT_DIR="$ROOT_DIR/build/windows"
$PROJECT_NAME="aliyun_log_c_sdk"
if (Test-Path $OUTPUT_DIR) {
    Remove-Item -Recurse -Force $OUTPUT_DIR
}
$PROJECT_DIR="$OUTPUT_DIR/$PROJECT_NAME"

# unzip lib curl
if(!(Test-Path projects/windows/Include)) {
  $libcurlzip = "projects/windows/libcurl.zip"
  Expand-Archive -Path "$libcurlzip" -DestinationPath projects/windows/ -Force
  Remove-Item -Force "$libcurlzip"
}

# project
Copy-Item -Path "projects/windows/$PROJECT_NAME" -Recurse -Force -Destination "$OUTPUT_DIR/$PROJECT_NAME"
# lib curl
Copy-Item -Recurse -Force -Path projects/windows/Release -Destination "$PROJECT_DIR/Release"
Copy-Item -Recurse -Force -Path projects/windows/Debug -Destination "$PROJECT_DIR/Debug"
Copy-Item -Recurse -Force -Path projects/windows/Include -Destination "$PROJECT_DIR/Include"

# log header
Copy-Item src/*.h "$PROJECT_DIR/Include/"
if (!(Test-Path "$PROJECT_DIR/Include/curl_adapter")) {
    New-Item -ItemType Directory -Path "$PROJECT_DIR/Include/curl_adapter"
}
Copy-Item src/curl_adapter/adapter.h "$PROJECT_DIR/Include/curl_adapter/"

function Package-Windows {
  param(
    [string]$buildMode
  )
  Set-Location "$ROOT_DIR/scripts"
  ./build-windows.ps1 "$buildMode"
  Set-Location ..

  Write-Output "generate artifact $buildMode"
  Copy-Item "./projects/windows/build/lib/$buildMode/*" "$PROJECT_DIR/$buildMode/"
  
}

Package-Windows "Release"
Package-Windows "Debug"

Compress-Archive -Path "$OUTPUT_DIR" -DestinationPath "$ROOT_DIR/build/$PROJECT_NAME-windows.zip"
Set-Location "$ROOT_DIR/scripts"
