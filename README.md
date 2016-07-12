# ex-2.2 (from 2BSD release)
This is `vi` version 2.2 taken from 2BSD.
It had been released in May 1979.
## Installation notes
The software is downloaded with
```sh
git clone https://github.com/n-t-roff/ex-2.2.git
```
and can be kept up-to-date with
```sh
git pull
```
Some configuration (e.g. installation paths) can be done in the
[`makefile`](https://github.com/n-t-roff/ex-2.2/blob/master/Makefile.in).
For compiling it on BSD and Linux autoconfiguration is required:
```sh
$ ./configure
```
The software is build with
```sh
$ make
```
and installed with
```
$ su
# make install
# exit
```
All generated files are removed with
```sh
$ make distclean
```
## Features which had been invented after version 2.2

* `j`, `k`, `l`.
  Use the alternative commands for cursor motions:

  * `h`: `h` works, alternative is `^H`
  * `j`: `^N`
    (`+` and &lt;ENTER&gt; are similar)
  * `k`: `^P`
    (`-` is similar)
  * `l`: &lt;SPACE&gt;

* `ZZ`, `:x` (use `:wq`)
* `~`, `^E`, `^Y`, `:map`
* Job control
* The comment character `"`.
  If you need to comment something in `~/.exrc`,
  put these lines to the end of the file
  and insert an empty line before them.
* There is no read-only mode.
  Program name `view` and option `-R` did not exist.
* The documents
  [viin.pdf](http://n-t-roff.github.io/ex/2.2/viin.pdf),
  [viapp.pdf](http://n-t-roff.github.io/ex/2.2/viapp.pdf)
  and
  [exrm.pdf](http://n-t-roff.github.io/ex/2.2/exrm.pdf)
  describe vi version 2.2 in detail,
  [ex2.0-3.1.pdf](http://n-t-roff.github.io/ex/3.2/ex2.0-3.1.pdf)
  shows the differences to later vi versions.

**Attention**:
The original `vi` had not been 8-bit clean!
Moreover it does automatically change all 8-bit characters to 7-bit
in the whole file even if no editing is done!
This will e.g. destroy all UTF-8 characters.
