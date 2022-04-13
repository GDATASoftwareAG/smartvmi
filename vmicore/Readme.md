# VMICore

_VMICore_ is a VM Introsepction tool capable of dynamic malware analysis.

## How to Build

-   Install Build Requirements
    -   g++ or clang++
    -   cmake
    -   tclap

-   Clone this repository

-   **\[Optionally]** Create an output directory

-   Inside the output directory (or your current working directory for that matter), run:

```console
[user@localhost output_dir]$ cmake <path_to_top_level_project_dir>
[user@localhost output_dir]$ cmake --build .
```

## How to Run

```console
[user@localhost output_dir]$ ./vmicore -c <path_to_configuration.yml> -n <domain_name>
```

Note: All parameters are optional but the program will abort if the configuration could not be found.
Default search location is `/etc/vmicore.conf `.
