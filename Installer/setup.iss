; setup.iss
; Скрипт установщика для File Manager Pro
; Автоматическая сборка со всеми зависимостями

#define MyAppName "File Manager Pro"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "FileManager Inc."
#define MyAppURL "https://example.com/filemanager"
#define MyAppExeName "FileManager.exe"
#define MyAppAssocName MyAppName + " File"
#define MyAppAssocExt ".fmproj"
#define MyAppAssocKey StringChange(MyAppAssocName, " ", "") + MyAppAssocExt

; Пути к файлам проекта (автоматическое определение)
#define SourcePath "..\GUI_CSharp\bin\Release\net6.0-windows\"
#define CoreLibPath "..\..\..\CoreLib_C\"
#define LogicLibPath "..\..\LogicLib_Cpp\"
#define RedistPath ".\Redist\"

[Setup]
; Основная информация о приложении
AppId={A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

; Настройки установки
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=.\License.txt
InfoBeforeFile=.\Readme.txt
OutputDir=.\Output
OutputBaseFilename=FileManagerPro_Setup
Compression=lzma2/ultra
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
SetupMutex={#MyAppName}_Setup_Mutex
AppMutex={#MyAppName}_App_Mutex

; Внешний вид
WizardSmallImageFile=.\Resources\setup-icon.bmp
SetupIconFile=.\Resources\app-icon.ico

; Подпись установщика (опционально)
;SignTool=signtool
;SignedUninstaller=yes

[Languages]
; Поддерживаемые языки
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
; Задачи, предлагаемые пользователю
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 6.1; Check: not IsAdminInstallMode
Name: "associatefm"; Description: "Ассоциировать файлы .fmproj с {#MyAppName}"; GroupDescription: "Расширения файлов:"; Flags: unchecked
Name: "addtopath"; Description: "Добавить в PATH переменную окружения"; GroupDescription: "Системные настройки:"; Flags: unchecked
Name: "installcontext"; Description: "Добавить в контекстное меню проводника"; GroupDescription: "Интеграция с проводником:"; Flags: unchecked

[Files]
; Основные файлы приложения
Source: "{#SourcePath}{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourcePath}*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourcePath}*.pdb"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#SourcePath}*.config"; DestDir: "{app}"; Flags: ignoreversion

; Иконки и ресурсы
Source: ".\Resources\*"; DestDir: "{app}\Resources"; Flags: ignoreversion recursesubdirs createallsubdirs

; Документация
Source: ".\Help\*"; DestDir: "{app}\Help"; Flags: ignoreversion recursesubdirs createallsubdirs

; Нативные библиотеки (C/C++)
Source: "{#CoreLibPath}CoreLib_C.dll"; DestDir: "{app}"; Flags: ignoreversion 64bit
Source: "{#CoreLibPath}CoreLib_C.lib"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist 64bit
Source: "{#LogicLibPath}LogicLib_Cpp.dll"; DestDir: "{app}"; Flags: ignoreversion 64bit
Source: "{#LogicLibPath}LogicLib_Cpp.lib"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist 64bit

; Распространяемые библиотеки Visual C++ (x64)
Source: "{#RedistPath}vcruntime140.dll"; DestDir: "{app}"; Flags: ignoreversion 64bit
Source: "{#RedistPath}vcruntime140_1.dll"; DestDir: "{app}"; Flags: ignoreversion 64bit
Source: "{#RedistPath}msvcp140.dll"; DestDir: "{app}"; Flags: ignoreversion 64bit
Source: "{#RedistPath}msvcp140_1.dll"; DestDir: "{app}"; Flags: ignoreversion 64bit
Source: "{#RedistPath}msvcp140_2.dll"; DestDir: "{app}"; Flags: ignoreversion 64bit
Source: "{#RedistPath}msvcp140_codecvt_ids.dll"; DestDir: "{app}"; Flags: ignoreversion 64bit
Source: "{#RedistPath}concrt140.dll"; DestDir: "{app}"; Flags: ignoreversion 64bit
Source: "{#RedistPath}vccorlib140.dll"; DestDir: "{app}"; Flags: ignoreversion 64bit

; Распространяемые библиотеки .NET (если нужно)
; Source: "{#RedistPath}dotNetRuntime\*"; DestDir: "{app}\dotNetRuntime"; Flags: ignoreversion recursesubdirs

; Примеры и шаблоны
Source: ".\Examples\*"; DestDir: "{app}\Examples"; Flags: ignoreversion recursesubdirs createallsubdirs

[Registry]
; Ассоциация файлов
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocExt}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppAssocName}"; Flags: uninsdeletevalue; Tasks: associatefm
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocName}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName} Project"; Flags: uninsdeletekey; Tasks: associatefm
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocName}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"; Tasks: associatefm
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocName}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: associatefm

; Добавление в контекстное меню проводника
Root: HKCR; Subkey: "Directory\shell\{#MyAppName}"; ValueType: string; ValueName: ""; ValueData: "Открыть в {#MyAppName}"; Flags: uninsdeletekey; Tasks: installcontext
Root: HKCR; Subkey: "Directory\shell\{#MyAppName}"; ValueType: string; ValueName: "Icon"; ValueData: "{app}\{#MyAppExeName},0"; Tasks: installcontext
Root: HKCR; Subkey: "Directory\shell\{#MyAppName}\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%V"""; Tasks: installcontext

; Запись в PATH переменную окружения
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; Check: NeedsAddPath(ExpandConstant('{app}')); Tasks: addtopath

; Настройки приложения в реестре
Root: HKCU; Subkey: "Software\{#MyAppPublisher}\{#MyAppName}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\{#MyAppPublisher}\{#MyAppName}\Settings"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"
Root: HKCU; Subkey: "Software\{#MyAppPublisher}\{#MyAppName}\Settings"; ValueType: dword; ValueName: "Version"; ValueData: {#MyAppVersion}

; Автозапуск при входе в систему (опционально)
; Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "{#MyAppName}"; ValueData: """{app}\{#MyAppExeName}"" /minimized"; Flags: uninsdeletevalue

[Icons]
; Ярлыки в меню Пуск
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{group}\Справка"; Filename: "{app}\Help\index.html"
Name: "{group}\Примеры"; Filename: "{app}\Examples"

; Дополнительные ярлыки
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
; Запуск после установки
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
Filename: "{app}\Help\index.html"; Description: "Открыть справку"; Flags: nowait postinstall skipifsilent unchecked
Filename: "https://example.com/filemanager/docs"; Description: "Посетить сайт документации"; Flags: nowait postinstall skipifsilent shellexec unchecked

; Установка распространяемых пакетов (если нужно)
; Filename: "{app}\dotNetRuntime\windowsdesktop-runtime-6.0.0-win-x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Установка .NET Runtime..."; Flags: waituntilterminated skipifdoesntexist

[UninstallRun]
; Действия при удалении
Filename: "{cmd}"; Parameters: "/C taskkill /f /im ""{#MyAppExeName}"" 2>nul 1>nul"; Flags: runhidden

[UninstallDelete]
; Удаление дополнительных файлов
Type: filesandordirs; Name: "{app}\Logs"
Type: filesandordirs; Name: "{app}\Temp"
Type: filesandordirs; Name: "{app}\Cache"
Type: files; Name: "{app}\*.log"
Type: files; Name: "{app}\*.tmp"

[Code]
// Pascal-скрипт для кастомной логики

// Проверка версии .NET
function IsDotNetInstalled: Boolean;
var
  ErrorCode: Integer;
  IsInstalled: Boolean;
  Version: String;
begin
  IsInstalled := RegQueryStringValue(HKLM, 'SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full', 'Version', Version);
  if not IsInstalled then
    IsInstalled := RegQueryStringValue(HKLM, 'SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Client', 'Version', Version);
  
  if IsInstalled then
  begin
    // Проверяем .NET 4.8 или выше
    if CompareVersion(Version, '4.8.0') >= 0 then
      Result := True
    else
    begin
      MsgBox('Требуется .NET Framework 4.8 или выше.'#13#10'Пожалуйста, установите последнюю версию .NET Framework.', mbError, MB_OK);
      Result := False;
    end;
  end
  else
  begin
    MsgBox('.NET Framework не найден.'#13#10'Пожалуйста, установите .NET Framework 4.8 или выше.', mbError, MB_OK);
    Result := False;
  end;
end;

// Проверка наличия Visual C++ Redistributable
function IsVCRedistInstalled: Boolean;
var
  Version: String;
begin
  // Проверяем Visual C++ 2015-2022 Redistributable (x64)
  Result := RegKeyExists(HKLM, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64');
  if not Result then
    Result := RegKeyExists(HKLM, 'SOFTWARE\WOW6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x64');
end;

// Проверка, нужно ли добавлять в PATH
function NeedsAddPath(Param: string): boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKEY_LOCAL_MACHINE,
    'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
    'Path', OrigPath)
  then begin
    Result := True;
    exit;
  end;
  
  // Ищем путь в переменной PATH
  Result := Pos(';' + Param + ';', ';' + OrigPath + ';') = 0;
end;

// Проверка прав администратора
function IsAdmin: Boolean;
begin
  Result := IsAdminLoggedOn or IsPowerUserLoggedOn;
end;

// Проверка свободного места на диске
function CheckDiskSpace: Boolean;
var
  Drive: String;
  FreeSpace, RequiredSpace: Int64;
begin
  Drive := ExtractFileDrive(ExpandConstant('{app}'));
  RequiredSpace := 200 * 1024 * 1024; // 200 MB
  
  FreeSpace := DiskFreeSpace(Drive);
  
  if FreeSpace < RequiredSpace then
  begin
    MsgBox('Недостаточно места на диске.'#13#10 +
           'Требуется: ' + IntToStr(RequiredSpace div (1024*1024)) + ' MB'#13#10 +
           'Свободно: ' + IntToStr(FreeSpace div (1024*1024)) + ' MB',
           mbError, MB_OK);
    Result := False;
  end
  else
    Result := True;
end;

// Проверка версии Windows
function IsWindowsVersionSupported: Boolean;
var
  Version: TWindowsVersion;
begin
  GetWindowsVersionEx(Version);
  
  // Требуется Windows 10 или выше
  if (Version.Major > 10) or 
     (Version.Major = 10 and Version.Minor >= 0) then
    Result := True
  else if (Version.Major = 10) then
    Result := True
  else
  begin
    MsgBox('Требуется Windows 10 или новее.', mbError, MB_OK);
    Result := False;
  end;
end;

// Обработчик события инициализации установки
function InitializeSetup: Boolean;
begin
  // Проверяем, не запущено ли уже приложение
  if CheckForMutexes('{#MyAppName}_App_Mutex') then
  begin
    MsgBox('Пожалуйста, закройте File Manager Pro перед установкой.', mbError, MB_OK);
    Result := False;
    exit;
  end;
  
  // Проверяем версию Windows
  if not IsWindowsVersionSupported then
  begin
    Result := False;
    exit;
  end;
  
  // Проверяем .NET
  if not IsDotNetInstalled then
  begin
    Result := False;
    exit;
  end;
  
  // Проверяем наличие Visual C++ Redistributable
  if not IsVCRedistInstalled then
  begin
    if MsgBox('Требуется Visual C++ Redistributable 2015-2022.'#13#10 +
              'Установить сейчас?', mbConfirmation, MB_YESNO) = IDYES then
    begin
      // Скачиваем и устанавливаем VC++ Redist
      ShellExec('open', 
                'https://aka.ms/vs/17/release/vc_redist.x64.exe',
                '', '', SW_SHOW, ewWaitUntilTerminated, ErrorCode);
    end
    else
    begin
      Result := False;
      exit;
    end;
  end;
  
  Result := True;
end;

// Обработчик события инициализации деинсталляции
function InitializeUninstall: Boolean;
begin
  // Закрываем приложение перед удалением
  Exec(ExpandConstant('{cmd}'), '/C taskkill /f /im "{#MyAppExeName}" 2>nul 1>nul', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Sleep(1000); // Даём время на завершение процессов
  
  Result := True;
end;

// Кастомная страница для выбора компонентов
var
  ComponentsPage: TInputOptionWizardPage;

procedure InitializeWizard;
begin
  // Создаём кастомную страницу выбора компонентов
  ComponentsPage := CreateInputOptionPage(wpSelectComponents,
    'Выбор компонентов', 'Какие компоненты установить?',
    'Выберите компоненты для установки, затем нажмите Далее.',
    True, False);
  
  ComponentsPage.Add('Основные файлы приложения');
  ComponentsPage.Add('Примеры и шаблоны');
  ComponentsPage.Add('Документация и справка');
  ComponentsPage.Add('Интеграция с проводником');
  
  // Выбираем по умолчанию
  ComponentsPage.Values[0] := True;
  ComponentsPage.Values[1] := True;
  ComponentsPage.Values[2] := True;
  ComponentsPage.Values[3] := True;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  // Пропускаем страницу выбора компонентов, если установка тихая
  if (PageID = ComponentsPage.ID) and WizardSilent() then
    Result := True
  else
    Result := False;
end;

// Проверка выбранных компонентов
function GetComponentsSelection: String;
begin
  Result := '';
  if ComponentsPage.Values[0] then Result := Result + 'main,';
  if ComponentsPage.Values[1] then Result := Result + 'examples,';
  if ComponentsPage.Values[2] then Result := Result + 'docs,';
  if ComponentsPage.Values[3] then Result := Result + 'explorer_integration,';
end;

[InstallDelete]
; Удаление старых версий
Type: filesandordirs; Name: "{app}\*"; Check: IsOldVersion

function IsOldVersion: Boolean;
var
  OldVersion: String;
begin
  // Проверяем наличие старой версии
  if RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#MyAppName}_is1', 
                         'DisplayVersion', OldVersion) then
  begin
    if CompareVersion(OldVersion, '{#MyAppVersion}') < 0 then
      Result := True
    else
      Result := False;
  end
  else
    Result := False;
end;

[Messages]
; Кастомизация сообщений
ButtonNext=Далее >
ButtonBack=< Назад
ButtonInstall=Установить
ButtonCancel=Отмена
ButtonFinish=Завершить
ButtonBrowse=Обзор...
ButtonWizardBrowse=Обзор...

WelcomeLabel1=Добро пожаловать в мастер установки [name]
WelcomeLabel2=Мастер установки [name/ver] на ваш компьютер.%n%nРекомендуется закрыть все другие приложения перед началом установки.

ClickNext=Нажмите "Далее" для продолжения.
ClickInstall=Нажмите "Установить" для начала установки.
ClickFinish=Нажмите "Завершить" для выхода из мастера установки.

StatusExtractFiles=Извлечение файлов...
StatusCreateIcons=Создание ярлыков...
StatusCreateRegistryKeys=Создание записей в реестре...
StatusRegisterFiles=Регистрация файлов...
StatusRollback=Отмена изменений...
StatusUninstalling=Удаление [name]...

FinishedHeadingLabel=Завершение работы мастера установки [name]
FinishedLabel=Мастер успешно установил [name] на ваш компьютер.
FinishedLabelNoIcons=Мастер успешно установил [name] на ваш компьютер.