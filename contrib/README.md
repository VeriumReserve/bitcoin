Repository Tools
---------------------

### [Developer tools](/contrib/devtools) ###
Specific tools for developers working on this repository.
Contains the script `github-merge.py` for merging GitHub pull requests securely and signing them using GPG.

### [Linearize](/contrib/linearize) ###
Construct a linear, no-fork, best version of the blockchain.

### [Qos](/contrib/qos) ###

A Linux bash script that will set up traffic control (tc) to limit the outgoing bandwidth for connections to the Verium network. This means one can have an always-on veriumd instance running, and another local veriumd/verium-qt instance which connects to this node and receives blocks from it.

Build Tools and Keys
---------------------

### [Debian](/contrib/debian) ###
Contains files used to package veriumd/verium-qt
for Debian-based Linux systems. If you compile veriumd/verium-qt yourself, there are some useful files here.

### [MacDeploy](/contrib/macdeploy) ###
Scripts and notes for Mac builds. 

### [RPM](/contrib/rpm) ###
RPM spec file for building verium-core on RPM based distributions.
