Get-ChildItem -Path .\src, .\include, .\tests, .\unittest -Include *.hpp, *.cpp -Recurse |
ForEach-Object {
    Write-Output $_.FullName
    &clang-format -i -style=file $_.FullName
}
