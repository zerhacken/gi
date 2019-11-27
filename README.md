# Path Tracing
[![codecov](https://codecov.io/gh/zerhacken/gi/branch/master/graph/badge.svg)](https://codecov.io/gh/zerhacken/gi)[![Codacy Badge](https://api.codacy.com/project/badge/Grade/7747dda8fc0644789e4b4e6686ca8ffa)](https://www.codacy.com/manual/zerhacken/zerhacken-gi?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=zerhacken/iq&amp;utm_campaign=Badge_Grade)[![Build Status](https://travis-ci.org/zerhacken/gi.svg?branch=master)](https://travis-ci.org/zerhacken/gi)

fiddlings for recreational digging into global illumination

![gi](gi.png)

## Building code

```
git clone https://github.com/zerhacken/gi

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../.
cmake --build .
```

## clang-format

```
clang-format -style=llvm -i main.cpp
```

## wsl
```
sudo apt-get install cmake
sudo apt-get install lcov
sudo apt-get install build-essential gdb
```
