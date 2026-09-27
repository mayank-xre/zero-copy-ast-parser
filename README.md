# High-Throughput Zero-Allocation AST Parser & SPSC Queue

An ultra-low latency, zero-allocation C++ pipeline designed for high-frequency parsing of parenthesized expressions (S-expressions) over a lock-free Single-Producer Single-Consumer (SPSC) ring buffer.

Achieves **~1.0 GB/s** single-core streaming parser throughput (up to **25.8 Million msgs/sec**) by combining cache-line aligned data structures, acquire-release atomic memory orderings, and an iterative (non-recursive) AST construction algorithm.

---

## Key Features

* **Zero Memory Allocations on Hot Path:** All tokens and Abstract Syntax Tree (AST) nodes are placed directly into cache-aligned, pre-allocated thread-local arrays.
* **Lock-Free SPSC Ring Buffer:** Template-based circular queue (`SPSC<BUF_SZ, C_SZ, B_SZ>`) utilizing atomic acquire-release semantics, cache-line padding (`alignas(128)`) to eliminate false sharing, and batch publishing.
* **Non-Recursive AST Builder:** Converts S-expressions into an explicit Left-Child / Right-Sibling AST using an iterative stack array (`levels[100]`), avoiding call-stack overhead and recursion limits.
* **Optimized Hardware Synchronization:** Integrated ARM (`yield`) and x86 (`pause`) CPU instruction hints in wait loops to prevent core starvation and memory bus lockup.
* **Verification Hash:** Built-in validation checksumming (`get_hash()`) to guarantee correctness while preventing compiler dead-code elimination (DCE) during benchmarking.

---

## Performance & Benchmarks

Tested with 10,000,000 messages across 100,000 unique payloads pinned to Apple M5 P-cores via `QOS_CLASS_USER_INTERACTIVE`.

| Workload Type | Avg Payload Size | Throughput (msgs/sec) | GB Throughput | Avg Latency / Message |
| :--- | :--- | :--- | :--- | :--- |
| **Short Messages** | ~36.0 bytes | **28.17 M/s** | **1.015 GB/s** | **35.5 ns** |
| **General Messages** | ~87.3 bytes | **12.29 M/s** | **1.073 GB/s** | **81.4 ns** |
| **Mixed Workload** | ~223.3 bytes | **4.55 M/s** | **1.016 GB/s** | **219.8 ns** |
| **Nested Expressions**| ~205.9 bytes | **4.28 M/s** | **0.882 GB/s** | **233.4 ns** |
| **Long Messages** | ~1459.1 bytes | **0.72 M/s** | **1.049 GB/s** | **1390.9 ns** |

*Note: The parser maintains a consistent ~1.0–1.07 GB/s byte-processing throughput across varying payload complexities.*

## Architecture Overview

### 1. Data Structure Alignment (`ExprStructs.hpp`)

* **`Expression` (16 bytes):** Descriptor containing a raw buffer pointer (`const char* exp`) and buffer length (`int len`). Packed into 16-byte alignment (`alignas(16)`) so that 4 descriptors fit into standard 64-byte L1 cache lines.
* **`Node` (16 bytes):** Compact tree node representing either a `LIST` or `ATOM`:
  ```cpp
  struct alignas(16) Node {
      int32_t left{0};    // Index of left child node
      int32_t next{0};    // Index of right sibling node
      uint16_t offset{0}; // Byte offset in string
      uint16_t size{0};   // Byte size of token
      uint8_t type{0};    // 0 = LIST, 1 = ATOM
  };
  ```

### 2. Lock-Free SPSC Ring Buffer (`SPSC.hpp`)

* **Acquire-Release Semantics:** Synchronizes producer and consumer without full memory barriers (`std::memory_order_acquire` / `std::memory_order_release`).
* **Cache Line Isolation:** `head`, `tail`, and queue state structures are separated across 128-byte alignment boundaries to avoid L1/L2 cache-line bouncing (false sharing) between core execution contexts.
* **Hybrid Batching & Stall Prevention:** Tail pointer updates are published in batches of 32 (`B_SZ`) under load, but immediately flush if the queue was previously empty.

### 3. Lexer & Iterative AST Builder (`Lexer.hpp`)

1. **`lex()`**: Linear scan over byte buffers isolating tokens (opening parenthetical `-1`, closing parenthetical `-2`, or `[start_idx, end_idx]` atom index pairs).
2. **`TreeParse()`**: Non-recursive tree constructor. Emulates stack depth via a flat `int levels[100]` array to construct the left-child/right-sibling pointer topology in linear time $O(N)$.

---

## Project Structure

```text
.
├── ExprStructs.hpp    # Aligned Expression descriptors & AST Node definitions
├── Lexer.hpp          # Lexical scanner & iterative TreeParse AST algorithm
├── SPSC.hpp           # Lock-free SPSC circular queue with atomic synchronization
├── Synthesize.hpp     # Synthetic payload generator for benchmarking workloads
└── main.cpp           # Benchmark execution framework & verification suite
```

---

## Build & Run Instructions

### Prerequisites
* C++20 compliant compiler (`g++` 10+ or `clang++` 12+)
* CMake 3.16+ (Optional) or Direct CLI compilation

### Direct Compilation (Optimized Release)

To compile with maximum compiler optimizations and execution tuning:

```bash
# Using Clang++ or G++
clang++ -std=c++20 -O3 -march=native -flto=thin main.cpp -o benchmark
g++ -std=c++20 -O3 -march=native -flto=thin main.cpp -o benchmark 

# Run the benchmark suite
./benchmark_run
```

---
