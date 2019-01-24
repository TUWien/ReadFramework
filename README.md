# READ Framework
READ Framework is the basis for modules developed at CVL/TU Wien for the EU project READ. The READ project  has  received  funding  from  the European  Union’s  Horizon  2020 research  and innovation programme under grant agreement No 674943.


[![Build Status](https://travis-ci.org/TUWien/ReadFramework.svg?branch=master)](https://travis-ci.org/TUWien/ReadFramework)
[![codecov](https://codecov.io/gh/TUWien/ReadFramework/branch/master/graph/badge.svg)](https://codecov.io/gh/TUWien/ReadFramework)

Documentation can be found here: http://read-api.caa.tuwien.ac.at/ReadFramework/

## Build on Windows
### Compile dependencies
- `Qt` SDK or the compiled sources (>= 5.8.0)
- `OpenCV` (>= 3.2.0)

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
note that Qt 5.5 is needed, thus Ubuntu version must be >= 16.04 or backports of Qt 5.5 have to be used (see .travis.yml for an ppa repository and names packages which need to be installed).

Get required packages:

``` console
sudo apt-get install qt5-qmake qttools5-dev-tools qt5-default libqt5svg5 qt5-image-formats-plugins libopencv-dev cmake git
```

You also need OpenCV > 3.0. Either you can compile it yourself or perhaps you find a repository (you can also use the one from the .travis file, but be carefull, these packages are not tested, then you need following packages:
``` console
sudo apt-get install libopencv-dev libopencv-stitching-dev libopencv-imgcodecs-dev libopencv-flann-dev   libopencv-features2d-dev libopencv-calib3d-dev libopencv-hdf-dev libopencv-reg-dev libopencv-shape-dev libopencv-xobjdetect-dev libopencv-xfeatures2d-dev libopencv-ximgproc-dev libopencv-highgui-dev
```

Get the READ Framework sources from github:
``` console
git clone https://github.com/TUWien/ReadFramework
```
This will by default place the source into ./ReadFramework

Go to the ReadFramework directory and run `cmake` to get the Makefiles:
``` console
cd ReadFramework
cmake .
```

Compile READ Framework:
``` console
make
```

You will now have a binary (ReadFramework), which you can test (or use directly). Also the build libraries are in this directory. To install it to /usr/local/bin, use:
``` console
sudo make install
```

## Build on macOS (MacPorts)

Get required packages:
``` console
sudo port install cmake qt5 opencv
```

By default, `qmake` is installed in `/opt/local/libexec/qt5/bin/qmake` which might not be in your `PATH`. Set a link to an appropriate directory, e.g.:
``` console
sudo ln -s /opt/local/libexec/qt5/bin/qmake /opt/local/bin/qmake
```

Get the READ Framework sources from github:
``` console
git clone https://github.com/TUWien/ReadFramework.git
```

This will by default place the source into ./ReadFramework

Go to the ReadFramework directory and run `cmake` to get the Makefiles:
``` console
cd ReadFramework
cmake .
```

Compile READ Framework:
``` console
make
```

You will now have a macOS app (`ReadFramework.app/`), which contains a command-line interface (`ReadFramework.app/Contents/MacOS/ReadFramework`) which you can test (or use directly). Also the resulting libraries are in the working directory. To install everything it to `/usr/local/`, use:
``` console
sudo make install
```

### Authors
- Markus Diem
- Stefan Fiel
- Florian Kleber

### Links:
- [1] https://cvl.tuwien.ac.at/
- [2] https://transkribus.eu/Transkribus/
- [3] https://github.com/TUWien/
- [4] http://nomacs.org
