# mdata-client

This repository contains metadata retrieval and manipulation tools for use
within guests of the SmartOS hypervisor.  These guests may be either
SmartOS Zones or BHYVE / KVM virtual machines.

This repository is part of the TritonDataCenter project.  See the
[contribution guidelines](https://github.com/TritonDataCenter/triton/blob/master/CONTRIBUTING.md)
and general documentation at the main
[Triton project](https://github.com/TritonDataCenter/triton) page.

# Commands

There are four commands provided in this consolidation:

* [mdata-list(8)][mdata_list]; list custom metadata keys in the metadata store
* [mdata-get(8)][mdata_get]; get the value of a particular metadata key
* [mdata-put(8)][mdata_put]; set the value of a particular metadata key
* [mdata-delete(8)][mdata_delete]; remove a metadata key

Manual pages for these tools are available in this repository, and are
generally shipped with the OS (in the case of SmartOS) or in the package (e.g.
[for Ubuntu][launchpad_pkg]).  They are also viewable on the web at the links
above.

# Protocol and Transport

The Triton SmartOS Metadata Protocol [is documented online][protocol].  The programs in
this repository are clients that communicate using this protocol.  The SmartOS
(or TritonDataCenter) hypervisor provides a [common set][datadict] of supported
base metadata keys for guests to consume, as well as the ability to support
arbitrary additional user-provided metadata.

In a SmartOS container/zone guest, a UNIX domain socket is used to communicate
with the metadata server running in the hypervisor.  In a bhyve or KVM guest,
such as a Linux virtual machine, the client tools will make use of the second
serial port (e.g.  `ttyb`, or `COM2`) to communicate with the hypervisor.

# OS Support

The tools currently build and function on SmartOS, some \*BSDs, various Linux
distributions, and Windows (cygwin). Support for other operating systems is
absolutely welcome.

## Cygwin

To build mdata executables on Cygwin, use the 64-bit Cygwin installer 2.893 or
later available at https://cygwin.com/install.html. Building requires binutils,
gcc-core and make, all under the Devel category. Invoke the Cygwin terminal,
and run make on this repo.

make will produce several exe files. These programs can be copied to and run on
any version of Windows supported by Cygwin, so long as cygwin1.dll (found in
C:\cygwin\bin of the development machine's Cygwin install) is copied into the
same directory as the binaries.

# License

MIT (See _LICENSE_.)

[mdata_docs]: https://eng.tritondatacenter.com/mdata/
[protocol]: https://eng.tritondatacenter.com/mdata/protocol.html
[datadict]: https://eng.tritondatacenter.com/mdata/datadict.html
[mdata_get]: https://smartos.org/man/8/mdata-get
[mdata_delete]: https://smartos.org/man/8/mdata-delete
[mdata_put]: https://smartos.org/man/8/mdata-put
[mdata_list]: https://smartos.org/man/8/mdata-list
[launchpad_pkg]: https://launchpad.net/ubuntu/+source/joyent-mdata-client
