; Inno Setup script - builds Apprehension-Setup.exe (installs the VST3 for FL Studio and other hosts)
#ifndef VstDir
  #define VstDir "..\build\Apprehension_artefacts\Release\VST3\Apprehension.vst3"
#endif

[Setup]
AppId={{6D1E4C52-9B7A-4F0C-8A53-2E7B1C9D4A21}
AppName=Apprehension
AppVersion=1.3.1
AppPublisher=Reyaan.
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=Output
OutputBaseFilename=Apprehension-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Files]
Source: "{#VstDir}\*"; DestDir: "{commoncf64}\VST3\Apprehension.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

Source: "..\assets\manual.html"; DestDir: "{commonappdata}\Reyaan\Apprehension"; DestName: "Apprehension Manual.html"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Apprehension Manual"; Filename: "{commonappdata}\Reyaan\Apprehension\Apprehension Manual.html"

[Messages]
FinishedLabel=Apprehension by Reyaan. was installed. In FL Studio: Options > Manage plugins > Find installed plugins, then add it from a mixer effect slot.
