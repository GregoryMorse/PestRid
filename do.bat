REM Sign
if not exist PestRidDrv\PestRid.cer certutil.exe -verifystore Root "PestRid CA"
if not errorlevel 1 certutil.exe -delstore Root "PestRid CA"
if not exist PestRidDrv\PestRid.cer makecert.exe -r -pe -eku 1.3.6.1.5.5.7.3.3 -n "CN=PestRid CA" -ss CA -sr CurrentUser -a sha1 -sky signature -sv PestRidDrv\PestRid.pvk PestRidDrv\PestRid.cer
certutil.exe -verifystore Root "PestRid CA"
if errorlevel 1 certutil.exe -addstore Root PestRidDrv\PestRid.cer
if not exist PestRidDrv\PestRid.pfx pvk2pfx.exe -pvk PestRidDrv\PestRid.pvk -spc PestRidDrv\PestRid.cer -pfx PestRidDrv\PestRid.pfx
signtool.exe sign /v /f PestRidDrv\PestRid.pfx /t http://timestamp.verisign.com/scripts/timstamp.dll "%~dp0\%~1\PestRid.exe"
if %errorlevel%==2 set errorlevel=0