# boilerplate

A small utility which is used to add the boilerplate text containing copyright and license information at the top of all of our source files. To ensure that paths are emitted correctly it should always be run from the project’s root directory.

As well as the boilerplate program itself, the `all.py` utility provides a convenient way of updating all of the C++ header files, source files, Python utilities, and so on within the directory hierarchy of the pstore project.

By default, `boilerplate.py` uses an external tool [“FIGlet”](http://www.figlet.org) to generate banner text. If not available, this feature can be disabled.
