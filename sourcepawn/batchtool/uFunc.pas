(* AMX Mod X
*    compile.exe
*
* by the AMX Mod X Development Team
*
*
*  This program is free software; you can redistribute it and/or modify it
*  under the terms of the GNU General Public License as published by the
*  Free Software Foundation; either version 2 of the License, or (at
*  your option) any later version.
*
*  This program is distributed in the hope that it will be useful, but
*  WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
*  General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software Foundation,
*  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
*  In addition, as a special exception, the author gives permission to
*  link the code of this program with the Half-Life Game Engine ("HL
*  Engine") and Modified Game Libraries ("MODs") developed by Valve,
*  L.L.C ("Valve"). You must obey the GNU General Public License in all
*  respects for all of the code used other than the HL Engine and MODs
*  from Valve. If you modify this file, you may extend this exception
*  to your version of the file, but you are not obligated to do so. If
*  you do not wish to do so, delete this exception statement from your
*  version.
*)

unit uFunc;

interface

uses
  Windows, SysUtils, Classes, Math, IniFiles;

resourcestring
  COMPILER_EXE = 'spcomp.exe';

procedure AppExit;
procedure CompilePlugin(const Name, OutputDir: String);
function GetAgeFromDat(const FileName: String): Integer;
procedure SetAgeToDat(const FileName: String; const Age: Integer);
function GetConsoleOutput(const Command: String; var Output: TStringList): Boolean;

implementation

procedure AppExit;
begin
  WriteLn;
  Write('Press enter to exit ...');
  ReadLn;
  Halt;
end;

procedure CompilePlugin(const Name, OutputDir: String);
var
  Output: TStringList;
  i: Word;
  cStart,cEnd: Longword;
  FileName, FilePath, Compiled: String;
begin
  FileName := ExtractFileName(Name);
  FilePath := ExtractFilePath(Name);
  if (OutputDir = '') then
    Compiled := FilePath+'compiled\'+ChangeFileExt(Filename,'.smx')
  else
    Compiled := OutputDir+ChangeFileExt(Filename,'.smx');

  if (FilePath='') then
    FilePath := ExtractFilePath(ParamStr(0));

  WriteLn;
  WriteLn('//// '+ExtractFileName(FileName));

  if FileExists(Compiled) and ( GetAgeFromDat(Name)=FileAge(Name) ) then
  begin
    WriteLn('// Already compiled.');
    WriteLn('// ----------------------------------------');
    Exit;
  end;

  Output := TStringList.Create;

  try
    cStart := GetTickCount;
    if not GetConsoleOutput(ExtractFilePath(ParamStr(0))+COMPILER_EXE+' "'+Name+'" "-o'+Compiled+'"',Output) then
    begin
      WriteLn('// Internal error.');
      AppExit;
    end;
    cEnd := GetTickCount;

    for i := 3 to (Output.Count-1) do
    begin
      WriteLn('// '+Output.Strings[i]);
    end;

    WriteLn('//');
    WriteLn('// Compilation Time: '+FloatToStr(SimpleRoundTo((cEnd-cStart) / 1000,-2))+' sec');
    WriteLn('// ----------------------------------------');
    Output.Free;
  except
    WriteLn('// Internal error.');
    AppExit;
  end;

  SetAgeToDat(Name,FileAge(Name));
end;

function GetAgeFromDat(const FileName: String): Integer;
var
  Ini: TIniFile;
begin
  Ini := TIniFile.Create(ExtractFilePath(ParamStr(0))+'compile.dat');
  Result := Ini.ReadInteger(FileName,'Age',-1);
  Ini.Free;
end;

procedure SetAgeToDat(const FileName: String; const Age: Integer);
var
  Ini: TIniFile;
begin
  Ini := TIniFile.Create(ExtractFilePath(ParamStr(0))+'compile.dat');
  Ini.WriteInteger(FileName,'Age',Age);
  Ini.UpdateFile;
  Ini.Free;
  SetFileAttributes(PChar(ExtractFilePath(ParamStr(0))+'compile.dat'), faHidden);
end;

function GetConsoleOutput(const Command: String; var Output: TStringList): Boolean;
var
  StartupInfo: TStartupInfo;
  ProcessInfo: TProcessInformation;
  SecurityAttr: TSecurityAttributes;
  PipeOutputRead: THandle;
  PipeOutputWrite: THandle;
  PipeErrorsRead: THandle;
  PipeErrorsWrite: THandle;
  Succeed: Boolean;
  Buffer: array [0..255] of Char;
  NumberOfBytesRead: DWORD;
  Stream: TMemoryStream;
  Errors: String;
begin
  FillChar(ProcessInfo, SizeOf(TProcessInformation), 0);
  FillChar(SecurityAttr, SizeOf(TSecurityAttributes), 0);

  SecurityAttr.nLength := SizeOf(SecurityAttr);
  SecurityAttr.bInheritHandle := True;
  SecurityAttr.lpSecurityDescriptor := nil;

  CreatePipe(PipeOutputRead, PipeOutputWrite, @SecurityAttr, 0);
  CreatePipe(PipeErrorsRead, PipeErrorsWrite, @SecurityAttr, 0);

  FillChar(StartupInfo, SizeOf(TStartupInfo), 0);
  StartupInfo.cb:=SizeOf(StartupInfo);
  StartupInfo.hStdInput := 0;
  StartupInfo.hStdOutput := PipeOutputWrite;
  StartupInfo.hStdError := PipeErrorsWrite;
  StartupInfo.wShowWindow := SW_HIDE;
  StartupInfo.dwFlags := STARTF_USESHOWWINDOW or STARTF_USESTDHANDLES;

  if  CreateProcess(nil, PChar(command), nil, nil, true, CREATE_DEFAULT_ERROR_MODE or CREATE_NEW_CONSOLE or NORMAL_PRIORITY_CLASS, nil, nil, StartupInfo, ProcessInfo) then begin
    Result := True;
    CloseHandle(PipeOutputWrite);
    CloseHandle(PipeErrorsWrite);

    Stream := TMemoryStream.Create;
    try
      while True do begin
        Succeed := ReadFile(PipeOutputRead, Buffer, 255, NumberOfBytesRead, nil);
        if not Succeed then Break;
        Stream.Write(Buffer, NumberOfBytesRead);
      end;
    finally
      // nothing
    end;
    Stream.Position := 0;
    Output.LoadFromStream(Stream);
    Stream.Free;
    CloseHandle(PipeOutputRead);

    Errors := '';
    try
      while True do
      begin
        Succeed := ReadFile(PipeErrorsRead, Buffer, 255, NumberOfBytesRead, nil);
        if not Succeed then Break;
        Errors := Errors + Copy(Buffer, 1, NumberOfBytesRead);
      end;
    finally
      // nothing
    end;
    CloseHandle(PipeErrorsRead);

    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    CloseHandle(ProcessInfo.hProcess);
  end
  else
  begin
    Result := False;
    CloseHandle(PipeOutputRead);
    CloseHandle(PipeOutputWrite);
    CloseHandle(PipeErrorsRead);
    CloseHandle(PipeErrorsWrite);
  end;
  // error management
  if (Errors <> '') then begin
    if (Output.Count > 1) then begin
      if (Trim(Output[Output.Count -2]) = '') then
        Output.Strings[Output.Count -2] := TrimRight(Errors)
      else
        Output.Insert(Output.Count -1, TrimRight(Errors));
    end
    else
      Output.Add(Errors);
    Output.Text := Output.Text; // pseudo-rearrangement
  end;
end;

end.
