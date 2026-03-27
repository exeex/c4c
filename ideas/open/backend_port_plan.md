# Backend Port Plan

## Goal

Port `ref/claudes-c-compiler/src/backend/` to `src/backend/`, rewriting from Rust to C++, consuming the existing LIR (`src/codegen/lir/`) as input instead of LLVM IR text emission.

Targets: **aarch64**, **x86-64**, **rv64** — Linux only (Ubuntu).

## Why This Work

The current pipeline ends at `lir_printer.cpp` which emits LLVM IR text, then shells out to `llc` + `as` + `ld`. This creates a hard dependency on the LLVM toolchain at runtime. A native backend:

- removes the LLVM runtime dependency
- enables tighter integration with the LIR (no serialization round-trip)
- gives us control over codegen quality and compilation speed
- is required for self-hosting

## Target Priority

1. **AArch64** — primary dev/test machine is macOS ARM (Linux cross via Docker)
2. **x86-64** — most common server target, CI machines
3. **RISC-V 64** — forward-looking, QEMU user-mode testing

## Architecture Overview

```
LirModule (src/codegen/lir/ir.hpp)
    │
    ▼
src/backend/
  ├── common/
  │   ├── target.hpp           Target enum + factory
  │   ├── arch_codegen.hpp     ArchCodegen abstract base class
  │   ├── codegen_state.hpp    Output buffer, stack slot map, reg cache
  │   ├── generation.hpp       LIR dispatch → ArchCodegen virtual calls
  │   ├── stack_layout.hpp     Three-tier slot allocation
  │   ├── regalloc.hpp         Linear-scan register allocator
  │   ├── liveness.hpp         Live interval computation
  │   ├── call_abi.hpp         Unified ABI classification
  │   └── peephole.hpp         Shared peephole utilities
  ├── aarch64/
  │   ├── codegen/             Instruction selection (emit, prologue, calls, alu, memory, float, ...)
  │   ├── assembler/           GNU ARM syntax parser → ELF64 .o encoder
  │   └── linker/              AArch64 ELF64 linker
  ├── x86/
  │   ├── codegen/             Instruction selection (AT&T syntax emission)
  │   ├── assembler/           AT&T syntax parser → ELF64 .o encoder (REX/ModRM/SIB)
  │   └── linker/              x86-64 ELF64 linker
  ├── riscv64/
  │   ├── codegen/             Instruction selection (RV asm emission)
  │   ├── assembler/           RV syntax parser → ELF64 .o encoder
  │   └── linker/              RISC-V ELF64 linker
  └── emit.cpp                 Entry: LirModule → target selection → codegen → asm/link
```

## LIR Attachment Point

The LIR currently has two instruction flavours in `LirInst` (a `std::variant`):

- **Stage 0**: structured (`LirLoad`, `LirBinary`, `LirCall`, ...) with `LirValueId` references
- **Stage 3**: string-operand ops (`LirLoadOp`, `LirBinOp`, `LirCallOp`, ...) designed for LLVM text emission

`stmt_emitter.cpp` currently produces Stage 3 instructions. The backend dispatch should use `std::visit` on the `LirInst` variant, routing each alternative to the corresponding `ArchCodegen` virtual method.

Long-term, migrating `stmt_emitter` to produce Stage 0 instructions would give the backend richer type info and avoid string parsing. But that is a separate refactor — not a prerequisite.

## Key Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Trait → C++ | Abstract base class with pure virtual methods | Straightforward; CRTP is premature optimization |
| Assembly output | `std::string` buffer | Simple; built-in assembler replaces this later |
| ABI classification | Shared `classify_arg()` / `classify_ret()` interface, per-target impl | Three targets share the pattern, differ in rules |
| Register allocation | Skip initially (all stack slots), add linear-scan in Phase 2/5 | Reduces initial complexity |
| Assembler + linker | External `as` + `ld` first, built-in last | Validate codegen correctness before encoding correctness |

## Implementation Phases

### Phase 0 — Skeleton + External Toolchain Fallback

Scope:
- `ArchCodegen` base class with pure virtuals for each LirInst kind
- `CodegenState`: asm output buffer, stack slot map, label counter
- `Target` enum (`AArch64`, `X86_64`, `RV64`) + factory function
- `generation.cpp`: walk `LirModule` → dispatch `LirInst` to `ArchCodegen`
- External fallback: fork+exec `as` and `ld` (or `cc` for linking)
- CMake integration: `src/backend/` added to build

Validation:
- Compile `int main() { return 0; }` → produce working executable via external as/ld
- Unit test: `Target::create()` returns correct subclass

### Phase 1 — AArch64 Basic Codegen

Scope:
- Prologue/epilogue: `stp x29,x30,[sp,#-N]!` / `ldp` / `ret`
- Stack layout Tier 1: alloca → fixed slots (`[x29, #-N]` addressing)
- Integer ALU: add/sub/mul/sdiv/udiv, shift, bitwise
- Load/store: ldr/str with register+offset addressing
- Comparisons: cmp + cset / b.cond
- Control flow: b / b.cond / cbz/cbnz (for condbr), switch via cmp chain
- Function calls: AAPCS64 (x0-x7 GP args, d0-d7 FP args)
- Returns: value in x0 (integer) or d0 (float)
- Globals: adrp + add :lo12:
- Casts: sxtw/uxtw/scvtf/fcvtzs/fmov etc.
- Float basics: fadd/fsub/fmul/fdiv (double/float via d/s registers)

Validation:
- c-testsuite simple tests, run natively on ARM Mac (Docker Linux) or QEMU
- Incremental: start with return-only, then arithmetic, then calls, then control flow

### Phase 2 — AArch64 Completion

Scope:
- Stack layout Tier 2+3: liveness-packed + block-local slot sharing
- Register allocation: linear-scan (callee-saved x20-x28, caller-saved x9-x15)
- Peephole: redundant store/load elimination, copy propagation
- Variadic: va_start/va_arg (GP+FP register save area on stack)
- i128: register pair ops (x0:x1) + libcall fallback
- F128: soft-float libcalls (`__addtf3`, `__multf3`, ...)
- Inline asm: constraint parsing (`r`, `w`, `m`, `i`)
- Atomics: ldxr/stxr exclusive loops, dmb barriers

Validation:
- llvm_gcc_c_torture allowlist, target >90% pass rate on AArch64
- QEMU or native execution

### Phase 3 — x86-64 Basic Codegen

Scope:
- Prologue/epilogue: push rbp / mov rbp,rsp / sub rsp,N / pop rbp / ret
- Stack layout Tier 1: alloca → fixed slots (`-N(%rbp)` addressing)
- Integer ALU: add/sub/imul/idiv, shift, bitwise (two-operand form)
- Load/store: mov with `offset(%rbp)` addressing
- Comparisons: cmp + setcc / jcc
- Control flow: jmp / jcc / switch via cmp chain or jump table
- Function calls: System V AMD64 (rdi,rsi,rdx,rcx,r8,r9 + xmm0-xmm7)
- Returns: rax (integer) or xmm0 (float)
- Globals: RIP-relative addressing (`symbol(%rip)`)
- Casts: movsx/movzx/cvtsi2sd/cvttsd2si etc.
- Float basics: SSE movsd/addsd/subsd/mulsd/divsd

Validation:
- c-testsuite simple tests on x86-64 Linux (CI or Docker)

### Phase 4 — x86-64 Completion

Scope:
- Stack layout Tier 2+3
- Register allocation: linear-scan (callee-saved rbx,r12-r15; caller-saved r10,r11,r8,r9)
- Peephole: copy propagation, dead store elim, tail call opt (15-pass suite from ref)
- Variadic: register save area (AL count for XMM args)
- i128: rax:rdx pair ops + libcall fallback
- F128: x87 80-bit extended (`fldt`/`fstpt`)
- Inline asm: constraint parsing (`r`, `m`, `i`, `a`, `b`, `c`, `d`, `S`, `D`, `x`)
- Atomics: lock prefix + cmpxchg

Validation:
- llvm_gcc_c_torture allowlist, target >90% pass rate on x86-64

### Phase 5 — RISC-V 64 Full Codegen

Scope:
- Prologue/epilogue: addi sp / sd ra,s0 / ld / ret
- Stack layout: all three tiers, `-N(s0)` addressing
- Integer ALU: add/sub/mul/div (no imm mul — li + mul)
- Load/store: ld/sd/lw/sw (12-bit offset limit → lui/addi for large offsets)
- Control flow: j / beq/bne/blt/bge
- Function calls: LP64D (a0-a7 GP, fa0-fa7 FP; variadic routes FP through GP)
- Float: fadd.d/fsub.d/fmul.d/fdiv.d
- Globals: auipc + addi (or `la` pseudo)
- Register allocation: linear-scan (s1-s11 callee-saved, t0-t6 caller-saved)
- Peephole: 3-phase (local pattern match, global copy prop, local cleanup)
- i128: GP register pair + libcall
- F128: soft-float libcalls through GP pairs
- Inline asm: constraint parsing (`r`, `m`, `i`)
- Atomics: AMO instructions + LR/SC loops

Validation:
- QEMU user-mode: `qemu-riscv64 -L /usr/riscv64-linux-gnu/ ./a.out`
- llvm_gcc_c_torture allowlist, target >90% pass rate

### Phase 6 — Built-in Assembler

Per-target, in priority order:

1. **AArch64**: fixed 32-bit instruction encoding → ELF64 .o
   - Parse GNU ARM assembly syntax
   - Encode each instruction to 4 bytes
   - Emit sections, symbols, relocations

2. **x86-64**: variable-length encoding → ELF64 .o
   - Parse AT&T syntax (register operands, immediates, memory)
   - Infer operand sizes, emit REX/VEX prefixes, ModRM+SIB
   - Most complex encoder of the three

3. **RV64**: fixed 32-bit + optional compressed 16-bit → ELF64 .o
   - Parse RISC-V canonical syntax
   - Encode base + C extension instructions

Validation:
- Byte-level comparison: `objdump -d ours.o` vs `objdump -d gas.o`
- Round-trip: assemble → disassemble → re-assemble → compare

### Phase 7 — Built-in Linker

Shared infrastructure (`linker_common/`):
- ELF64 object parser (sections, symbols, relocations)
- Static archive (.a) reader with iterative member pulling
- Section merging + virtual address assignment
- Symbol resolution (strong/weak, local/global)
- CRT startup linking (crt1.o + crti.o + crtn.o + libc)

Per-target relocation application:
- AArch64: ADRP/ADD/LDR relocations, PLT/GOT
- x86-64: GOTPCREL, PLT32, PC32, TLS relocations
- RV64: HI20/LO12, PLT/GOT, linker relaxation

Dynamic linking (optional, lower priority):
- .dynsym / .dynstr / .gnu.hash
- PLT/GOT construction
- .interp + PT_INTERP for dynamic executables

Validation:
- Functional comparison: built-in linker output vs `ld` output, same test results
- Static-linked hello world as first milestone

## Testing Strategy

### Test Tiers

| Tier | What | How | When |
|------|------|-----|------|
| Unit | ABI classification, immediate encoding, stack slot math | GoogleTest | Every phase |
| Snapshot | Expected asm text for known LIR input | Golden file diff | Phase 1+ |
| End-to-end native | Compile C → run → check exit code/stdout | Shell script + ctest | Phase 1+ |
| End-to-end cross | Cross-compile → QEMU user-mode → check | ctest + qemu wrapper | Phase 3+ (x86), Phase 5 (rv64) |
| Torture suite | llvm_gcc_c_torture + c-testsuite | Existing ctest infra | Phase 2+ |
| Asm verification | Built-in vs `as` binary comparison | objdump diff | Phase 6 |
| Link verification | Built-in vs `ld` functional comparison | Run both, compare | Phase 7 |

### Cross-Compilation Setup

```bash
# AArch64 (if not native)
sudo apt install gcc-aarch64-linux-gnu qemu-user-static
qemu-aarch64 -L /usr/aarch64-linux-gnu/ ./a.out

# x86-64 (if not native)
sudo apt install gcc-x86-64-linux-gnu qemu-user-static
qemu-x86_64 -L /usr/x86-64-linux-gnu/ ./a.out

# RISC-V 64
sudo apt install gcc-riscv64-linux-gnu qemu-user-static
qemu-riscv64 -L /usr/riscv64-linux-gnu/ ./a.out
```

### Incremental Test Milestones

Phase 0: `return 0` → exit code 0
Phase 1: `return 2+3` → exit code 5; `if/else` → correct branch; `fib(10)` → 55
Phase 2: torture suite >90%
Phase 3: same milestones on x86-64
Phase 4: torture suite >90% on x86-64
Phase 5: torture suite >90% on rv64
Phase 6: `objdump` match for all passing tests
Phase 7: static-linked executables run identically

## Guardrails

- Do not modify existing LIR data structures for backend convenience in early phases. Adapt the backend to consume what exists.
- Do not modify vendored test-suite sources.
- Each phase should be independently testable — no big-bang integration.
- Prefer external tool fallback over partial built-in implementation. A correct `as`-assembled binary is better than a buggy hand-encoded one.
- Keep per-target code isolated. Shared code goes in `common/`; target-specific code never `#include`s another target.
- One mechanism per commit. Do not mix "add AArch64 float ops" with "refactor stack layout".

## Good First Patch

Phase 0 skeleton: `ArchCodegen` base class, `Target` factory, `CodegenState`, and the external as/ld fallback. Validate with `int main() { return 0; }` producing a working AArch64 Linux executable.
