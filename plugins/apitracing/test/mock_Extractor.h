#ifndef APITRACING_MOCK_EXTRACTOR_H
#define APITRACING_MOCK_EXTRACTOR_H

#include "../src/lib/os/Extractor.h"
#include <gmock/gmock.h>

namespace ApiTracing
{
    class MockExtractor : public IExtractor
    {
      public:
        MOCK_METHOD(std::vector<ExtractedParameterInformation>,
                    extractParameters,
                    (VmiCore::IInterruptEvent & event,
                     const std::shared_ptr<std::vector<ParameterInformation>>& parametersInformation),
                    (override));
        MOCK_METHOD(std::vector<uint64_t>,
                    getShallowExtractedParams,
                    (VmiCore::IInterruptEvent & event,
                     const std::shared_ptr<std::vector<ParameterInformation>>& parametersInformation),
                    (override));
        MOCK_METHOD(std::vector<ExtractedParameterInformation>,
                    getDeepExtractParameters,
                    (std::vector<uint64_t> shallowParameters,
                     const std::shared_ptr<std::vector<ParameterInformation>>& parametersInformation,
                     uint64_t cr3),
                    (override));
    };
}
#endif // APITRACING_MOCK_EXTRACTOR_H
