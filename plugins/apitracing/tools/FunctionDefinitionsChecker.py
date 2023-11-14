import sys
import yaml


def check_parameter_translation(function_definitions_yaml):
    modules_node = function_definitions_yaml["Modules"]
    parameter_types = get_parameter_types(modules_node)
    structures_node = function_definitions_yaml["Structures"]

    struct_names = set()
    for struct in structures_node:
        struct_names.add(struct)

    struct_parameter_types = get_struct_parameter_types(struct_names, structures_node)
    # Replace structs as parameters with the extracted struct parameters
    parameter_types = parameter_types.union(struct_parameter_types) - struct_names

    highlevel_parameter_node = function_definitions_yaml["HighLevelParameterTypes"]
    highlevel_parameter_names, highlevel_parameter_types = get_high_level_parameter_names_and_types(
        highlevel_parameter_node)
    backing_parameter_node = function_definitions_yaml["BackingParameterTypes"]
    backing_parameter_names = get_backing_parameter_names(backing_parameter_node)

    # Sets error default value
    error = find_ring_in_highlevel_parameter_types(highlevel_parameter_node)

    missing_function_parameters = (
        parameter_types - struct_names - highlevel_parameter_names - backing_parameter_names)
    if missing_function_parameters:
        print("Function parameters with missing definitions:", missing_function_parameters)
        error = 1

    missing_struct_parameters = (
        struct_parameter_types - struct_names - highlevel_parameter_names - backing_parameter_names)
    if missing_struct_parameters:
        print("Used struct parameters with missing definitions: ", missing_struct_parameters)
        error = 1

    missing_high_level_parameters = (
        highlevel_parameter_types - highlevel_parameter_names - backing_parameter_names)
    if missing_high_level_parameters:
        print("Highlevel parameter types with missing definitions: ", missing_high_level_parameters)
        error = 1

    multiple_parameter_definitions = highlevel_parameter_names & backing_parameter_names
    if multiple_parameter_definitions:
        print("Parameters which are defined as both a high level parameter and a backing parameter",
              multiple_parameter_definitions)
        error = 1

    multiple_parameter_definitions = (highlevel_parameter_names & struct_names)
    if multiple_parameter_definitions:
        print("Parameters which are defined as both a high level parameter and a struct",
              multiple_parameter_definitions)
        error = 1

    multiple_parameter_definitions = (struct_names & backing_parameter_names)
    if multiple_parameter_definitions:
        print("Parameters which are defined as both a struct and a backing parameter", multiple_parameter_definitions)
        error = 1

    return error


def find_ring_in_highlevel_parameter_types(highlevel_parameter_node):
    error = 0
    for address_width, param_types in highlevel_parameter_node.items():
        for param in param_types.keys():
            visited_high_level_params = set(param)
            current_node = param
            while current_node in param_types.keys():
                current_node = param_types[current_node]
                if current_node in visited_high_level_params:
                    error = 1
                    print("Highlevelparameter resolution for address width:", address_width, "starting from:",
                          param, "contains ring between",
                          param_types[current_node],
                          "and", current_node)
                    break
                visited_high_level_params.add(current_node)
    return error


def get_parameter_types(modules_node):
    parameter_types = set()
    for module_name, function in modules_node.items():
        for function_name, parameters in function.items():
            if "Parameters" in parameters.keys():
                for param_type in parameters["Parameters"].values():
                    parameter_types.add(param_type)
    return parameter_types


def get_struct_parameter_types(struct_names, structures_node):
    struct_parameter_types = set()
    for struct_name, parameters in structures_node.items():
        for parameter_name, parameter_description in parameters.items():
            # Don't add structs as parameters since we already iterate over all structs
            if parameter_description["type"] not in struct_names:
                struct_parameter_types.add(parameter_description["type"])
    return struct_parameter_types


def get_high_level_parameter_names_and_types(highlevel_parameter_node):
    highlevel_parameter_names = set()
    highlevel_parameter_types = set()
    for address_width, param_types in highlevel_parameter_node.items():
        for name, highlevel_param_type in param_types.items():
            highlevel_parameter_names.add(name)
            highlevel_parameter_types.add(highlevel_param_type)
    return highlevel_parameter_names, highlevel_parameter_types


def get_backing_parameter_names(backing_parameter_node):
    backing_parameter_names = set()
    for name, size in backing_parameter_node.items():
        backing_parameter_names.add(name)
    return backing_parameter_names


def main():
    error = 1
    with open("../configuration/functiondefinitions/functionDefinitions.yaml", "r") as stream:
        try:
            function_definitions_yaml = yaml.safe_load(stream)
            error = check_parameter_translation(function_definitions_yaml)
        except yaml.YAMLError as exc:
            print(exc)
    return error


if __name__ == '__main__':
    sys.exit(main())
