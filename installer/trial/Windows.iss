#ifndef AppVersion
  #error AppVersion must be supplied from the packaged plugin
#endif
#ifndef PluginDir
  #error PluginDir must point to the validated Trial VST3 bundle
#endif
#ifndef OutputDir
  #define OutputDir "dist"
#endif
[Setup]
AppId={{06D484E6-7284-4BE5-A42D-6354613B93F4}
AppName=GrainsDosage Trial
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
OutputBaseFilename=GrainsDosage-Trial-{#AppVersion}-Windows10-11-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
DisableWelcomePage=no
InfoBeforeFile={#SourcePath}\..\BEFORE-INSTALL.txt
#if FileExists(SourcePath + "\wizard-large.bmp")
WizardImageFile={#SourcePath}\wizard-large.bmp
WizardImageStretch=yes
#endif
#if FileExists(SourcePath + "\wizard-small.bmp")
WizardSmallImageFile={#SourcePath}\wizard-small.bmp
#endif
CloseApplications=yes
CloseApplicationsFilter=*.exe,*.dll,*.vst3
RestartApplications=no
UninstallDisplayName=GrainsDosage Trial VST3
[Files]
Source: "{#PluginDir}\*"; DestDir: "{commoncf64}\VST3\GrainsDosage.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"

[Code]
procedure InitializeWizard;
begin
  { Use the full welcome area so the supplied landscape logo is not squeezed
    into Inno Setup's narrow stock sidebar. Navigation stays below the image. }
  WizardForm.WelcomeLabel1.Visible := False;
  WizardForm.WelcomeLabel2.Visible := False;
  WizardForm.WizardBitmapImage.SetBounds(0, 0,
    WizardForm.WelcomePage.ClientWidth, WizardForm.WelcomePage.ClientHeight);
  { Keep the normal completion/restart messages legible on the last page. }
  WizardForm.WizardBitmapImage2.Visible := False;
  WizardForm.FinishedHeadingLabel.Left := ScaleX(24);
  WizardForm.FinishedHeadingLabel.Width := WizardForm.FinishedPage.ClientWidth - ScaleX(48);
  WizardForm.FinishedLabel.Left := ScaleX(24);
  WizardForm.FinishedLabel.Width := WizardForm.FinishedPage.ClientWidth - ScaleX(48);
end;
