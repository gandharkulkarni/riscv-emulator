# RISC-V Emulation and Cache Simulation

## Table of Contents
- Introduction
- Features
- Installation
- Usage
- Dynamic Analysis
- Cache Simulation
- Contributing
- Acknowledgments
- License

## Introduction
This project presents a comprehensive emulator for a subset of the RISC-V ISA, capable of executing specific assembly programs and performing dynamic analysis of instruction execution. It also includes a cache simulator with various configurations to analyze performance implications on modern processors.

## Features
- **Emulation**: Supports a subset of RISC-V instructions sufficient to run provided and custom assembly programs.
- **Dynamic Analysis**: Collects metrics such as instruction count, type-specific counts, and branch behavior.
- **Cache Simulation**: Implements multiple cache configurations, including direct-mapped and set-associative caches with different block sizes and replacement policies.

## Installation
To install the emulator and cache simulator, clone the repository and compile the source code using the provided Makefile:

```bash
git clone [Repository-URL]
cd [Repository-Name]
make 
```

## Usage
Run the emulator by specifying the program file as an argument:

```bash
./rv_emu [program_file]
```

Dynamic Analysis
To perform dynamic analysis, use the -a flag followed by the program file:

```bash
./rv_emy -a [program_file]
```

## Cache Simulation
To simulate different cache configurations, use the -c flag followed by the cache type and the program file:

```bash
./rv_emu -c [cache_type] [program_file]
```

## Acknowledgments
Special thanks to Professor Greg Benson for his expert guidance and support throughout the development of this project.
