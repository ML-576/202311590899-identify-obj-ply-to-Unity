# gen_test_models.ps1 —— 生成测试模型 (新建文件, 2026)
# 生成: cube.obj + cube.mtl + texture.png (OBJ路线)
#       cube_ascii.ply / cube_bin.ply (带顶点颜色) / quad.ply (多边形面)
#       sphere_big.ply (约1.6万三角形, 大模型性能测试)
$ErrorActionPreference = "Stop"
$dir = Join-Path $PSScriptRoot "UnityAssets\StreamingAssets\Models"
New-Item -ItemType Directory -Force -Path $dir | Out-Null

# ---------- 1. cube.obj ----------
$obj = @"
mtllib cube.mtl
o TestCube
v 0.000000 0.000000 1.000000
v 1.000000 0.000000 1.000000
v 1.000000 1.000000 1.000000
v 0.000000 1.000000 1.000000
v 0.000000 0.000000 -1.000000
v 1.000000 0.000000 -1.000000
v 1.000000 1.000000 -1.000000
v 0.000000 1.000000 -1.000000
vt 0.000000 0.000000
vt 1.000000 0.000000
vt 1.000000 1.000000
vt 0.000000 1.000000
vn 0.000000 0.000000 1.000000
vn 0.000000 0.000000 -1.000000
vn 1.000000 0.000000 0.000000
vn -1.000000 0.000000 0.000000
vn 0.000000 1.000000 0.000000
vn 0.000000 -1.000000 0.000000
g Front
usemtl Red
f 1/1/1 2/2/1 3/3/1 4/4/1
g Back
usemtl Blue
f 6/1/2 5/2/2 8/3/2 7/4/2
g Right
usemtl Red
f 2/1/3 6/2/3 7/3/3 3/4/3
g Left
usemtl Blue
f 5/1/4 1/2/4 4/3/4 8/4/4
g Top
usemtl Blue
f 4/1/5 3/2/5 7/3/5 8/4/5
g Bottom
usemtl Red
f 1/1/6 5/2/6 6/3/6 2/4/6
"@
[System.IO.File]::WriteAllText((Join-Path $dir "cube.obj"), $obj.Replace("`n","`r`n"))

# ---------- 2. cube.mtl ----------
$mtl = @"
newmtl Red
Ka 0.2 0.05 0.05
Kd 0.9 0.25 0.25
Ks 0.4 0.4 0.4
Ns 60.0
map_Kd texture.png

newmtl Blue
Ka 0.05 0.05 0.2
Kd 0.25 0.35 0.9
Ks 0.4 0.4 0.4
Ns 60.0
map_Kd texture.png
"@
[System.IO.File]::WriteAllText((Join-Path $dir "cube.mtl"), $mtl.Replace("`n","`r`n"))

# ---------- 3. texture.png (System.Drawing 生成 64x64 棋盘格) ----------
Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap 64,64
for ($y=0; $y -lt 64; $y++) {
    for ($x=0; $x -lt 64; $x++) {
        $on = (([int]($x/8) + [int]($y/8)) % 2) -eq 0
        $c = if ($on) { [System.Drawing.Color]::White } else { [System.Drawing.Color]::FromArgb(255,160,60,200) }
        $bmp.SetPixel($x,$y,$c)
    }
}
$bmp.Save((Join-Path $dir "texture.png"), [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

# ---------- 4. cube_ascii.ply ----------
$verts = @(
    @(-1,-1, 1),@(1,-1, 1),@(1, 1, 1),@(-1, 1, 1),
    @(-1,-1,-1),@(1,-1,-1),@(1, 1,-1),@(-1, 1,-1))
$tris = @(
    @(0,2,1),@(0,3,2), @(4,5,6),@(4,6,7), @(1,2,6),@(1,6,5),
    @(0,7,3),@(0,4,7), @(3,6,2),@(3,7,6), @(0,1,5),@(0,5,4))
$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("ply"); $lines.Add("format ascii 1.0")
$lines.Add("comment generated test cube")
$lines.Add("element vertex 8")
foreach ($p in "x","y","z") { $lines.Add("property float $p") }
$lines.Add("element face 12")
$lines.Add("property list uchar int vertex_indices")
$lines.Add("end_header")
foreach ($v in $verts) { $lines.Add(("{0} {1} {2}" -f $v[0],$v[1],$v[2])) }
foreach ($t in $tris)  { $lines.Add(("3 {0} {1} {2}" -f $t[0],$t[1],$t[2])) }
[System.IO.File]::WriteAllLines((Join-Path $dir "cube_ascii.ply"), $lines)

# ---------- 5. cube_bin.ply (binary_little_endian, 带顶点颜色) ----------
$ms = New-Object System.IO.MemoryStream
$bw = New-Object System.IO.BinaryWriter($ms)
$enc = [System.Text.Encoding]::ASCII
$hdr = "ply`nformat binary_little_endian 1.0`ncomment colored cube`nelement vertex 8`nproperty float x`nproperty float y`nproperty float z`nproperty uchar red`nproperty uchar green`nproperty uchar blue`nelement face 12`nproperty list uchar int vertex_indices`nend_header`n"
$bw.Write($enc.GetBytes($hdr))
for ($i=0; $i -lt 8; $i++) {
    $bw.Write([single]$verts[$i][0]); $bw.Write([single]$verts[$i][1]); $bw.Write([single]$verts[$i][2])
    $bw.Write([byte](255 - $i*20)); $bw.Write([byte](60 + $i*15)); $bw.Write([byte](80 + $i*20))
}
foreach ($t in $tris) {
    $bw.Write([byte]3)
    foreach ($i in $t) { $bw.Write([int]$i) }
}
$bw.Flush()
[System.IO.File]::WriteAllBytes((Join-Path $dir "cube_bin.ply"), $ms.ToArray())
$bw.Close()

# ---------- 6. quad.ply (两个四边形面, 验证三角化, 带法线) ----------
$qv = @(@(-1,-1,0),@(1,-1,0),@(1,1,0),@(-1,1,0),@(0,-1,1),@(0,1,1))
$qf = @(@(0,1,2,3), @(0,1,5,4))   # 四边形面
$lines2 = New-Object System.Collections.Generic.List[string]
$lines2.Add("ply"); $lines2.Add("format ascii 1.0")
$lines2.Add("element vertex 6")
foreach ($p in "x","y","z","nx","ny","nz") { $lines2.Add("property float $p") }
$lines2.Add("element face 2")
$lines2.Add("property list uchar int vertex_indices")
$lines2.Add("end_header")
$nz=@(0,0,1)
foreach ($v in $qv) { $lines2.Add(("{0} {1} {2} {3} {4} {5}" -f $v[0],$v[1],$v[2],$nz[0],$nz[1],$nz[2])) }
foreach ($f in $qf) { $lines2.Add(("4 {0} {1} {2} {3}" -f $f[0],$f[1],$f[2],$f[3])) }
[System.IO.File]::WriteAllLines((Join-Path $dir "quad.ply"), $lines2)

# ---------- 7. sphere_big.ply (binary_little_endian, 大模型) ----------
$seg = 128; $rings = 64
$nv = ($rings-1)*$seg + 2
$nf = 2*$seg*($rings-2) + 2*$seg   # 球面三角形数(侧面2*seg*(rings-2) + 上下帽各seg)
$ms2 = New-Object System.IO.MemoryStream
$bw2 = New-Object System.IO.BinaryWriter($ms2)
$hdr2 = "ply`nformat binary_little_endian 1.0`ncomment big sphere for perf test`nelement vertex $nv`nproperty float x`nproperty float y`nproperty float z`nelement face $nf`nproperty list uchar int vertex_indices`nend_header`n"
$bw2.Write($enc.GetBytes($hdr2))
# 顶点
for ($r=1; $r -lt $rings; $r++) {
    $phi = [math]::PI * $r / $rings
    for ($s=0; $s -lt $seg; $s++) {
        $th = 2*[math]::PI * $s / $seg
        $bw2.Write([single]([math]::Sin($phi)*[math]::Cos($th)))
        $bw2.Write([single]([math]::Cos($phi)))
        $bw2.Write([single]([math]::Sin($phi)*[math]::Sin($th)))
    }
}
$bw2.Write([single]0.0); $bw2.Write([single]1.0); $bw2.Write([single]0.0)   # 顶极点
$bw2.Write([single]0.0); $bw2.Write([single]-1.0); $bw2.Write([single]0.0)  # 底极点
# 面
function W3($bw2,$a,$b,$c) { $bw2.Write([byte]3); $bw2.Write([int]$a); $bw2.Write([int]$b); $bw2.Write([int]$c) }
for ($r=0; $r -lt $rings-2; $r++) {
    for ($s=0; $s -lt $seg; $s++) {
        $s2 = ($s+1) % $seg
        $a = $r*$seg+$s; $b = $r*$seg+$s2; $c = ($r+1)*$seg+$s; $d = ($r+1)*$seg+$s2
        W3 $bw2 $a $b $d; W3 $bw2 $a $d $c
    }
}
$top = $nv-2; $bot = $nv-1; $lastr = ($rings-2)*$seg
for ($s=0; $s -lt $seg; $s++) {
    $s2 = ($s+1) % $seg
    W3 $bw2 $top ($s) ($s2)            # 顶帽
    W3 $bw2 $bot ($lastr+$s2) ($lastr+$s)  # 底帽
}
$bw2.Flush()
[System.IO.File]::WriteAllBytes((Join-Path $dir "sphere_big.ply"), $ms2.ToArray())
$bw2.Close()

Write-Host "生成完成 -> $dir"
Get-ChildItem $dir | Select-Object Name, @{n='KB';e={[math]::Round($_.Length/1KB,1)}}
