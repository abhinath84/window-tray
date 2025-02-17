#pragma once
#include <napi.h>
#include <vector>
#include <string>

namespace Napi
{
    namespace details
    {
        template <>
        struct vf_fallback<std::vector<std::uint32_t>>
        {
            static Value From(napi_env env, const std::vector<std::uint32_t> &value)
            {
                Array object = Array::New(env, value.size());
                for (auto i = 0; i != value.size(); ++i)
                {
                    object.Set(i, Number::New(env, value[i]));
                }
                return object;
            }
        };
    }
}


struct NodeEventCallback
{
    NodeEventCallback(Napi::Env e, Napi::FunctionReference&& function, Napi::ObjectReference&& object)
        : env(e), callback(std::move(function)), receiver(std::move(object))
    {

    }

    Napi::Env env;

    Napi::FunctionReference callback;
    Napi::ObjectReference receiver;
};



#define NAPI_METHOD_INSTANCE(Class, Method) \
    InstanceMethod(#Method, &Class::Method)
