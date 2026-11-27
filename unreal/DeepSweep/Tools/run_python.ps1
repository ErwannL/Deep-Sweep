# Lance un script Python editeur en mode commandlet (sans interface).
param([Parameter(Mandatory)][string]$Script, [string]$Engine = "C:\Program Files\Epic Games\UE_5.4")
$proj = Join-Path $PSScriptRoot "..\DeepSweep.uproject" | Resolve-Path
$s = Resolve-Path $Script
& "$Engine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $proj -run=pythonscript "-script=$s" -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput
exit $LASTEXITCODE
