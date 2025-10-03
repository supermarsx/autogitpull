<#
Cross-platform PowerShell wrapper for CMake builds on Windows.
Delegates to scripts/build.py when Python is available, otherwise
invokes CMake directly. No custom build logic beyond calling CMake.

Examples:
  pwsh -File scripts/build.ps1
  pwsh -File scripts/build.ps1 -Config Debug -Test
#>

[CmdletBinding()]
param(
  [ValidateSet('Debug','Release','RelWithDebInfo','MinSizeRel')]
  [string]$Config = $env:CONFIG ? $env:CONFIG : 'Release',
  [string]$BuildDir = $env:BUILD_DIR ? $env:BUILD_DIR : 'build',
  [string]$Generator = $env:CMAKE_GENERATOR,
  [int]$Parallel = ($env:PARALLEL ? [int]$env:PARALLEL : 0),
  [string]$Target,
  [switch]$Test,
  [switch]$Install,
  [string]$Prefix = $env:PREFIX
)

$ErrorActionPreference = 'Stop'

function Invoke-Cmd([string[]]$Args) {
  Write-Host "+ $($Args -join ' ')"
  & $Args
  if ($LASTEXITCODE -ne 0) { throw "Command failed: $($Args -join ' ')" }
}

# Prefer Python wrapper
$pyCandidates = @('py -3','python3','python')
$pyExe = $null
foreach ($c in $pyCandidates) {
  try {
    $cmd = $c.Split(' ')[0]
    if (Get-Command $cmd -ErrorAction SilentlyContinue) { $pyExe = $c; break }
  } catch {}
}

if ($pyExe) {
  $scriptPath = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) 'build.py'
  $argsList = @($scriptPath, '--config', $Config, '--build-dir', $BuildDir)
  if ($Generator) { $argsList += @('-G', $Generator) }
  if ($Parallel -gt 0) { $argsList += @('-j', "$Parallel") }
  if ($Target) { $argsList += @('--target', $Target) }
  if ($Test) { $argsList += @('--test') }
  if ($Install) { $argsList += @('--install') }
  if ($Prefix) { $argsList += @('--prefix', $Prefix) }
  Invoke-Cmd @($pyExe) + $argsList
  exit 0
}

# Fallback to CMake directly
$cfgArgs = @('cmake','-S','.', '-B', $BuildDir, "-DCMAKE_BUILD_TYPE=$Config")
if ($Generator) { $cfgArgs += @('-G', $Generator) }
Invoke-Cmd $cfgArgs

$buildArgs = @('cmake','--build', $BuildDir, '--config', $Config)
if ($Parallel -gt 0) { $buildArgs += @('-j', "$Parallel") }
if ($Target) { $buildArgs += @('--target', $Target) }
Invoke-Cmd $buildArgs

if ($Install) {
  $instArgs = @('cmake','--install', $BuildDir)
  if ($Prefix) { $instArgs += @('--prefix', $Prefix) }
  Invoke-Cmd $instArgs
}

if ($Test) {
  $ctest = (Get-Command ctest -ErrorAction SilentlyContinue)
  $ctestCmd = if ($ctest) { 'ctest' } else { 'ctest' }
  Invoke-Cmd @($ctestCmd,'--test-dir', $BuildDir, '--output-on-failure','-C', $Config)
}

