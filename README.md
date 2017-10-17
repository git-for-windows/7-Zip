# 7-Zip with an enhanced SFX component

![alt text](http://www.7-zip.org/7ziplogo.png)

For more than a decade now, Git for Windows made use of the "modified SFX" that used to be available from http://7zsfx.info. Sadly, that project seems to have gone defunct some time during 2016 and nobody seems to know why.

Git for Windows used the "modified SFX" instead of the one shipped with [the official 7-Zip](http://7-zip.org/), for the following reasons:

* The portable Git distribution wants to install into the final location rather than into a temporary directory (which would get deleted after the SFX ran the included `setup.exe`). The "modified SFX" supports configuration to ask the user where to extract/install the archive.
* When canceling the installation, it is nice to be able to ask the user whether they are sure, and the modified SFX supports that.
* The "modified SFX" allows to set the window title of the "BeginPrompt" window as well as the progress window.

This fork supports those needs of the portable Git for Windows package. The `master` branch will always follow upstream 7-Zip's source code, while Git for Windows' modifications live in a branch based on the newest 7-Zip version (currently `v16.04-VS2017-sfx`) with the idea of rebasing the changes into a new branch whenever a new 7-Zip version comes out and then continue development on that new branch.

Please note that this project does not aim for a full replacement of the "modified SFX" so far:

* there is no support yet for any of the `--sfxconfig`, `--sfxversion`, `--sfxlang`, `--sfxtest`, `--sfxwaitall`, and the `--sfxelevation` command-line option.
* a *lot* of features are not reimplemented in this project, such as the `GUIMode`, the `GUIFlags`, or the `OverwriteMode` setting, just to name a few.

Pull Requests implemented interesting and useful features are warmly welcome, of course!

