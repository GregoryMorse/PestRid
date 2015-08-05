REM Sign drivers
if not exist PestRid.cer certutil.exe -verifystore Root "PestRid CA"
if not errorlevel 1 certutil.exe -delstore Root "PestRid CA"
if not exist PestRid.cer makecert.exe -r -pe -eku 1.3.6.1.5.5.7.3.3 -n "CN=PestRid CA" -ss CA -sr CurrentUser -a sha1 -sky signature -sv PestRid.pvk PestRid.cer
certutil.exe -verifystore Root "PestRid CA"
if errorlevel 1 certutil.exe -addstore Root PestRid.cer
if not exist PestRid.pfx pvk2pfx.exe -pvk PestRid.pvk -spc PestRid.cer -pfx PestRid.pfx
IF "%1" EQU "64" (
signtool.exe sign /v /f PestRid.pfx /t http://timestamp.verisign.com/scripts/timstamp.dll objfre_wnet_amd64\amd64\PestRidDrv.sys
) ELSE (
signtool.exe sign /v /f PestRid.pfx /t http://timestamp.verisign.com/scripts/timstamp.dll objfre_w2k_x86\i386\PestRidDrv.sys
signtool.exe sign /v /f PestRid.pfx /t http://timestamp.verisign.com/scripts/timstamp.dll objfre_wxp_x86\i386\PestRidDrv.sys
signtool.exe sign /v /f PestRid.pfx /t http://timestamp.verisign.com/scripts/timstamp.dll objfre_wnet_x86\i386\PestRidDrv.sys
)