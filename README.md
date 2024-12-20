# WAD Filesystem

The **WAD Filesystem** project integrates a custom library for parsing and managing WAD files with a FUSE-based filesystem. This allows users to interact with WAD file contents as if they were regular files and directories on their system.

## Key Components

### `libWad`
- A library for handling WAD files, which are archives commonly used in gaming to store assets.
- Provides functionality to:
  - Load and parse WAD files (`Wad::loadWad`).
  - Build a hierarchical directory structure from the WAD file contents.
  - Create, modify, and retrieve files and directories within a WAD archive.

### `wadfs`
- Implements a FUSE-based filesystem that utilizes the `libWad` library.
- Key operations include:
  - Mounting a WAD file to a directory.
  - Accessing WAD contents using standard file and directory operations (`getattr`, `readdir`, `read`, `write`, etc.).
  - Creating new files and directories within the mounted filesystem.

## Features
- **Interactive Filesystem**: WAD contents can be browsed and manipulated like a regular filesystem.
- **Data Access**: Supports reading and writing to files within the WAD archive.
- **Custom Hierarchy**: Dynamically builds and manages a directory structure based on WAD descriptors.
- **Extensibility**: Users can add new files and directories, expanding the WAD archive as needed.

## Example Use Case

1. Mount a WAD file using the `wadfs` executable:
   ```bash
   ./wadfs <WAD file> <mount directory>
2. Access and manipulate the contents using standard shell commands:
   ```bash
   ls <mount directory>
   cat <mount directory>/file.txt
   echo "New Data" > <mount directory>/new_file.txt

## Requirements
- Dependencies:
  - FUSE library for the filesystem integration.
  - Linux Terminal
  - A standard C++ compiler (e.g., g++).
- Build Instructions:

## wad,tar,gz
Extract this file to get the above folders "wadfs" and "wad". Or just use the given folders.
  - Navigate to the libWad and wadfs directories and run make to compile library and FUSE Dameon.
