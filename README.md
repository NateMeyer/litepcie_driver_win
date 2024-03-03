# Litex Windows Driver and Library

This repo contains Visual Studio projects for building a driver, library and test application for the [LitePCIe LiteX core](https://github.com/enjoy-digital/litepcie).

The library is intended to have a compatible API with the liblitepcie found in the [LitePCIe user software](https://github.com/enjoy-digital/litepcie/tree/master/litepcie/software/user/liblitepcie) to facilitate cross platform applications.

## Setup Test Signing

1. Create a [Test Signing Certificate](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/creating-test-certificates)

2. Enable loading Test Signed Drivers on your Test PC


## Building with Visual Studio

1. Download and install Visual Studio 2022 and the WDK (https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk)

2. Open the solution

3. Setup a test certificate for the `litepciedrv` project

4. Build the solution

## Building with EWDK

1. Download and install the EWDK.

2. Launch the EWDK environment

```cmd
> <EWDKInstallDir>\LaunchBuildEnv.cmd
```

3. From within the EWDK environment, build the solution with `msbuild`
```cmd
<LitePCIe-Repo-Dir>\litepciedrv > msbuild litepciedrv.vcxproj /p:Configuration=Debug /p:Platform=x64
```

## Build Library and test app with cmake

```cmd
<LitePCIe-Repo-Dir>\build > cmake ..
<LitePCIe-Repo-Dir>\build > cmake --build .
```
