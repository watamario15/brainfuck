# Brainfuck for SHARP Brain

![](running.png)

**Brainfuck** interpreter and debugger for [SHARP **Brain**](https://jp.sharp/edictionary/). This interpreter is "nice" in terms of [this specification](http://www.muppetlabs.com/~breadbox/bf/standards.html).

## Specs

Default options are shown in **bold** typeface.

|       Item       |                                    Spec                                    |
| :--------------: | :------------------------------------------------------------------------: |
|   Memory type    | **8-bit two's-complement integer (-128 to 127)**, 8-bit integer (0 to 255) |
|  Memory length   |                              **65,536 bytes**                              |
|   I/O charset    |                            **UTF-8**, Shift_JIS                            |
|   Input error    |                        **input 0**, input FF, error                        |
| Integer overflow |                           **wrap around**, error                           |
|    Debugging     |                     Break on `@` (disabled by default)                     |

"error" options are useful for debugging and portability check.

## System Requirements

- Windows XP or later
- Windows CE 4.0 or later
  - Tested on SHARP Brain PW-SH1, which is Windows Embedded CE 6.0 (Armv5TEJ)

Might also work on older OSes than these, but these are the ones I'm confident in some degree.

## How To Use

Download appropriate one from [here](https://github.com/watamario15/brainfuck-sharp-brain/releases) and run it on your device. No installation needed.

## How To Build

This project supports various build tools.

### Windows PC

- `win.sh`: Builds binaries using MinGW_w64. Requires MinGW_w64 to be accessible from PATH.
  - Requires POSIX compliant environment.
- `win.bat`: Batchfile version of `win.sh`. Use for Windows build of MinGW_w64.
- `vs2022proj-pc/`: VS2022 solution and project. Just open the `.sln` file and build, with the C++ compilation support installed.
  - Requires Windows machine.

### Windows CE (SHARP Brain)

- `brain.sh`: Builds binaries using CeGCC. Requires CeGCC to be accessible from PATH.
  - Requires POSIX compliant environment.
- `brain.bat`: Batchfile version of `brain.sh`. Use for Windows build of CeGCC.
- `vs2005proj/` and `vs2008proj/`: VS2005/2008 solution and project. Just open the `.sln` file and build, with the mobile C++ compilation support installed.
  - Requires Windows machine.
- `evc4proj/`: eMbedded Visual C++ 4.0 project. Just open the `.vcw` file and build.
  - Requires Windows machine.
