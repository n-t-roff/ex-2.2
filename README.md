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
For compiling on BSD, Linux and Solaris autoconfiguration is required:
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
## Usage notes for version 2.2

* When inserting characters before a tab,
  characters following the tab are shifted.
  This is fixed with `^L`.
  (The bug is fixed in vi version 2.6.)
* If the screen with is not a multiple of the tab width
  tab characters are displayed wrong
  after screen updates
  which results in a left shift of the subsequent text.
  (This bug is fixed in vi version 3.4.)
  The display is fixed with the following actions:

  * The current line is always displayed correct after `^L`.
  * All screen lines are fixed with screen updates after
    `^F` `^B`, `''` `''`. `^^` `^^` and so on.
  * The issue does not occur if the terminal width is set
    to a multiple of the tab width (e.g. a multiple of 8)
    *before* vi is started.

* As with vi version 1.1
  it is not possible to delete a range of lines
  with `:`*startline*`,`*endline*`d` and then insert these
  lines with `p` or `P`.
  Use *n*`d` to delete and then `p` or `P` to insert.

Features which had been invented after version 2.2:

* `j`, `k`, `l`.
  Use the alternative commands for cursor motions:

  * `h` works, alternative is `^H`
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
