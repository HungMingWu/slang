#include "json.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "slang/symbols/ASTVisitor.h"

namespace {
struct PortInfo {
    std::string direction;
    std::string name;
    std::string range;
};

struct ParameterInfo {
    std::string name;
    std::string value;
};

struct ModuleInfo {
    std::string hierarchy;
    std::string module_name;
    std::string original_module_name;
    std::string port_prefix;
};

template<typename T>
using StrHashMap = std::unordered_map<std::string, T>;

struct State1 {
    std::vector<PortInfo> ports;
    std::vector<ParameterInfo> parameters;
    std::vector<ModuleInfo> sub_modules;
};

template <typename T>
std::string getHierarchicalPath(const T& symbol) {
    std::string hier;
    symbol.getHierarchicalPath(hier);
    return hier;
}

std::string getParentHierarchicalPath(std::string path)
{
    auto last = path.find_last_of('.');
    return (last == std::string::npos) ? "" : path.substr(0, last);
}

std::vector<std::string> getSplitArray(std::string str, std::string pattern) {
    std::vector<std::string> result;
    std::string::size_type begin, end;
    end = str.find(pattern);
    begin = 0;

    while (end != std::string::npos) {
        if (end - begin != 0) {
            result.push_back(str.substr(begin, end - begin));
        }
        begin = end + pattern.size();
        end = str.find(pattern, begin);
    }

    if (begin != str.length()) {
        result.push_back(str.substr(begin));
    }
    return result;
}

std::string transformString(slang::ArgumentDirection direction) {
    switch (direction) {
        case slang::ArgumentDirection::In:
            return "input";
        case slang::ArgumentDirection::Out:
            return "output";
        case slang::ArgumentDirection::InOut:
            return "inout";
        default:
            return "ref";
    }
}

std::ostream& operator<<(std::ostream& os, const std::vector<PortInfo>& vec) {
    bool first = true;
    for (const auto& v : vec) {
        if (!first)
            os << ",\n";
        os << v.direction << " logic" << v.range << " " << v.name;
        first = false;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<ParameterInfo>& vec) {
    bool first = true;
    for (const auto& parameter : vec) {
        if (!first)
            os << ",";
        os << "." << parameter.name << "(" << parameter.value << ")";
        first = false;
    }
    return os;
}

struct MyVisitor : public slang::ASTVisitor<MyVisitor, false, false> {
    std::size_t ref_module_count{};
    std::unordered_set<std::string> job_done_set;
    StrHashMap<std::vector<std::string>> module_node_vecs;
    StrHashMap<State1> node_hier_state;
    std::vector<std::string> duplicate_node;
    std::unordered_map<std::string, std::string> node_candidate_module;
    StrHashMap<std::unordered_set<std::string>> clockSet;
    std::unordered_map<std::string, std::string> node_candidate;

    void createCustoEmitVDone(const std::vector<std::string> &not_found_node,
                              const std::vector<std::string> &module_missmatch_node) {
        std::string filename;
        if (duplicate_node.empty() && not_found_node.empty() &&
            module_missmatch_node.empty()) {
            filename = ".CustomEmitVDone"; /*(v3Global.opt.makeDir() + "/.CustomEmitVDone")*/
            std::ofstream out(filename);
            out << "CustomEmitV Done.";
        }
        else {
            filename = "/.CustomEmitVError"; /*(v3Global.opt.makeDir() + "/.CustomEmitVError")*/
            std::ofstream out(filename);
            out << "duplicate hierarchy:\n";
            for (const auto& dup : duplicate_node)
                out << dup << "\n";
            out << "hierarchy not found:\n";
            for (const auto& node : not_found_node)
                out << node << "\n";
            out << "wrong module:\n";
            for (const auto& node : module_missmatch_node)
                out << node << "\n";
        }
    }

    void createFacingNodeData(StrHashMap<std::string> node_candidate) {
        nlohmann::json obj;
        StrHashMap<std::string> prefix_map;
        for (const auto& [moduleHierarchy, state] : node_hier_state) {
            if (!moduleHierarchy.empty()) {
                nlohmann::json obj;
                if (node_candidate.count(moduleHierarchy) == 0)
                    continue;
                auto moduleNameInfo = getSplitArray(node_candidate[moduleHierarchy], ":");
                obj["moduleName"] = moduleNameInfo[0];
                obj["oringnalModuleName"] = moduleNameInfo[1];
                for (const auto& v : state.sub_modules) {
                    nlohmann::json subobj;
                    subobj["moduleName"] = v.module_name;
                    subobj["originalModuleName"] = v.original_module_name;
                    subobj["hierarchy"] = v.hierarchy;
                    std::string hierarchy_left = v.hierarchy.substr(moduleHierarchy.length() + 1);
                    if (prefix_map.find(hierarchy_left) == prefix_map.end()) {
                        subobj["portPrefix"] = v.port_prefix;
                        prefix_map[hierarchy_left] = v.port_prefix;
                    }
                    else {
                        subobj["portPrefix"] = prefix_map[hierarchy_left];
                    }
                    obj["facingNode"].push_back(subobj);
                }
                obj[moduleHierarchy] = obj;
            }
        }
        std::string filename =
            "FacingNodeData"; // /*(v3Global.opt.makeDir() + "/FacingNodeData"; */
        std::ofstream out(filename);
        out << obj.dump();
    }

    void createModuleData(StrHashMap<std::string> node_candidate) {
        nlohmann::json obj;

        for (const auto& [first, state] : node_hier_state) {
            nlohmann::json subobj;
            std::cout << "first: " << first << "\n";
            if (node_candidate.count(first) == 0)
                continue;
            auto moduleNameInfo = getSplitArray(node_candidate[first], ":");
            subobj["moduleName"] = moduleNameInfo[0];
            subobj["oringnalModuleName"] =
                (moduleNameInfo.size() > 1) ? moduleNameInfo[1] : moduleNameInfo[0];
            for (const auto& v : state.ports) {
                nlohmann::json portInfo;
                portInfo["direction"] = v.direction;
                portInfo["name"] = v.name;
                portInfo["range"] = v.range;
                subobj["modulePort"].push_back(portInfo);
            }
            obj[first] = subobj;
        }
        std::string filename = "ModuleData"; // /* (v3Global.opt.makeDir() + "/ModuleData")*/
        std::ofstream out(filename);
        out << obj.dump();
    }

    std::string gen_top_sv(std::string node_hier, std::string module_name) {
        std::stringstream os;
        std::string dlmtr = "";
        std::vector<std::string> assignVec;
        StrHashMap<std::string> sub_node_set;
        // module port & save reference module port's assignment
        os << "module " << module_name << "_top" << "(\n";
        os << node_hier_state[node_hier].ports;
        for (const auto& candidate_hier : module_node_vecs[module_name]) {
            for (auto& module_info : node_hier_state[candidate_hier].sub_modules) {
                auto assign_port_name_prefix = module_info.port_prefix;
                auto sub_node_hier = module_info.hierarchy;
                auto sub_node_hier_str = sub_node_hier.substr(candidate_hier.length() + 1);
                auto moudle_hier_prefix =
                    module_name + "_top." + module_name + "." + sub_node_hier_str + ".";
                auto it = sub_node_set.find(sub_node_hier_str);
                if (it == end(sub_node_set)) {
                    sub_node_set.insert({ sub_node_hier_str, assign_port_name_prefix });
                    for (const auto& port_info : node_hier_state[sub_node_hier].ports) {
                        std::string assign_string;
                        auto& clock_set = clockSet[sub_node_hier];
                        if (clock_set.count(port_info.name) == 0) {
                            if (port_info.direction.compare("inout") != 0) {
                                auto make_assign_string =
                                    [](const std::string& first, const std::string& last,
                                       const std::string& name) -> std::string {
                                    return std::string("assign ") + first + name + " = " + last +
                                           name;
                                };
                                if (port_info.direction.compare("input") == 0) {
                                    os << dlmtr << "\n"
                                       << "output";
                                    assign_string =
                                        make_assign_string(assign_port_name_prefix,
                                                           moudle_hier_prefix, port_info.name);
                                }
                                else {
                                    os << dlmtr << "\n"
                                       << "input";
                                    assign_string =
                                        make_assign_string(moudle_hier_prefix,
                                                           assign_port_name_prefix, port_info.name);
                                }
                                os << " logic" << port_info.range;
                                os << " " << assign_port_name_prefix << port_info.name;
                                assignVec.push_back(assign_string);
                                dlmtr = ",";
                            }
                        }
                    }
                }
                else {
                    module_info.port_prefix = sub_node_set[sub_node_hier_str];
                }
            }
        }
        os << ");\n\n";
        // original module instance
        os << module_name << " ";
        // module parameter
        if (!node_hier_state[node_hier].parameters.empty()) {
            os << "#(" << node_hier_state[node_hier].parameters << ")";
        }

        os << module_name << "(\n";
        dlmtr = "";
        for (const auto& port_info : node_hier_state[node_hier].ports) {
            os << dlmtr << "\n." << port_info.name << "(" << port_info.name << ")";
            dlmtr = ",";
        }

        os << ");\n\n";

        // new port's assignment
        for (const auto& assign : assignVec)
            os << assign << ";\n";
        os << "endmodule";
        return os.str();
    }

    std::string gen_empty_sv(std::string node_hier, std::string module_name) {
        std::stringstream os;
        std::string dlmtr = "";
        os << "module " << module_name;
        dlmtr = "";
        if (!node_hier_state[node_hier].parameters.empty()) {
            os << "#(\n";
            for (const auto& v : node_hier_state[node_hier].parameters) {
                os << dlmtr << "\nparameter " << v.name << "=" << v.value;
                dlmtr = ",";
            }
            os << ")\n";
        }

        if (!node_hier_state[node_hier].ports.empty()) {
            os << "(\n" << node_hier_state[node_hier].ports << ");\n";
        }
        os << "endmodule";
        return os.str();
    }
    void handle(const slang::RootSymbol& symbol) {
        std::cout << "ROOT:" << symbol.name << std::endl;
        visitDefault(symbol);
    }

    void handle(const slang::InstanceSymbol& symbol) {
        std::cout << "INSTANCE:" << symbol.name << ", define:" << symbol.getDefinition().name
                  << std::endl;
        std::string current_node_hier = getHierarchicalPath(symbol);
        if (job_done_set.count(current_node_hier) == 0) {
            std::string current_module_name(symbol.getDefinition().name);
            std::string current_module_oringinal_name =
                current_module_name; // Assume it equals right now
            if (node_candidate.count(current_node_hier) != 0) {
                node_candidate[current_node_hier] =
                    current_module_name + ":" + current_module_oringinal_name;

                module_node_vecs[current_module_name].push_back(current_node_hier);

                ModuleInfo sub_module;
                sub_module.hierarchy = current_node_hier;
                sub_module.module_name = current_module_name;
                sub_module.original_module_name = current_module_oringinal_name;
                sub_module.port_prefix = "Sss" + std::to_string(ref_module_count++) + "_";

                std::string pre_hier = getParentHierarchicalPath(current_node_hier);
                node_hier_state[pre_hier].sub_modules.push_back(sub_module);
            }
            job_done_set.insert(current_node_hier);
        }

        std::cout << "    hierarchical:" << current_node_hier << std::endl;
        visit(symbol.body);
    }
    void handle(const slang::PortSymbol& symbol) {
        std::cout << "    Name:" << symbol.name << ", kind:" << toString(symbol.kind)
                  << ", direct:" << toString(symbol.direction)
                  << ", type:" << symbol.getType().toString() << std::endl;
        PortInfo info;
        info.direction = transformString(symbol.direction);
        info.name = symbol.name;
        info.range =
            (symbol.getType().hasFixedRange() && symbol.getType().getFixedRange().width() > 1)
                ? symbol.getType().getFixedRange().toString()
                : "";
        node_hier_state[getParentHierarchicalPath(getHierarchicalPath(symbol))].ports.push_back(info);
        visitDefault(symbol);
    }
    void handle(const slang::ParameterSymbol& symbol) {
        std::cout << "    Name:" << symbol.name << ", kind:" << toString(symbol.kind)
                  << ", value:" << symbol.getValue() << std::endl;
        ParameterInfo info1;
        info1.name = symbol.name;
        info1.value = symbol.getValue().toString();
        node_hier_state[getParentHierarchicalPath(getHierarchicalPath(symbol))]
            .parameters.push_back(info1);
        visitDefault(symbol);
    }

    template<typename T>
    void handle(const T& symbol) {
        visitDefault(symbol);
    }
    void generate() {
        std::vector<std::string> not_found_node;
        std::vector<std::string> module_missmatch_node;
        for (const auto& [name, vecs] : module_node_vecs) {
            if (!vecs.empty()) {
                // top
                {
                    std::ofstream out(name + "_top.sv");
                    std::string a1 = gen_top_sv(vecs[0], name);
                    out << a1;
                }

                // dummy
                {
                    std::ofstream out(name + ".sv");
                    out << gen_empty_sv(vecs[0], name);
                }
            }
        }
        for (auto it : node_candidate) {
            if (it.second.compare("") == 0) {
                not_found_node.push_back(it.first);
            }
            else {
                auto moduleNameInfo = getSplitArray(it.second, ":");
                if (node_candidate_module[it.first].compare(moduleNameInfo[1]) != 0) {
                    module_missmatch_node.push_back(it.first);
                }
            }
        }

        createFacingNodeData(node_candidate);
        createModuleData(node_candidate);
        createCustoEmitVDone(not_found_node, module_missmatch_node);
    }
};
} // namespace

static std::string replaceQuestionMask(const std::string& input) {
    auto replace_string = input[0] == '?' ? input.substr(1) : input;
    std::size_t found = replace_string.find("?");
    return found == std::string::npos ? replace_string : replace_string.replace(found, 1, "'");
}

namespace verilog {
void emit2(const slang::RootSymbol& root, std::ifstream& input) {

    MyVisitor visitor;
    std::string line_text;
	while (getline(input, line_text)) {
        auto nodeInfo = getSplitArray(line_text, ";");
        const auto& master = nodeInfo[0];
        // node hier
        if (visitor.node_candidate.count(master) == 0) {
            visitor.node_candidate[master] = "";
            visitor.node_candidate_module[master] = nodeInfo[1];
            for (int i = 2; i < nodeInfo.size(); ++i) {
                // clockSet initiall
                visitor.clockSet[master].insert(nodeInfo[i]);
            }
        }
        else {
            visitor.duplicate_node.push_back(master);
        }
    }

    root.visit(visitor);
    visitor.generate();
}
} // namespace verilog
