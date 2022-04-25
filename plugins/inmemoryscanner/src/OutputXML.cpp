#include "OutputXML.h"
#include "rapidxml/rapidxml_print.hpp"
#include <fstream>

OutputXML::OutputXML()
{
    auto decl = doc.allocate_node(rapidxml::node_declaration);
    decl->append_attribute(doc.allocate_attribute("version", "1.0"));
    doc.append_node(decl);

    root = doc.allocate_node(rapidxml::node_element, "root");
    doc.append_node(root);
}

void OutputXML::addResult(const std::string& processName,
                          int pid,
                          uint64_t baseAddress,
                          const std::vector<Rule>& results)
{
    std::scoped_lock guard(lock);
    auto processNode = doc.allocate_node(rapidxml::node_element, "process");
    root->append_node(processNode);
    processNode->append_attribute(doc.allocate_attribute("name", doc.allocate_string(processName.c_str())));
    processNode->append_attribute(doc.allocate_attribute("pid", doc.allocate_string(std::to_string(pid).c_str())));

    for (const auto& result : results)
    {
        auto ruleNode = doc.allocate_node(rapidxml::node_element, "rule");
        processNode->append_node(ruleNode);
        ruleNode->append_attribute(
            doc.allocate_attribute("namespace", doc.allocate_string(result.ruleNamespace.c_str())));
        ruleNode->append_attribute(doc.allocate_attribute("name", doc.allocate_string(result.ruleName.c_str())));

        for (const auto& match : result.matches)
        {
            auto matchNode = doc.allocate_node(rapidxml::node_element, "match");
            ruleNode->append_node(matchNode);
            matchNode->append_attribute(doc.allocate_attribute("name", doc.allocate_string(match.matchName.c_str())));
            matchNode->append_attribute(doc.allocate_attribute(
                "position", doc.allocate_string(std::to_string(baseAddress + match.position).c_str())));
        }
    }
}

std::unique_ptr<std::string> OutputXML::getString() const
{
    std::stringstream ss;
    ss << doc;
    return std::make_unique<std::string>(ss.str());
}
