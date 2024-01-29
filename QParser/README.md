# QParser &mdash; Proof of Concept Code

QParser (a temporary name) is an attempt to reproduce the QSpice template code generator for more convenient use and add features, reliability, and flexibility.  I have dreams....

This is experimental code.  By "experimental," I mean <b><i>really, really experimental!</b></i>

Let's be clear:  I'm reverse-engineering the QSpice schematic file format.  It's all guess-work and very likely wrong.  The code published here will change rapidly and without warning or explanation.  The code comments are the only up-to-date documentation.  And it's almost certainly broken in ways that I've yet to discover.  Toy with it at your own risk.

If you want to play along, I will provide the *.exe.  I'll also provide sources for those of you that are properly afraid of running executables from unreliable sources.  You'll need
the open source community version of Qt and configured for C++20, at least for the moment.

I will definitely need folk to test the code-generation and compare it to the QSpice-generated code.  Do let me know if you try it.  In fact, please try to break it.  Let me know what works/doesn't work.

--robert