@echo off
:: ==============================================
:: �Զ����뵱ǰĿ¼�µ����� .glsl ��ɫ���ļ�
:: ���ɵ� spv �ļ��ŵ� spv �ļ�����
:: ��Ҫ��װ VulkanSDK ��������ȷ·��
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
