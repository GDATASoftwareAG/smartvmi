#pragma once

#include "Common.h"
#include "rapidxml/rapidxml.hpp"
#include <memory>
#include <mutex>

class OutputXML
{
  public:
    OutputXML();

    void addResult(const std::string& processName, int pid, uint64_t baseAddress, const std::vector<Rule>& results);

    [[nodiscard]] std::unique_ptr<std::string> getString() const;

  private:
    rapidxml::xml_document<> doc;
    rapidxml::xml_node<char>* root;
    std::mutex lock{};
};
