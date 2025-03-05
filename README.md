# Memory-Efficient Longest Flow Path (MELFP)

Part of the [Memory-Efficient I/O-Improved Drainage Analysis System (MIDAS)](https://github.com/HuidaeCho/midas)

A journal manuscript on this algorithm is currently under review.

## Test data

You can find test data at https://data.isnew.info/melfp.html.

## How to test on Linux

1. Build MELFP first
2. Test it
```bash
cd test
./pretest.sh
./test.sh
```

## How to test on Windows

For testing on Windows, you don't need to build MELFP yourself. `test\test.bat` will use the included binary `windows\melfp.exe`.

```bash
cd test
pretest.bat
test.bat
```

## How to build on Linux

1. First, install the [GDAL](https://gdal.org/) library.
2. Build MELFP
```bash
make
```
or
```bash
mkdir build
cd build
cmake ..
make
```

## How to build on Windows

1. Install [Visual Studio Community Edition](https://visualstudio.microsoft.com/vs/community/). Select these two components:
   * MSVC v143 - VS 2022 C++ x64/x86 build tools (Latest)
   * Windows 11 SDK (10.0.26100.0)
2. Install [Git for Windows](https://gitforwindows.org/)
3. Install [Miniconda](https://www.anaconda.com/download/success)
```cmd
curl -O https://repo.anaconda.com/miniconda/Miniconda3-latest-Windows-x86_64.exe
Miniconda3-latest-Windows-x86_64.exe /S /D=C:\opt\miniconda
C:\opt\miniconda\condabin\conda.bat init
```
4. Start Developer Command Prompt for VS 2022
5. Setup Conda for GRASS build
```cmd
conda config --add channels conda-forge
conda config --set channel_priority strict
conda create -n melfp cmake libgdal
conda activate melfp
```
6. Download the source code
```cmd
cd \opt
git clone git@github.com:HuidaeCho/melfp.git
cd melfp
mkdir build
cd build
```
7. Build MELFP
```cmd
cmake ..
msbuild melfp.sln /p:Configuration=Release
```
or
```cmd
cmake ..
cmake --build . --config Release
```
or
```cmd
cmake -DCMAKE_CONFIGURATION_TYPES=Release
msbuild melfp.sln
```
8. Find melfp.exe in `Release\melfp.exe`
