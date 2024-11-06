Source code generator for X680x0 version 3.16
=============================================

![Linux Build](https://github.com/vampirefrog/dis/actions/workflows/linux-build.yml/badge.svg)

```text
Copyright (C)1989-1992 K.Abe
Copyright (C)1994-1997 R.ShimiZu
Copyright (C)1997-2010 Tachibana
```

I've taken the [rururutan code](http://www.vesta.dti.ne.jp/~tsato/arc/dis-3.16w32.zip) and made it work on a 64-bit target.

Documentation
-------------

* [README.DOC](README.DOC)
* [README.W32](README.W32)
* [table.doc](table.doc)
* [tablefile.5](tablefile.5)
* [labelfile.5](labelfile.5)
* [dis_option.1](dis_option.1)

Compiling on 64-bit linux
-------------------------
```bash
cd src
make -f ports/Makefile_Linux
```

You can also install in `/usr/local/bin`:
```bash
make -f ports/Makefile_Linux install
```

Links
-----
* [X68K binaries](https://nfggames.com/X68000/Mirrors/x68pub/x68tools/PROGRAM/DIS/)
* [rururutan software](http://www.vesta.dti.ne.jp/~tsato/software.html)
