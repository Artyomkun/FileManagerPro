[Code]
function CheckForUpdates: Boolean;
var
  UpdateURL: String;
  UpdateFile: String;
begin
  UpdateURL := 'https://example.com/filemanager/update/version.txt';
  UpdateFile := ExpandConstant('{tmp}\version_check.txt');
  
  if DownloadTemporaryFile(UpdateURL, UpdateFile, '', '', 0, nil) then
  begin
    // Сравниваем версии
    // ...
  end;
end;