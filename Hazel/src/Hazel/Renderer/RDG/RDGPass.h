#pragma once
#include <string>
#include <vector>
#include <functional>
#include "RDGResource.h"
namespace GameEngine {

    class RDGPass
    {
    public:
        using ExecuteLambda = std::function<void()>;
        RDGPass(const std::string& name): Name(name){}
        RDGPass(const std::string& InName, RDGPassType InType, ExecuteLambda InExecute)
            : Name(InName), Type(InType), Execute(InExecute)
        {
        }

        std::string Name;
        RDGPassType Type;

        std::vector<RDGResourceRef> ReadResources;

        std::vector<RDGResourceRef> WriteResources;

        ExecuteLambda Execute;
    };

    using RDGPassRef = std::shared_ptr<RDGPass>;

}