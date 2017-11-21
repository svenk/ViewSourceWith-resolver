# ViewSourceWith in Firefox Quantum, as external editor

The [Dafzilla ViewSourceWith](https://addons.mozilla.org/de/firefox/addon/dafizilla-viewsourcewith/) Firefox plugin
got incompatible with the [latest Firefox](https://www.mozilla.org/firefox/). Instead, what we can still do is to
use [view_source.editor](http://kb.mozillazine.org/View_source.editor.external) in `about:config`, i.e. set

```
view_source.editor.external = true
view_source.editor.path = "C:\Path\to\your\editor.exe"
```

This allows you to effectively move your `ViewSourceWith` logic out from JavaScript inside Firefox to the operating
system, as a program written in the language of your choice.

## The need of an executable instead of a script language on Windows

A scripting language with XML or HTML querying support might be a good choice for rapidly enabling functionality.
Working on Microsoft Windows, I first tried to integrate a PowerShell script,
[ViewSourceWith.ps1](https://gist.github.com/svenk/d932c8286c3b446c615a174f00fe71d5), into Firefox. The shell script
provided high level XML querying combined with calling an external editor. However, the Firefox integration did not
properly work due to broken argument passing (probably escaping). A script language will typically require you to 
set the `editor.path = ...\interpreter.exe` and the `editor.args = "Path\to\your\script.ps1 --filename=" or similar.

Instead, if you go with a C++ program where you have full control of the command line arguments, i.e.
`int main(int argc, char** argv)`, you can steer Firefox better.

## How to use this little tool

You might want to inspect the C++ code, it uses the traditional Windows API and can easily be compiled with MinGW
without any dependencies. Depending on your needs, you might want to extend it to more sophisticated HTML parsers
or pass the parsing to an external script.

In this particular case, I parse the given HTML file for some `<meta name="localfile" content="C:/foo/bar.php">`
where I then open an editor of choice with `C:/foo/bar.php`. This allows a neat mapping of server side to client
side files. This is of course not fully covering the features of ViewSourceWith, i.e. Firefox does not pass me
any information about the URL, it is only the generated HTML. As I use this only for editing a personal website,
it is okay as I inject the needed information into a development version of the generated HTML.

Credits: SvenK 2017-11-21, GPL2.
