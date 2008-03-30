(* SourceMod
*    compile.exe
*
* by the SourceMod Development Team (adapted from AMX Mod X's compiler tool)
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

program compile;

{$APPTYPE CONSOLE}
{$R version.res}
{$R icon.res}

uses
  SysUtils, Classes, IniFiles,
  uFunc in 'uFunc.pas';

var sr: TSearchRec;
    i: Word;
    cfg: TIniFile;
    outdir: String;
begin
  WriteLn('//SourceMod Batch Compiler');
  WriteLn('// by the SourceMod Dev Team');
  WriteLn;

  if not FileExists(ExtractFilePath(ParamStr(0))+COMPILER_EXE) then
  begin
    WriteLn('// Could not find '+COMPILER_EXE);
    AppExit;
  end;

  cfg := TIniFile.Create(ExtractFilePath(ParamStr(0)) + 'compiler.ini');
  outdir := cfg.ReadString('Main', 'Output', '');
  if (outdir <> '') then begin
    if (DirectoryExists(outdir)) then
      outdir := IncludeTrailingPathDelimiter(outdir)
    else if (DirectoryExists(ExtractFilePath(ParamStr(0)) + outdir)) then
      outdir := ExtractFilePath(ParamStr(0)) + IncludeTrailingPathDelimiter(outdir);
  end
  else
    outdir := '';
  cfg.Free;

  if (outdir = '') and (not DirectoryExists(ExtractFilePath(ParamStr(0))+'compiled')) then
    CreateDir(ExtractFilePath(ParamStr(0))+'compiled');

  if ( ParamCount > 0 ) then
  begin
    for i := 1 to ParamCount do
    begin
      if FileExists(ParamStr(i)) then
        CompilePlugin(ParamStr(i), outdir)
      else
      begin
        WriteLn;
        WriteLn('// File not found.');
      end;
    end;
  end
  else
  begin
    if ( FindFirst('*.sp',faAnyFile,sr) = 0 ) then
    begin
      repeat
        CompilePlugin(sr.Name, outdir);
      until ( FindNext(sr) <> 0 );
    end
    else
    begin
      WriteLn('// No file found.');
    end;
    FindClose(sr);
  end;

  AppExit;
end.
