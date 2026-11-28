; Installateur Deep Sweep (Inno Setup 6) - installation par utilisateur, sans droits administrateur.
; Compilation : ISCC.exe /DAppVersion=0.3.0 DeepSweep.iss  (depuis unreal\Installer)

#ifndef AppVersion
  #define AppVersion "0.3.0"
#endif

[Setup]
AppId={{8C2F5E7A-3D41-4B8E-9A6C-DEE95EE7A001}
AppName=Deep Sweep
AppVersion={#AppVersion}
AppPublisher=Deep Sweep (projet perso)
AppPublisherURL=https://github.com/ErwannL/Deep-Sweep
AppSupportURL=https://github.com/ErwannL/Deep-Sweep/releases
DefaultDirName={localappdata}\Programs\Deep Sweep
DefaultGroupName=Deep Sweep
PrivilegesRequired=lowest
OutputDir=..\Build
OutputBaseFilename=DeepSweep-Setup-v{#AppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
DiskSpanning=no
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\DeepSweep.exe
CloseApplications=yes
; Une mise a jour remplace la version precedente au meme endroit
UsePreviousAppDir=yes

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Tasks]
Name: "desktopicon"; Description: "Creer un raccourci sur le Bureau"; GroupDescription: "Raccourcis :"

[InstallDelete]
; nettoie les anciens fichiers du jeu (les reglages du joueur sont ailleurs, dans AppData)
Type: filesandordirs; Name: "{app}\DeepSweep\Content"
Type: filesandordirs; Name: "{app}\DeepSweep\Binaries"

[Files]
Source: "..\Build\Windows\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Deep Sweep"; Filename: "{app}\DeepSweep.exe"
Name: "{group}\Desinstaller Deep Sweep"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Deep Sweep"; Filename: "{app}\DeepSweep.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe"; Parameters: "/quiet /norestart"; StatusMsg: "Installation des composants Microsoft (Visual C++, DirectX)..."; Flags: runasoriginaluser skipifdoesntexist; Check: NeedsPrereqs
Filename: "{app}\DeepSweep.exe"; Description: "Lancer Deep Sweep"; Flags: nowait postinstall skipifsilent

[Code]
function NeedsPrereqs: Boolean;
begin
  { Les runtimes Visual C++ 2015-2022 x64 sont-ils deja presents ? }
  Result := not RegKeyExists(HKLM, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64');
end;
