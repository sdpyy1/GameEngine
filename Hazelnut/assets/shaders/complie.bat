@echo off
:: ==============================================
:: 自动编译当前目录下的所有 .glsl 着色器文件
:: 生成的 spv 文件放到 spv 文件夹中
:: 需要安装 VulkanSDK 并配置正确路径
:: ==============================================

set GLSLC="D:\context\VulkanSDK\1.3.243.0\Bin\glslc.exe"

if not exist spv (
    mkdir spv
)

echo ==============================================
echo [Shader Compilation Started]
echo ==============================================

for %%f in (*.glsl) do (
    echo Compiling %%f ...
    %GLSLC% -fshader-stage=vert %%f -DVERTEX_SHADER -o spv/%%~nfVert.spv
    %GLSLC% -fshader-stage=frag %%f -DFRAGMENT_SHADER -o spv/%%~nfFrag.spv
)

echo ==============================================
echo [All shaders compiled successfully!]
echo Output folder: spv\
echo ==============================================

pause
