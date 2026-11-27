# Compile la cible editeur DeepSweepEditor (Win64 Development).
param([string]$Engine = "C:\Program Files\Epic Games\UE_5.4")
$proj = Join-Path $PSScriptRoot "..\DeepSweep.uproject" | Resolve-Path
& "$Engine\Engine\Build\BatchFiles\Build.bat" DeepSweepEditor Win64 Development "-Project=$proj" -WaitMutex -NoHotReloadFromIDE
exit $LASTEXITCODE
