# SAFED



## Overview

**SAFED** is a from-scratch operating system developed as part of a university project. The goal of this project is to gain a deep understanding of core OS concepts, including memory management, process scheduling, file systems, and hardware interaction.

## Features

- Kernel written in C
- Basic memory management
- Simple process scheduling
- Keyboard and screen I/O
- Basic file system support

## Getting Started

### Prerequisites

To build and run SAFED, you need:

- A Unix-like development environment (Linux/macOS)

- GCC cross-compiler for i386

- `make` for build automation

### Building the OS

Access (cd) each module manually (kernel, memoria, cpu, filesystem) first

```sh
make all
```

### Cleaning the Build

```sh
make clean
```

## Directory Structure

```
tp-sisop-2024-SAFED/

├── cpu
├── filesystem
├── kernel
├── memoria
├── pruebas
├── README.md
├── tp.code-workspace
└── utils

```

