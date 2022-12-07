#ifndef APITRACING_MOCK_FUNCTIONDEFINITIONS_H
#define APITRACING_MOCK_FUNCTIONDEFINITIONS_H

#include "../src/lib/config/FunctionDefinitions.h"
#include <gmock/gmock.h>

namespace ApiTracing
{
    class MockFunctionDefinitions : public IFunctionDefinitions
    {
      public:
        MOCK_METHOD(std::shared_ptr<std::vector<ParameterInformation>>,
                    getFunctionParameterDefinitions,
                    (const std::string& moduleName, const std::string& functionName, uint16_t addressWidth),
                    (override));
        MOCK_METHOD(void, init, (), (override));
    };
}
#endif // APITRACING_MOCK_FUNCTIONDEFINITIONS_H
