#ifndef AppVersion
  #define AppVersion "0.9.4"
#endif
#ifndef ProjectDir
  #define ProjectDir ".."
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif

[Setup]
AppId={{06D484E6-7284-4BE5-A42D-6354613B93F4}
AppName=GrainsDosage
AppVersion={#AppVersion}
AppPublisher=Daniel Melegari
DefaultDirName={autopf}\GrainsDosage
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
OutputDir={#OutputDir}
OutputBaseFilename=GrainsDosage-{#AppVersion}-Windows10-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
CloseApplicationsFilter=*.exe,*.dll,*.vst3
RestartApplications=no
UninstallDisplayName=GrainsDosage VST3
InfoBeforeFile={#ProjectDir}\installer\BEFORE-INSTALL.txt

[Files]
Source: "{#ProjectDir}\build\VST3\Release\GrainsDosage.vst3\*"; DestDir: "{commoncf64}\VST3\GrainsDosage.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ProjectDir}\INSTALL-WINDOWS.txt"; DestDir: "{app}"; Flags: ignoreversion

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
