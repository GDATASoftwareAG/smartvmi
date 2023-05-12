# SmartVMI

Virtual Machine Introspection (VMI) for memory forensics and machine-learning.

# SmartVMI Code

## VmiCore

The SmartVMI project is split into a core component which manages access to the virtual machine and provides a high
abstraction layer for ease of plugin implementation.
See [VmiCore Readme](vmicore/Readme.md) for additional information as well as how to build/use this project.

## Plugins

To allow for easy extension SmartVMI provides a plugin interface. You can find already implemented plugins which also
serve as examples for how to use this project in the plugins folder.
For additional information see the corresponding plugin readme:

* [InMemoryScanner](plugins/inmemoryscanner/Readme.md)
* [ApiTracing](plugins/apitracing/Readme.md)

# SmartVMI Research Project

The project “Synthesizing ML training data in the IT security domain for VMI-based attack detection and analysis” (
SmartVMI) is a research project funded by the BMBF and DLR.
See: [www.smartvmi.org](http://www.smartvmi.org) for more information.
