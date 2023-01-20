[![Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=gdatasoftwareag_smartvmi_vmicore&metric=ncloc)](https://sonarcloud.io/summary/new_code?id=gdatasoftwareag_smartvmi_vmicore)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=gdatasoftwareag_smartvmi_vmicore&metric=coverage)](https://sonarcloud.io/summary/new_code?id=gdatasoftwareag_smartvmi_vmicore)

# VMICore

_VMICore_ is a VM Introsepction tool capable of dynamic malware analysis.

## How to Build

- Install Build Requirements
    - g++ or clang++
    - cmake
    - vcpkg

- Clone this repository

- **\[Optionally]** Create an output directory

- Inside the output directory (or your current working directory for that matter), run:

```console
[user@localhost source_dir]$ cmake --preset <gcc/clang>-debug
[user@localhost source_dir]$ cmake --build --preset <gcc/clang>-build-debug
```

## How to Run

```console
[user@localhost output_dir]$ ./vmicore -c <path_to_configuration.yml> -n <domain_name>
```

Note: All parameters are optional but the program will abort if the configuration could not be found.
Default search location is `/etc/vmicore.conf `.

## Configuration

*VMICore* uses *YAML* as its configuration file format. An example configuration file can be found in `configurations/`.

### Plugin Configuration

*VMICore* is able to load plugins as shared object files at runtime. The folder in which to look for plugins can be
configured via the configuration file:

```yaml
plugin_system:
  directory: /usr/local/lib/
```

Plugins that should be loaded have to be declared by their shared object file name under the `plugins` node:

```yaml
plugin_system:
  directory: /usr/local/lib/
  plugins:
    libmyplugin.so:
      option1: value1
```

In the example above, everything under `libmyplugin.so` will be passed to the respective plugin as configuration
options.
