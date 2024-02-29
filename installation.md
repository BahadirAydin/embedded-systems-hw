# Arch Linux Installation Guide for MPLab X IDE and XC8 Compiler

This guide will walk you through the process of installing MPLab X IDE v5.45 and XC8 Compiler v2.30 on Arch Linux. You can download both the IDE and the compiler from the Microchip Archives (https://www.microchip.com/en-us/tools-resources/archives/mplab-ecosystem). **Search and download the specific version we need, not the latest one.**

## Prerequisites

Before proceeding with the installation, you need to enable multilib libraries for pacman. Follow the instructions provided in the Arch Linux Wiki (https://wiki.archlinux.org/title/official_repositories) to enable multilib repositories.Basically just uncomment the lines in "/etc/pacman.conf":
```
[multilib]
Include = /etc/pacman.d/mirrorlist
```

## Installing Dependencies

MPLab X IDE and XC8 Compiler require certain 32-bit libraries to be installed on your system. You can install these packages using pacman:

```bash
sudo pacman -S lib32-gcc-libs lib32-glibc lib32-libxext lib32-expat
```
The packages to be installed are:
- lib32-gcc-libs 
- lib32-glibc
- lib32-libxext
- lib32-expat

## Installing MPLab X IDE and XC8 Compiler

After downloading MPLab X IDE and XC8 Compiler from the provided link, follow these steps to install them:

1. Make the Installer Executable: Change the permission of the downloaded .sh or .run installer files to make them executable. Replace [filename] with the actual name of your downloaded file:

```bash
chmod +x [filename]
```
2. Run the Installer: Execute the installer with superuser privileges. Follow the on-screen instructions to complete the installation process:

```bash
sudo ./[filename]
```

Then the installation will guide you through the process. 

