; Instalador Windows (Inno Setup 6) — Choir Crowd
; Compilar: iscc /DAppVersion=1.0.0 /DArtefacts=..\..\build\ChoirCrowd_artefacts\Release packaging\windows\ChoirCrowd.iss

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef Artefacts
  #define Artefacts "..\..\build\ChoirCrowd_artefacts\Release"
#endif

[Setup]
AppId={{7C1E7E52-3B0B-4C55-9E3A-6C4C2B7A1C11}
AppName=Choir Crowd
AppVersion={#AppVersion}
AppPublisher=Logogueto
AppPublisherURL=https://github.com/kidymusiic-crypto/Mungoz-Choir
DefaultDirName={autopf}\Logogueto\Choir Crowd
DefaultGroupName=Logogueto
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\..\out
OutputBaseFilename=ChoirCrowd-{#AppVersion}-Windows-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=Choir Crowd {#AppVersion}

[Languages]
Name: "pt"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "ptbr"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Instalação completa"
Name: "custom"; Description: "Personalizada"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 (Ableton, FL Studio, Cubase, Studio One, Reaper, Bitwig...)"; Types: full custom; Flags: fixed
Name: "standalone"; Description: "Aplicação Standalone (toca sem DAW)"; Types: full

[Files]
; VST3 -> C:\Program Files\Common Files\VST3 (pasta padrão que todas as DAWs analisam)
Source: "{#Artefacts}\VST3\Choir Crowd.vst3\*"; DestDir: "{commoncf64}\VST3\Choir Crowd.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\Standalone\Choir Crowd.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{group}\Choir Crowd"; Filename: "{app}\Choir Crowd.exe"; Components: standalone
Name: "{group}\Desinstalar Choir Crowd"; Filename: "{uninstallexe}"

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Choir Crowd.vst3"
