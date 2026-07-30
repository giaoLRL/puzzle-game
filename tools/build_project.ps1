$ErrorActionPreference = "Stop"

# ============================================================
#  build_project.ps1 — MSPM0G3507 机械臂项目离线构建脚本
#  参考: C:\Users\PC\Documents\新建的循迹小车\tools\build_project.ps1
# ============================================================

$projectDir  = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildDir    = Join-Path $projectDir "Debug"
$objDir      = Join-Path $buildDir "obj"
$syscfgDir   = Join-Path $buildDir "syscfg"

# --- 工具链路径 ---
$sdkDir      = "C:\TI\mspm0_sdk_2_11_00_07"
$syscfgCli   = "D:\ti\ccs2051\sysconfig_1.26.2\sysconfig_cli.bat"
$compilerDir = "D:\ti\ccs2051\ccs\tools\compiler\ti-cgt-armllvm_4.0.4.LTS"
$compiler    = Join-Path $compilerDir "bin\tiarmclang.exe"

# --- 依赖检查 ---
foreach ($path in @($sdkDir, $syscfgCli, $compiler)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Required build dependency not found: $path"
    }
}

# --- 创建输出目录 ---
New-Item -ItemType Directory -Force -Path $objDir, $syscfgDir | Out-Null

# ============================================================
#  1. SysConfig 生成 ti_msp_dl_config.c/h
# ============================================================
Write-Host "[1/5] Running SysConfig..."
& $syscfgCli `
    -s (Join-Path $sdkDir ".metadata\product.json") `
    --script (Join-Path $projectDir "empty_mspm0g3507.syscfg") `
    -o $syscfgDir `
    --compiler ticlang
if ($LASTEXITCODE -ne 0) {
    throw "SysConfig generation failed with exit code $LASTEXITCODE"
}

# ============================================================
#  2. 编译参数
# ============================================================
$includeArgs = @(
    "-I`"$projectDir`"",
    "-I`"$syscfgDir`"",
    "-I`"$(Join-Path $sdkDir 'source')`"",
    "-I`"$(Join-Path $sdkDir 'source\third_party\CMSIS\Core\Include')`""
)
$compileArgs = @(
    "-c",
    "-march=thumbv6m",
    "-mcpu=cortex-m0plus",
    "-mfloat-abi=soft",
    "-mlittle-endian",
    "-mthumb",
    "-O2",
    "-gdwarf-3",
    "-D__MSPM0G3507__",
    "-D__USE_SYSCONFIG__"
) + $includeArgs

# ============================================================
#  3. 源文件列表
# ============================================================
$sources = @(
    @{ Source = "empty_mspm0g3507.c";                     Object = "empty_mspm0g3507.o" },
    @{ Source = "app\test_grab.c";                        Object = "test_grab.o" },
    @{ Source = "bsp\bsp_timer.c";                        Object = "bsp_timer.o" },
    @{ Source = "bsp\bsp_uart.c";                         Object = "bsp_uart.o" },
    @{ Source = "control\ik.c";                           Object = "ik.o" },
    @{ Source = "control\motion.c";                       Object = "motion.o" },
    @{ Source = "drivers\magnet.c";                       Object = "magnet.o" },
    @{ Source = "drivers\servo.c";                        Object = "servo.o" },
    @{ Source = "protocol\cmd_parser.c";                  Object = "cmd_parser.o" },
    @{ Source = "protocol\ringbuf.c";                     Object = "ringbuf.o" },
    @{ Source = (Join-Path $syscfgDir "ti_msp_dl_config.c"); Object = "ti_msp_dl_config.o" },
    @{
        Source = (Join-Path $sdkDir "source\ti\devices\msp\m0p\startup_system_files\ticlang\startup_mspm0g350x_ticlang.c")
        Object = "startup_mspm0g350x_ticlang.o"
    }
)

# ============================================================
#  4. 编译
# ============================================================
Write-Host "[2/5] Compiling $($sources.Count) source files..."

$objects = foreach ($item in $sources) {
    $source = $item.Source
    if (-not [System.IO.Path]::IsPathRooted($source)) {
        $source = Join-Path $projectDir $source
    }
    $object = Join-Path $objDir $item.Object
    Write-Host "  $($item.Object)"
    & $compiler @compileArgs -o $object $source
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed: $($item.Source)"
    }
    $object
}

# ============================================================
#  5. 链接
# ============================================================
Write-Host "[3/5] Linking..."
$output = Join-Path $buildDir "empty_mspm0g3507.out"
$map    = Join-Path $buildDir "empty_mspm0g3507.map"

$linkArgs = @(
    "-march=thumbv6m",
    "-mcpu=cortex-m0plus",
    "-mfloat-abi=soft",
    "-mlittle-endian",
    "-mthumb",
    "-O2",
    "-gdwarf-3",
    "-Wl,-m`"$map`"",
    "-Wl,-i`"$(Join-Path $sdkDir 'source')`"",
    "-Wl,-i`"$syscfgDir`"",
    "-Wl,-i`"$(Join-Path $compilerDir 'lib')`"",
    "-Wl,--diag_wrap=off",
    "-Wl,--display_error_number",
    "-Wl,--warn_sections",
    "-Wl,--rom_model",
    "-o", "`"$output`""
) + $objects + @(
    "-Wl,-ldevice_linker.cmd",
    "-Wl,-ldevice.cmd.genlibs",
    "-Wl,-llibc.a"
)

& $compiler @linkArgs
if ($LASTEXITCODE -ne 0) {
    throw "Link failed with exit code $LASTEXITCODE"
}

Write-Host "[4/5] Build completed: $output"
Write-Host "[5/5] Size: $((Get-Item $output).Length) bytes"
