# Vukan架构

1. RenderPass(不是Vulkan的RenderPass，更抽象的指一个Pass所有的信息)->Pipeline->FBO/Shader->RenderPass

# 已实现功能
1. 导入Vulkan环境
2. 抽象各种Vulkan资源（2025.10.3）
3. 渲染Pass抽象层架构完成，现在上层已经不需要原生Vulkan代码(2025.10.6)

# 项目目标
以Hazel-2D完结的代码为项目起点，打造支持OpenGL/Vulkan的实时渲染器，并支持实施切换，进一步学习其他开源引擎的实现，实现各种渲染效果。
