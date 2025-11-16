#pragma once
#include "RHIBase.h"
namespace Hazel
{
#if ENABLE_DEBUG_MODE
    #define COMMANDLIST_DEBUG_OUTPUT() do {  \
            printf("RHICommandList[%d]: %s\n", currentCommandIndex, __FUNCTION__);  \
            currentCommandIndex++;  \
        } while(0)  
    #define COMMANDLIST_DEBUG_RESET_INDEX() do {  \
            currentCommandIndex = 0;  \
        } while(0) 
#else 
    #define COMMANDLIST_DEBUG_OUTPUT() do {} while(0)
    #define COMMANDLIST_DEBUG_RESET_INDEX() do {} while(0)
#endif



    /*UE里的Command对象传入的是CommandList对象，Execute时获取CommandList的GetContext的具体执行*/
    /*RHICommand给每个命令套了一个壳子，其实本质还是调用context提供的方法，但是他会组装一个命令需要的参数*/
    typedef struct RHICommand
    {
        RHICommand() = default;
        virtual ~RHICommand() = default;

        virtual void Execute(RHICommandContextRef context) = 0;
    } RHICommand;
    typedef struct RHICommandImmediate
    {
        RHICommandImmediate() = default;
        virtual ~RHICommandImmediate() = default;

        virtual void Execute(RHICommandContextImmediateRef context) = 0;
    } RHICommandImmediate;


    /*RHICommandList是RHICommand的载体，方法通过RHICommandList调用后会根据配置选择是直接执行命令还是缓存命令*/
    class RHICommandList {
        RHICommandList(const CommandListInfo& info) : info(info) {}
        void BeginCommand();
        void EndCommand();



    protected:
        inline void AddCommand(RHICommand* command) { commands.push_back(command); }


    private:
        std::vector<RHICommand*> commands;
        CommandListInfo info;
#if ENABLE_DEBUG_MODE
        int currentCommandIndex = 0;
#endif
    };


    class RHICommandListImmediate
    {
    public:
        RHICommandListImmediate(const CommandListImmediateInfo& info) : info(info) {}

        void Flush(); // 立即执行缓存的API命令

        void GenerateMips(RHITextureRef src);



    protected:
        CommandListImmediateInfo info;

        inline void AddCommand(RHICommandImmediate* command) { commands.push_back(command); }
        std::vector<RHICommandImmediate*> commands;
    };

#define ADD_COMMAND(commandName, ...) do { \
    RHICommand* command = new RHICommand##commandName(__VA_ARGS__);  \
    AddCommand(command); \
} while (0)
#define ADD_COMMAND_IMMEDIATE(commandName, ...) do { \
    RHICommandImmediate* command = new RHICommandImmediate##commandName(__VA_ARGS__);  \
    AddCommand(command); \
} while (0)





    // 各种Command的封装
    struct RHICommandBeginCommand : public RHICommand
    {
        RHICommandBeginCommand() {}

        virtual void Execute(RHICommandContextRef context) override final;
    };
    struct RHICommandEndCommand : public RHICommand
    {
        RHICommandEndCommand() {}

        virtual void Execute(RHICommandContextRef context) override final;
    };


    // 各种CommandImmediate的封装
    struct RHICommandImmediateGenerateMips : public RHICommandImmediate
    {
        RHITextureRef src;

        RHICommandImmediateGenerateMips(RHITextureRef src)
            : src(src)
        {
        }
        virtual void Execute(RHICommandContextImmediateRef context) override final;
    };
}
