Set-Location -LiteralPath $PSScriptRoot

New-Item -ItemType Directory -Force -Path compiled | Out-Null

if ($args.Count -gt 0) {
	$sourceFiles = $args
} else {
	$sourceFiles = Get-ChildItem -Filter "*.sp" -File | ForEach-Object { $_.Name }
}

foreach ($sourceFile in $sourceFiles) {
	$smxFile = $sourceFile -creplace "\.sp$", ".smx"
	$outPath = Join-Path "compiled" $smxFile

	Write-Host "`nCompiling $sourceFile..."
	& .\spcomp.exe $sourceFile -o"$outPath"
}
