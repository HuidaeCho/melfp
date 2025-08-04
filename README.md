# Memory-Efficient Longest Flow Path (MELFP)

Part of the [Memory-Efficient I/O-Improved Drainage Analysis System (MIDAS)](https://github.com/HuidaeCho/midas)

**Citation**: Huidae Cho, September 2025. Loop Then Task: Hybridizing OpenMP Parallelism to Improve Load Balancing and Memory Efficiency in Continental-Scale Longest Flow Path Computation. Environmental Modelling & Software 193, 106630. [doi:10.1016/j.envsoft.2025.106630](https://doi.org/10.1016/j.envsoft.2025.106630). [Author's Version](https://idea.isnew.info/publications/Loop%20then%20task%20-%20Hybridizing%20OpenMP%20parallelism%20to%20improve%20load%20balancing%20and%20memory%20efficiency%20in%20continental-scale%20longest%20flow%20path%20computation%20-%20Cho.2025.pdf).

[MELFP Graphical Abstract](https://idea.isnew.info/publications/Loop%20then%20task%20-%20Hybridizing%20OpenMP%20parallelism%20to%20improve%20load%20balancing%20and%20memory%20efficiency%20in%20continental-scale%20longest%20flow%20path%20computation%20-%20Graphical%20abstract.svg)

Predefined flow direction encodings in GeoTIFF: power2 (default, r.terraflow, ArcGIS), taudem (d8flowdir), 45degree (r.watershed), degree<br>
![image](https://github.com/user-attachments/assets/990f0530-fded-4ee5-bfbb-85056a50ca1c)
![image](https://github.com/user-attachments/assets/a02dfc15-a825-4210-82c4-4c9296dafadc)
![image](https://github.com/user-attachments/assets/64f5c65a-c7cc-4e06-a69f-6fccd6435426)
![image](https://github.com/user-attachments/assets/fafef436-a5f2-464a-89a8-9f50a877932c)

Custom flow direction encoding is also possible by passing `-e E,SE,S,SW,W,NW,N,NE` (e.g., 1,8,7,6,5,4,3,2 for taudem).

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

For testing on Windows, you don't need to build MELFP yourself. `test\test.bat` will use the included binary `windows\melfp.exe`. However, the Windows version was about 3-12 times slower than the Linux version for 515,152 outlets on the same hardware.

```bash
cd test
pretest.bat
test.bat
```

When I tested it under WSL using the same hardware, it showed almost native Linux performance again.

* Linux (`test/linux_gcc.log`): 55382845 microsec (55.4 sec)
* Windows (`test/windows_msvc.log`): 171089000 microsec (171.1 sec)
* WSL (`test/wsl_gcc.log`): 60885546 microsec (60.9 sec)

These results suggest that the significant performance difference between Windows and WSL is not hardware-related, but likely due to differences in OpenMP runtime optimization, thread scheduling, task overhead, or system libraries.

For best performance on Windows systems, I highly recommend running the code in WSL.

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
msbuild melfp.sln -p:configuration=release
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
