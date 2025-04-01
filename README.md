# RISC-V Cache and Assembly Simulation

## Project Overview
This project was completed during my first year in the Computer Architecture course. 
It presents a full-featured simulation of a RISC-V-based processor and memory subsystem. It models how instructions are 
executed at the hardware level, simulates cache behavior using advanced eviction policies, and encodes human-readable 
assembly code into raw machine instructions.



---

## Key Capabilities
- **Manual Translation from C to RISC-V Assembly**  
  All instruction mapping and register allocation were written by hand using the RV32I and RV32M instruction sets.

- **Assembler to Machine Code Generator**  
  A custom encoding engine parses instructions and outputs 32-bit machine code with correct opcode layout, instruction types (R/I/S/B/U/J), and field segmentation.

- **Cache Simulation Engine**
    - Look-through write-back policy
    - Eviction strategies: **Least Recently Used (LRU)** and **bit-based pseudo-LRU (pLRU)**
    - Simulates full memory access pipeline, including cache hits, misses, line replacements, and memory writes

- **Performance Analytics**
    - Computes hit/miss statistics
    - Benchmarks different policies under the same workload for comparison
    - Visual logging of cache state transitions

- **Command-Line Configurable**  
  Easily adjustable from the terminal:
  ```bash
  --asm <path>         # Path to assembly source file (required)
  --bin <path>         # Output file path to save generated machine code (optional, but recommended)
  --replacement <int>  # Cache policy selection:
                       #    0 – run both LRU and pLRU (default)
                       #    1 – run only LRU
                       #    2 – run only pLRU
  ```
  Example usage:
  ```bash
  ./cache_sim --asm code.asm --bin code.bin --replacement 0
  ```

---

## System Architecture
- **Language:** Modern C++20
- **ISA:** RISC-V RV32I + RV32M
- **Cache System Configuration:**
    - Total addressable memory: 2^18 bytes = 256KB
    - Cache size: 2 KB
    - 4-way set-associative
    - Line size: 64 bytes
    - Cache lines: 32
    - Index bits: 3
    - Offset bits: 6
    - Tag bits: 9

P.S. The current cache configuration parameters (e.g., address length, line size, associativity, and others) were 
calculated based on fixed values provided by the course headmaster as part of the original assignment.

Planned Improvement: In future versions, I aim to make the cache system fully configurable — allowing users to define 
cache size, associativity, line count, and replacement policy dynamically via command-line arguments or configuration 
files. This will make the simulator more versatile and suitable for a wider range of experiments and use cases.

###  Memory Address Breakdown
| Tag (9 bits) | Index (3 bits) | Offset (6 bits) |
|-------------|----------------|-----------------|

---

## Modular Components
### Cache System
- `CacheBase` – abstract base class for unified interface
- `CacheLRU` – tracks least recently used line per set
- `CachePLRU` – uses compact PLRU bit trees

### Simulator Core
- `CacheSimulator` – computes access stats, delegates requests to selected cache, manages eviction and replacement
- `Simulator` – orchestrates CLI, I/O, machine code generation, and statistics reporting

### Instruction Encoding
- `AssemblyToMachineCode()` – custom instruction parser + encoder for:
    - R-type: `add`, `sub`, `mul`, ...
    - I-type: `addi`, `lw`, `jalr`, ...
    - S/B/U/J types: `sw`, `beq`, `lui`, `jal`, etc.
- Operates on lambda-wrapped C++ functions simulating CPU behavior for each command

---

## Instruction Support
### RV32I Set
- Arithmetic: `add`, `sub`, `sll`, `xor`, `slt`, `or`, `and`
- Immediate: `addi`, `andi`, `ori`, `slti`
- Branches: `beq`, `bne`, `blt`, `bge`
- Memory: `lb`, `lh`, `lw`, `sb`, `sh`, `sw`
- Control: `jal`, `jalr`, `lui`, `auipc`

### RV32M Extension
- `mul`, `div`, `rem`, `mulh`, `divu`, `remu`

---

## Output Example
```
LRU    Hit Rate: 80.7821%
pLRU   Hit Rate: 86.3902%
```
Supports mode-specific output (only LRU or pLRU).

---

## Run It Yourself
```bash
# Build
make

# Run
./cache_sim \
  --asm path/to/program.asm \
  --bin output/program.bin \
  --replacement 0
```

---

## Use Cases & Educational Value
This simulator is designed to help students, educators, and system-level engineers:

- Understand how cache works under different eviction policies
- Visualize instruction execution and memory access at the hardware level
- Debug and test RISC-V assembly code through step-by-step simulation
- Analyze cache efficiency using hit/miss statistics

The project can be used for coursework, teaching demonstrations, or even as a foundation for more advanced CPU/memory simulation tools.
