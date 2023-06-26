# Plugin Development

This readme is intended to help developers get started with developing new plugins for *VMICore*.
Since these plugins are shared object files that are loaded dynamically at runtime it is possible to develop them
separately without having to fork this repository. Nevertheless, pull request that add new plugins are welcome.

## Prerequisites

The use of *CMake* as the build tool is recommended because it makes integration with the *VMICore* project easier. For
example, there is a *CMake* target containing public header files that is specifically intended to be used by plugins.
Furthermore, the code snippets in this guide focus solely on *CMake*.

Unfortunately, it is not possible to use *C* for developing plugins since the plugin interface uses *C++* types and
language features extensively. However, it might be possible to generate *Rust* language bindings und develop plugins
with *Rust* instead, but that will be the subject of future work.

## VMICore headers

In order to get started it is necessary to gain access to *VMICore*'s public headers. With *CMake* this can either be
done via a call to `add_subdirectory()` (recommended if working in a single repository)

```cmake
# Assuming the project resides in the plugins folder
set(VMICORE_DIRECTORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../vmicore")

add_subdirectory("${VMICORE_DIRECTORY_ROOT}/src/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-headers")
```

or via pulling in *VMICore* as a dependency (recommended if developing in a separate repository).

```cmake
include(FetchContent)
FetchContent_Declare(
        vmicore
        GIT_REPOSITORY https://github.com/GDATASoftwareAG/smartvmi.git
        GIT_TAG main
        SOURCE_SUBDIR vmicore/src/include/
)
FetchContent_MakeAvailable(vmicore)
```

Finally, the `vmicore-public-headers` library target has to be added to the link libraries of the plugin target.

```cmake
add_library(myplugin MODULE "src/myplugin.cpp")
target_link_libraries(myplugin vmicore-public-headers)
```

## Plugin Initialization

In order to get started with developing a new plugin, the plugin init function has to be implemented. It is declared
in `IPlugin.h` and returns the main plugin object. The plugin object has to inherit from `IPlugin`. Furthermore,
the `VMI_PLUGIN_API_VERSION_INFO` macro has to be used in one of the compilation units. For more information,
have a look at the [VMI_PLUGIN_API_VERSION_INFO Macro](#vmipluginapiversioninfo-macro) subsection below.

```c++
#include <vmicore/plugins/IPlugin.h>

VMI_PLUGIN_API_VERSION_INFO

std::unique_ptr<IPlugin>
VmiCore::Plugin::init(PluginInterface* pluginInterface,
                      std::shared_ptr<IPluginConfig> config,
                      std::vector<std::string> args)
{
    return std::make_unique<SomePlugin>(pluginInterface, config, args);
}
```

### Init Function

This function will be called by *VMICore* during plugin initialization. Returns the main plugin object.

### IPlugin

An interface from which all plugins must inherit. Offers an `unload()` function which is called right before the plugin
is destructed.

### PluginInterface

This is the main interface for calling any functionality provided by *VMICore*.

### VMI_PLUGIN_API_VERSION_INFO Macro

This macro initializes a global extern variable with the current API version of the `PluginInterface`. It will be
checked by *VMICore* before the `init()` function is called in order to verify the compatibility of the plugin with
*VMICore*. If this variable is absent or its value is different from the one defined in *VMICore* the plugin will not be
initialized.

### Configuration parsing

The `IPluginConfig` object offers multiple ways of accessing plugin specific configuration options.
The most convenient way of parsing the configuration is by using the [yaml-cpp](https://github.com/jbeder/yaml-cpp)
project. Since *VMICore* uses the same project it allows the plugin to work with the already parsed YAML objects
directly instead of having to parse the configuration twice. As a drawback, the plugin developer has to make sure that
the same *yaml-cpp* version as in *VMICore* is used in order to avoid unexpected behavior.

```cmake
FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.7.0
)
option(YAML_BUILD_SHARED_LIBS "" OFF)  # Only build static library
option(YAML_CPP_BUILD_TOOLS "" OFF)  # Avoid building unnecessary target
FetchContent_MakeAvailable(yaml-cpp)
# Required because libraries have to be relocatable and can therefore only link with relocatable code
set_property(TARGET yaml-cpp PROPERTY POSITION_INDEPENDENT_CODE TRUE)
# Make the function that provides the plugin root node in IPluginConfig available 
add_compile_definitions(YAML_CPP_SUPPORT)
```

```c++
#include <vmicore/plugins/IPlugin.h>

VMI_PLUGIN_API_VERSION_INFO

std::unique_ptr<IPlugin>
VmiCore::Plugin::init(PluginInterface* pluginInterface,
                      std::shared_ptr<IPluginConfig> config,
                      std::vector<std::string> args)
{
    auto pluginRootNode = config.rootNode();
    auto someValue = pluginRootNode["some_key"].as<std::string>();
    return std::make_unique<SomePlugin>(pluginInterface, config, args);
}
```

`IPluginConfig` also offers to provide the configuration as a string, so it can be read by any yaml parser.
As a third option, it is possible to obtain the path of an external config file via `configFilePath()`. This is the
recommended way for large configurations in order to keep memory consumption to a minimum. Furthermore, it allows for
choosing the config file format freely. For this option, only the configuration file path has to be specified in the
main config file.

```yaml
plugin_system:
  directory: /usr/local/lib/
  plugins:
    libmyplugin.so:
      config_file: /path/to/config/file
```

### Commandline Arguments

It is possible to pass commandline arguments to plugins:

```shell
$ vmicore -p 'myplugin: --arg1 --arg2 arg2_value'
```

Remarks:

- Note that the name of the plugin is the name of the shared object file without the `lib` prefix and the file
  extension. Conversely, all plugins are expected to adhere to the standard naming convention for shared libraries on
  *Linux*.
- Make sure your plugin arguments are interpreted as a single parameter.
- It is not possible to pass arguments multiple times to the same plugin.
  (e.g. `-p 'myplugin:--arg1' -p 'myplugin:--arg2'`)

Any given arguments are passed to the `init()` function as a vector of strings. It is recommended to use an argument
parser like *TCLAP* for parsing as the arguments are already in a correct format for this purpose.

## Unit Testing

*VmiCore* offers `googletest` mocks for various public interfaces. They can be included analogously
to `vmicore-public-headers` as described above.

```cmake
add_subdirectory("${VMICORE_DIRECTORY_ROOT}/test/include" "${CMAKE_CURRENT_BINARY_DIR}/vmicore-public-test-headers")

add_executable(myplugin-test "test/unittest.cpp")
target_link_libraries(myplugin vmicore-public-test-headers)
```

Keep in mind that this target depends on `vmicore-public-headers` as well as `gmock`.
