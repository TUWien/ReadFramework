# READ Framework
READ Framework is the basis for modules developed at CVL/TU Wien for the EU project READ. The READ project  has  received  funding  from  the European  Union’s  Horizon  2020 research  and innovation programme under grant agreement No 674943.
 

[![Build Status](https://travis-ci.org/TUWien/ReadFramework.svg?branch=master)](https://travis-ci.org/TUWien/Framework)

## Build on Windows
### Compile dependencies
- `Qt` SDK or the compiled sources (>= 5.4.0)
- `OpenCV` (>= 3.0.0)

### Compile ReadFramework
1. Clone the repository from `git@github.com:TUWien/ReadFramework.git`
2. Open CMake GUI
3. set your ReadFramework folder to `where is the source code`
4. choose a build folder
5. Hit `Configure`
6. Set `QT_QMAKE_EXECUTABLE` by locating the qmake.exe
7. Set `OpenCV_DIR` to your OpenCV build folder
8. Hit `Configure` then `Generate`
9. Open the `ReadFramework.sln` which is in your new build directory
10. Right-click the ReadFramework project and choose `Set as StartUp Project`
11. Compile the Solution
12. enjoy

### If anything did not work
- check if you have setup opencv
- check if your Qt is set correctly (otherwise set the path to `qt_install_dir/qtbase/bin/qmake.exe`)
- check if your builds proceeded correctly

## Build on Ubuntu

### related links:
[1] http://www.caa.tuwien.ac.at/cvl/
[2] https://transkribus.eu/Transkribus/
[3] https://github.com/TUWien/
[4] http://nomacs.org