# Testing Scripts

These are some scripts to automate using psychonic's fork of the gamedata checker tool.
Requires Lua 5.1 to parse results.

### Outputs

- The raw GDC logs: `BASE_PATH/scripts/test/logs`
- The game binaries (if enabled): `BASE_PATH/scripts/bin`
- Some temporary files used for parsing: `CURRENT_DIRECTORY/output.temp` and `CURRENT_DIRECTORY/exporter.temp`

### Game Script Usage

> **Warning**: Remember to set `BASE_PATH` or be in the correct directory!
> 
> If you do not wish to set `$BASE_PATH` as an argument, make sure your current directory is `sourcemod`, and it will be selected as a default. For example:
` C:\Repo\Sourcemod> scripts/test/csgo.ps1`
>
> If sourcemod is not your current directory, you will need to set it as an argument: `scripts/test/csgo.ps1 -BASE_PATH:"C:/Repo/Sourcemod"`

- (flag) `SUPPRESS_BUILD`:
  Whether to enable `--quiet` on the docker build
- (flag) `EXPORT_BINARIES`:
  Whether to export game binaries into `scripts/test/bin` for analysis
- `BASE_PATH`:
  The path to `sourcemod` in the github repo. Defaults to working directory.
### `test.ps1` Usage

See the existing same-specific scripts like `tf2.ps1` or `csgo.ps1`.

- (flag) `SUPPRESS_BUILD`:
  Whether to enable `--quiet` on the docker build
- (flag) `EXPORT_BINARIES`:
  Whether to export game binaries into `scripts/test/bin` for analysis
- `APP_ID_ROOT`:
  The app ID to use to check the version. Usually the game client's app ID
- `APP_ID`:
  The app ID of the SRCDS server to download.
- `APP_SM_ENGINE`:
  The engine name used by sourcemod in file names. Eg, left4dead2's is `l4d2` because that's what is used in the gamedata file **names**.
- `APP_ENGINE`:
  The engine name. Example: `left4dead2`, `csgo`
- `BASE_PATH`:
  The path to `sourcemod` in the github repo. Defaults to working directory.
- `BINARY_APPEND`:
  Some linux binaries will have `_srv` appended to the end of them in the depots. Set this to "_srv" to account for that.

### Dependencies

- **PowerShell**
- **Lua** (5.1 works)
- **Docker**

### Architecture

The scripts are mostly around the dockerfile, which:
- Builds GDC from source
- Builds DepotDownloader from source
- Downloads the requested depot
- Prepares everything to quickly run gdc on server binaries with `docker run`.

The dockerfile also caches game files, so docker will automatically skip downloading a new version of the binaries until a new version has come out (note-it is reliant on the script feeding it a version number to do this)