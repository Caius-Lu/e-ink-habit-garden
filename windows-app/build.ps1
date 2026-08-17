$ErrorActionPreference = 'Stop'
$windowsDir = if ($env:WINDIR) { $env:WINDIR } elseif ($env:SystemRoot) { $env:SystemRoot } else { 'C:\Windows' }
$csc = Join-Path $windowsDir 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if (-not (Test-Path $csc)) { throw 'Windows C# compiler was not found.' }

& $csc /nologo /target:winexe /optimize+ /codepage:65001 `
  /r:System.Windows.Forms.dll /r:System.Drawing.dll /r:System.Net.Http.dll `
  /r:System.Web.Extensions.dll /out:InkGarden.exe DeviceClient.cs InkGardenApp.cs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $csc /nologo /optimize+ /codepage:65001 /r:System.Net.Http.dll `
  /r:System.Web.Extensions.dll /out:DeviceClientTests.exe DeviceClient.cs DeviceClientTests.cs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& .\DeviceClientTests.exe
