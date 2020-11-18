# Redundant Includes

A utility intended to simplify the process of finding redundant #include statements in C++ source code. The general idea is that there is no need for a compilation to include header files more than once. If A includes B and B includes C then the there’s no need for A to include C itself. Doing so just means more source code for the compiler to consume. Include guards are a widely used technique to mitigate this bloat, but we can also simply minimize the set of #include statements in our source code.

A simple example: imagine three source files (a.h, b.h, and c.h). These contain the following:

a.h:

    #include "b.h"
    #include "c.h"

b.h:

    #include "c.h"

c.h: (empty)

It’s fairly clear here that there is no need for a.h to include c.h directly because b.h already does so. That’s easy to see here but, as a program’s dependency tree becomes larger and more complex, that may not always be true. That’s where this little tool comes in.

## Using the tool

With the working directory at the project’s root, run the utility. You’ll see a list of the unnecessary #include statements. Note that there may be no output if there are no redundant includes to report!

Each line of output contains:

    <source file> : <include file>

`<source file>` is the name of a file with a redundant #include statement, `<include file>` is the name of the file being included.