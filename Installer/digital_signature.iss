[Setup]
SignTool=signtool /tr http://timestamp.digicert.com /td sha256 /fd sha256 /a $f
SignedUninstaller=yes

[Code]
procedure SignFile(const FileName: string);
var
  ResultCode: Integer;
begin
  if Exec('signtool.exe', 
          'sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 "' + FileName + '"', 
          '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
  begin
    if ResultCode <> 0 then
      Log('Ошибка подписи файла: ' + FileName);
  end;
end;