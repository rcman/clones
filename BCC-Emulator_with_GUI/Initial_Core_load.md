# SDS 940 Initial Core Load (ICL) Startup Analysis

This document analyzes the startup sequence and system architecture extracted from the "Assembly of the Initial Core Load" technical document, dated approximately 1973.

---

## Overview

The ICL (Initial Core Load) is the bootstrap image for the SDS 940 operating system—the minimal kernel loaded into memory at system startup. The MAKE-ICL program assembles several component files into a unified bootable image.

---

## Startup Entry Points and Addresses

The MAKE-ICL output and BXFER extractions reveal the following memory layout:

| Component | Virtual Address | Pages | Purpose |
|-----------|----------------|-------|---------|
| Context Block (CB) | 600000B | 1 | Process/task control structures |
| MONITOR | 604000B | 10 | Core kernel code |
| MIB | 710000B | 1 | Monitor Information Block |
| Running Table (RT) | 664000B | 1 | Active process table |
| LISTENER | 434000B | 3 | Command interpreter |

> **Note:** Addresses suffixed with 'B' are octal notation.

---

## The LISTENER Entry Point

A particularly revealing section shows the sub-process entry point being examined and patched:

```
:-○̷403012B
2435520B
```

Address `403012B` contains the entry point value `2435520B`, which must be updated in the CB file to match each newly compiled LISTENER. The patching sequence writes this value into the Context Block at specific offsets using BRS commands (15D, 16D, 17D) to select different block records.

### Entry Point Patching Sequence

```
BRS 15D;U
CB.$$
10\ 3
4000;A
BIO 10;U
$$
BRS 17D;U
$
70\ 2B    2435620    2435520
```

The notation `70\ 2B` indicates word 70 (octal) contains two values being compared: the old value `2435620` versus the new `2435520`.

---

## System Initialization Flow

### Channel Table Initialization

From the MONITOR compilation, a critical initialization function is executed during the build:

```
:- INIT'CHT()
COMPILE 1-GO
EXIT TASK 1
```

The `INIT'CHT` function initializes the Channel Table (CHT)—the I/O channel configuration for the system. It runs during the build process to populate resident tables that become part of the ICL image.

---

## Ring Structure

The startup code reveals a protection ring architecture with distinct privilege levels:

### Ring 0 / Monitor Ring

- **Address Range:** 600000B - 652331B
- **Purpose:** Core kernel operations
- **Segments:**
  - Working Segment (WGS): 600160 - 602777
  - Code Segment (CS): 610000 - 652331

### Utility Ring

- **Address Range:** 403200 - 446034
- **Purpose:** User-facing services (LISTENER)
- **Segments:**
  - Working Segment (WGS): 403200 - 406777
  - Code Segment (CS): 435000 - 446034

---

## APU (Auxiliary Processor) Code

The APU code build involves two separate program packages assembled independently:

| Package | Size (words) | Compressed Size |
|---------|--------------|-----------------|
| AMCCD | 2457 | 1327 |
| UMCCD | 2513 | 1355 |

### Linking Configuration

```
%R 23250000    0
2325,0;R
240;F
```

- APU code loads at absolute address `2325xxxx`
- The `240;F` command fills/formats 240 (octal) words
- Two packages are linked into a single binary `APU:9BIN`

---

## Boot Sequence Inference

Based on the component layout and addresses, the likely startup sequence is:

```
┌─────────────────────────────────────────────────────────┐
│  1. ICL image loaded from storage into core memory      │
├─────────────────────────────────────────────────────────┤
│  2. Monitor at 604000B takes control                    │
├─────────────────────────────────────────────────────────┤
│  3. Running Table (RT) and Context Block (CB)           │
│     provide initial process/task structures             │
├─────────────────────────────────────────────────────────┤
│  4. Monitor initializes channels via pre-computed       │
│     CHT tables                                          │
├─────────────────────────────────────────────────────────┤
│  5. LISTENER at 434000B launched as first               │
│     user-ring process to accept commands                │
└─────────────────────────────────────────────────────────┘
```

---

## Build Toolchain Summary

The ICL assembly requires multiple specialized compilers and tools:

| Tool | Purpose | Type |
|------|---------|------|
| MSPL | Compile Monitor ring code | SPL variant |
| USPL | Compile Utility ring code | SPL variant |
| Old SPL (8-27.50) | Resident table initialization | 940 dump file |
| NARP | General assembler | Assembler |
| APUAS | APU code assembler | NARP variant |
| BXFER | Extract pages from dump files | QSPL binary |
| DDT | Debugger/loader | Interactive tool |
| MAKE-ICL | Final ICL assembly | 940 program |

---

## Key Observations

1. **Entry Point Validation:** The address `2435520B` in the LISTENER entry point falls within the Utility ring code segment range (435000-446034), confirming it's a valid entry address.

2. **Context Block Structure:** The CB modifications at offsets 15D, 16D, 17D (decimal 13, 14, 15) suggest the Context Block contains a table of entry points for different sub-process types.

3. **Manual Build Process:** The entire ICL assembly is a carefully orchestrated manual process—a stark contrast to modern automated build systems.

4. **Handwritten Annotations:** The original document contains handwritten notes about checking compilation state and occasional address issues at `604002B`, indicating this was a working reference document.

---

## Historical Context

The SDS 940, manufactured by Scientific Data Systems (later acquired by Xerox), was a pioneering timesharing computer. This document captures the intricate build process that system programmers followed to produce a bootable operating system image in the early 1970s.

---

*Analysis based on "Assembly of the Initial Core Load" technical document, circa 1973.*
