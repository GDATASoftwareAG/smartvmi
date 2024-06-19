<center><img src="assets/logos/smartvmi.svg" width="500"></center>

The project "Synthesizing ML training data in the IT security domain
for VMI-based attack detection and analysis" (SmartVMI) is a research project funded by the [BMBF](https://www.bmbf.de/) and [DLR](https://www.dlr.de/).

    BMBF support code: 01IS21063A-C
    Project Runtime: 01.10.2021 - 30.09.2023

The consortium working on the project consists of three entities. The [G DATA CyberDefense AG](https://www.gdata.de/), [Innowerk-IT GmbH](https://www.innowerk-it.de/) and [University of Passau](https://www.fim.uni-passau.de).


<a href="https://www.gdata.de/"><img src="assets/logos/gdata.jpg" height="80"></a>
<a href="https://www.innowerk-it.de/"><img src="assets/logos/innowerk.png" height="80"></a>
<a href="https://www.fim.uni-passau.de/"><img src="assets/logos/unipassau.png" height="80"></a>

___

## Attack pattern generation for trace detection

### Motivation
Just a few years ago, IT systems were primarily isolated and operated within easily protected boundaries. These systems could only communicate within a single company or within a fixed organizational context. Attacks were therefore usually limited in their impact. However, this situation has changed significantly. Today's IT systems are more like fragile entities. They use diverse, flexible, virtualized and highly networked applications, which are often operated outside the company or organizational structures. Complex attacks specifically tailored to a company or organization have been on the rise in the recent past. These attacks drastically illustrate the vulnerability and abuse potential of IT systems.

### Goals and approach
The SmartVMI project is dedicated to improving artificial intelligence (AI)-based attack detection, enabling attack defense and attack analysis, and supporting digital forensics by generating tailored synthetic attack patterns. This will enable the simulation of novel attack scenarios and the testing of existing attack detection and analysis mechanisms as well as the optimization of these mechanisms for new attacks. All tools developed in the course of the SmartVMI project will be made available in the open source model and validated training data obtained will be published in the public data set model. A university and two companies are working closely together in the consortium.

### Innovations and perspectives
The novelty of the project lies in the development of methods for generating customized synthetic training data for machine-based learning. This means that data does not have to be collected laboriously and possibly with a time delay, but can be generated promptly (if necessary also preventively) and in a targeted manner. The major benefit is the faster adaptation of attack detection mechanisms to new attacks as well as to new software systems. 

The tools as well as the generated and validated training data will be released to the public so that third parties can adapt them to their own mechanisms and infrastructures. Small and medium-sized enterprises in particular are expected to benefit from this offering. They should use the resulting data generation algorithms as an open source solution and use this reference implementation as a starting point for their own product developments and services.

In the German economy alone, computer crime causes damage of more than 10 billion euros every year. Securing IT systems against cyber attacks and cyber espionage is therefore crucial for the economy and society to be able to take advantage of the progress and opportunities offered by digitization. In the funded project, procedures are being researched that, on the one hand, use innovative forensic reconnaissance methods to investigate and understand attack scenarios. On the other hand, these findings will be used to create new possibilities for detecting and preventing such attacks in advance and in real time.

### Accepted Publications

* Stewart Sentanoe, Thomas Dangl, Hans P. Reiser. KVMIveggur: Flexible, secure, and efficient support for self-service virtual machine introspection. In: Forensic Science International: Digital Investigation, Volume 42S, 2022. https://doi.org/10.1016/j.fsidi.2022.301397 
* Thomas Dangl, Stewart Sentanoe, Hans P. Reiser: VMIFresh: Efficient and fresh caches for virtual machine introspection., In Computers & Security, Volume 135, 2023. https://doi.org/10.1016/j.cose.2023.103527.
* Thomas Dangl, Stewart Sentanoe, Hans P. Reiser: VMIFresh: Efficient and Fresh Caches for Virtual Machine Introspection. In Proceedings of the 17th International Conference on Availability, Reliability and Security (ARES), ACM,2022. https://doi.org/10.1145/3538969.3539002 
* Stewart Sentanoe, Christofer Fellicious, Hans P. Reiser, Michael Granitzer: “The Need for Speed”: Extracting Session Keys From the Main Memory Using Brute-force and Machine Learning. In Proceedings of 21st IEEE International Conference on Trust, Security and Privacy in Computing and Communications (TrustCom), 2022.	 https://doi.org/10.1109/TrustCom56396.2022.00140 
* Christofer Fellicious, Lorenz Wendlinger, Michael Granitzer: Neural Network based Drift Detection. In Proceedings of the the 8th International Online & Onsite Conference on Machine Learning, Optimization, and Data Science, 2022. <br>https://doi.org/10.1007/978-3-031-25599-1_28 
* Thomas Dangl, Stewart Sentanoe, Hans P. Reiser: Retrofitting AMD x86 Processors with Active Virtual Machine Introspection Capabilities. In: Proc. of Architecture of Computing Systems. ARCS 2023. Lecture Notes in Computer Science, vol 13949. Springer, https://doi.org/10.1007/978-3-031-42785-5_12 
* Thomas Dangl, Stewart Sentanoe, Hans P. Reiser: Active and passive virtual machine introspection on AMD and ARM processors. Journal of Systems Architecture, Vol. 149, 103101, 2024.	  https://doi.org/10.1016/j.sysarc.2024.103101 
* Christofer Fellicious, Stewart Sentanoe, Hans P. Reiser, Michael Granitzer PointerKex: A Pointer-based SSH Key Extraction method. In: Proceedings of the 10th International Conference on Machine Learning, Optimization and Data science (LOD), 2024 – accepted for publication 

### Accepted Poster

* S. Sentanoe, H. P. Reiser, C. Fellicious, M. Granitzer, T. Dangl, N. Beierl, S. Hausotte, M. Bischof, D. Eikenberg, K. Mayer, A. Pavic, S. Dambeck: "SmartVMI: Reconstructing meaningful kernel-level and application-level information about a target system using machine learning", NordSec 2022, Iceland
* Fellicious, Christofer, Sahib Julka, Lorenz Wendlinger, and Michael Granitzer. "DriftGAN: Using historical data for Unsupervised Recurring Drift Detection." In Proceedings of the 39th ACM/SIGAPP Symposium on Applied Computing, pp. 368-369. 2024

### Data Sets

* Christofer Fellicious, Stewart Sentanoe, Michael Granitzer, und Hans P. Reiser (2022). Machine Learning Assisted SSH Keys Extraction From The Heap Dump (v0.1). Zenodo. https://doi.org/10.5281/zenodo.6537904 
* Stewart Sentanoe, Christofer Fellicious, Hans P. Reiser, Michael Granitzer: Extracting Session Keys From the Main Memory Using Brute-force and Machine Learning (v0.1). Zenodo, 2022, https://doi.org/10.5281/zenodo.7014775 
* Fellicious, Christofer, Michael Granitzer, Hans P. Reiser, G DATA CyberDefense AG, and University of Passau. “API Traces for Malware Detection”. Zenodo, April 28, 2024, https://doi.org/10.5281/zenodo.11079764. 


___

With funding from the [BMBF](https://www.bmbf.de/) and organisational execution by the [DLR](https://www.dlr.de/).


<a href="https://www.bmbf.de/"><img src="assets/logos/bmbf.jpg" height="80"></a>
<a href="https://www.dlr.de/"><img src="assets/logos/dlr.jpg" height="80"></a>
