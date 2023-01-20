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
  hProcess:
    Type: HANDLE
    Offset: 0
  hThread:
    Type: HANDLE
    Offset: 8
  dwProcessId:
    Type: DWORD
    Offset: 16
  dwThreadId:
    Type: DWORD
    Offset: 20
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

## Extraction Format

Function call traces are logged in an unindented json format which is illustrated indented below for better visibility.
The parameter extraction logs are identified by the *key* ```Parameterlist:```.
Its *value* is a list of all function call parameters, whereby the parameter names form the *keys* of the contained *key:value* pairs.
Each *value* is either the value of the parameter as integer or string value, or it is a list containing
the *key:value* pairs of backing parameters if it's a pointer to a struct.


```json
{
    "Parameterlist": [
                    {"FileHandle":[
                                    {"HANDLE":45}]},
                    {"DesiredAccess":1180063},
                    {"ObjectAttributes":[
                                    {"Length":48},
                                    {"RootDirectory":0},
                                    {"ObjectName":"\\Device\\ConDrv\\Server"},
                                    {"Attributes":66},
                                    {"SecurityDescriptor":0},
                                    {"SecurityQualityOfService":0}]},
                  {"IoStatusBlock":958106952720},
                  {"AllocationSize":0},
                  {"FileAttributes":0},
                  {"ShareAccess":7},
                  {"CreateDisposition":2},
                  {"CreateOptions":0},
                  {"EaBuffer":0},
                  {"EaLength":0}]
}
```

The log example above is a function call of *NtCreateFile* with these parameters:

```c
__kernel_entry NTSTATUS NtCreateFile(
  [out]          PHANDLE            FileHandle,
  [in]           ACCESS_MASK        DesiredAccess,
  [in]           POBJECT_ATTRIBUTES ObjectAttributes,
  [out]          PIO_STATUS_BLOCK   IoStatusBlock,
  [in, optional] PLARGE_INTEGER     AllocationSize,
  [in]           ULONG              FileAttributes,
  [in]           ULONG              ShareAccess,
  [in]           ULONG              CreateDisposition,
  [in]           ULONG              CreateOptions,
  [in]           PVOID              EaBuffer,
  [in]           ULONG              EaLength
);
```

*FileHandle, DesiredAccess, ObjectAttributues, IOStatusBlocck,AllocationSize, FileAttributes,ShareAccess, CreateDisposition,
CreateOptions, EaBuffer* and *EaLength* are members of the list in parameterlist *value*.

```c
typedef struct _OBJECT_ATTRIBUTES {
  ULONG           Length;
  HANDLE          RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG           Attributes;
  PVOID           SecurityDescriptor;
  PVOID           SecurityQualityOfService;
} OBJECT_ATTRIBUTES;
```

*ObjectAttributes* is a pointer to a struct, so its value is a list containing the backing parameters.
The parameter *ObjectName* is a pointer to a unicode and forms an exception since it is extracted directly.
Both *PVOID* at the end are structs, that are currently not covered by our definitions.
You can find a list under the struct section in the [function definitions file](configuration/functiondefinitions/functionDefinitions.yaml).

## How to Build

- Install Build Requirements
  - g++ or clang
  - cmake
  - vcpkg
- Clone this repository
- Inside the source directory, run:

```console
[user@localhost source_dir]$ cmake --preset <gcc/clang>-debug
[user@localhost source_dir]$ cmake --build --preset <gcc/clang>-build-debug
```
