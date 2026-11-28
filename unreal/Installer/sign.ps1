# Signature de code (optionnelle) : a utiliser le jour ou un certificat de signature est disponible.
#   - Certificat .pfx :      $env:DS_SIGN_PFX = "C:\chemin\cert.pfx" ; $env:DS_SIGN_PASSWORD = "..."
#   - Azure Trusted Signing : voir https://learn.microsoft.com/azure/trusted-signing/ (dlib + metadata.json)
# Sans certificat, le script ne fait rien (Windows SmartScreen affichera "Executer quand meme").
param([Parameter(Mandatory)][string[]]$Files)

$signtool = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin\*\x64\signtool.exe" -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1
if (-not $env:DS_SIGN_PFX) { Write-Host "Pas de certificat (DS_SIGN_PFX) : signature ignoree."; exit 0 }
if (-not $signtool) { Write-Error "signtool.exe introuvable (Windows SDK)."; exit 1 }
foreach ($f in $Files) {
    & $signtool.FullName sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /f $env:DS_SIGN_PFX /p $env:DS_SIGN_PASSWORD $f
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
