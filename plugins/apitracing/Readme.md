[![Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=gdatasoftwareag_apitracing&metric=ncloc)](https://sonarcloud.io/summary/new_code?id=gdatasoftwareag_apitracing)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=gdatasoftwareag_apitracing&metric=coverage)](https://sonarcloud.io/summary/new_code?id=gdatasoftwareag_apitracing)

# ApiTracing Plugin

This plugin hooks functions of specific processes and extracts the function parameters.

## Tracing Config

In the tracing configuration tracing profiles are defined. An example can be
found [here](configuration/configuration.yml).
Each profile contains the traced modules as well as a flag indicating if child processes should also be traced.

```yaml
profiles:
  default:
    trace_children: true
    traced_modules:
      ntdll.dll:
        - LdrLoadDll
  calc:
    trace_children: true
    traced_modules:
      ntdll.dll:
        - function1
        - function2
traced_processes:
  calc.exe:
    profile: calc
  notepad.exe:
    profile: default
  paintstudio.view.exe:
    profile: default
```

The traced modules are the names of the shared libraries in which the target functions reside.
Note that the names are case-sensitive.

The second element in the tracing configuration contains the *traced_processes*.
For every process a profile should be defined, but several processes can be traced with the same profile.

## Function Definitions

In the [function definitions file](configuration/functiondefinitions/functionDefinitions.yaml) the parameters of the
target functions are defined.

A file containing a subset of available Windows API functions for Windows10 is provided with this project.

```yaml
Modules:
  KernelBase.dll:
    CreateProcessInternalW:
      Parameters:
        lpProcessInformation: LPPROCESS_INFORMATION
      ReturnValue: BOOL
Structures:
  LPPROCESS_INFORMATION:
    hProcess: HANDLE
    hThread: HANDLE
    dwProcessId: DWORD
    dwThreadId: DWORD
HighLevelParameterTypes:
  AddressWidth32Bit:
    DWORD: unsigned long
    HANDLE: unsigned int
  AddressWidth64Bit:
    DWORD: unsigned long
    HANDLE: unsigned __int64
BackingParameterTypes:
  unsigned __int64: 8
  unsigned long: 4
  unsigned int: 4
```

The function definitions file is split into 4 basic parts.
The *Modules* element should contain an entry for every *traced module* from the tracing configuration.
In the modules the target functions with their parameters, return value and if applicable the return parameters are
defined.

Each parameter type is resolved through *Structures* or *HighLevelParameterTypes*, split into 32 and 64-bit address
width,
until a *BackingParameterTypes* is reached.
A *BackingParameterType* is defined as a parameter for which a dedicated extraction function exists.

In the given (shortened) example the function *CreateProcessInternalW* has a parameter *lpProcessInformation* of the
type *LPPROCESS_INFORMATION*.
*LPPROCESS_INFORMATION* is a *Structure* containing several additional parameters.
For example the parameter *hProcess* is of the type *HANDLE*. Depending on the address width of the process it gets
resolved
to either *unsigned int* or *unsigned _int64*. Those are *BackingParameterTypes* so either 8 or 4 byte are read when
extracting this structure from the heap.