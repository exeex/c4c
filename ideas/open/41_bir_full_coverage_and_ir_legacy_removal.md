# BIR Full Coverage and Legacy IR Removal

Status: Open
Last Updated: 2026-04-03

## Goal

Complete the removal of the legacy `ir.*` backend IR by expanding BIR to full
production coverage and migrating the x86/aarch64 emitters to consume BIR
directly, eliminating the need for `ir.hpp`, `ir.cpp`, `ir_printer.*`,
`ir_validate.*`, `lir_to_backend_ir.cpp`, and `bir_to_backend_ir.cpp`.

## Current Architecture

The current state after idea 06 cutover:

```
LIR
 ├─ Path A (BIR, bounded): lir_to_bir → bir_to_backend_ir → BackendModule(ir.*)
 └─ Path B (Legacy, default): lir_to_backend_ir ─────────→ BackendModule(ir.*)
                                                                    ↓
                                              x86/aarch64 emitters (consume ir.*)
```

Both paths converge on `BackendModule`, which is defined by `ir.hpp`.
BIR (`bir.hpp`) is currently only a thin experimental intermediate that
immediately converts back to `ir.*` structures via `bir_to_backend_ir.cpp`.

The problem: `ir.*` was never removed — it is still the emitter-facing format.
Idea 06 only changed the routing layer, not the IR contract.

## Why Remove ir.*

- Two competing backend IR definitions (`ir.hpp` + `bir.hpp`) create confusion
  about which is authoritative
- `lir_to_backend_ir.cpp` at 3,669 lines is the largest single file in the
  backend; it should not exist alongside a new lowering path indefinitely
- `bir_to_backend_ir.cpp` is a pure conversion artifact that only exists because
  BIR is not yet the emitter format; it adds a gratuitous conversion step
- The x86/aarch64 emitters including `ir.hpp` means the old format is still load-
  bearing at the lowest level; true BIR ownership requires fixing this

## Scale of Work

| File | Lines | Disposition |
|------|-------|-------------|
| `ir.hpp` | 1,026 | Replace with expanded `bir.hpp` |
| `ir.cpp` | ~100 | Remove |
| `ir_printer.cpp` | 257 | Replace with expanded `bir_printer.cpp` |
| `ir_validate.cpp` | 929 | Replace with expanded `bir_validate.cpp` |
| `lir_to_backend_ir.cpp` | 3,669 | Replace with expanded `lir_to_bir.cpp` |
| `bir_to_backend_ir.cpp` | 105 | Remove (no longer needed) |
| `bir.hpp` | 78 | Expand to full IR |
| `bir_printer.cpp` | 63 | Expand |
| `bir_validate.cpp` | 167 | Expand |
| `lir_to_bir.cpp` | 236 | Expand to full coverage |

## Gap Analysis

What `ir.*` covers that `bir.*` does not:

### Type System
- `i8` scalar type (currently only i32/i64/void in BIR)
- Pointer types (`Ptr`)
- Array types (`[N x T]`)

### Instruction Set
- Multiply, divide, remainder (only add/sub in BIR)
- All 10 comparison predicates (Slt, Sle, Sgt, Sge, Eq, Ne, Ult, Ule, Ugt, Uge)
- Load / store with sign and zero extension
- Phi nodes (SSA join points)
- Function calls with full ABI encoding
- Pointer difference operations
- Branch (unconditional and conditional)

### Module-Level Features
- Global variables with linkage (External, ExternWeak)
- String constants
- Local stack allocation (alloca tracking)
- Variadic functions
- Function linkage control (declare vs define)

### Infrastructure
- `ir_printer`: LLVM-format emission for 35+ instruction shapes
- `ir_validate`: 929 lines of production validation checks
- `lir_to_backend_ir`: 35+ specialized pattern recognizers/adapters

### Emitter Interface
- `x86/codegen/emit.hpp` and `aarch64/codegen/emit.hpp` both include `ir.hpp`
  and consume BackendModule structures directly

## Approach

### Phase 0: Audit and boundary-pin

- Compile with all `ir.hpp` includes stripped and record the full error list
- This produces the definitive gap list with exact line numbers
- Tag each gap as: data structure, instruction kind, or emitter interface

### Phase 1: Expand BIR data structures

- Extend `bir.hpp` to cover all types and instruction kinds found in Phase 0
- Keep both headers alive; do not migrate callers yet
- Add roundtrip tests: for each new BIR instruction kind, verify printer output

### Phase 2: Expand lir_to_bir.cpp

- Port pattern recognizers from `lir_to_backend_ir.cpp` into `lir_to_bir.cpp`
  one pattern cluster at a time (arithmetic → memory → control flow → calls)
- After each cluster: remove the cluster's corresponding guard in
  `try_lower_to_bir` and verify the bounded fallback is no longer triggered
- Gate on ctest --test-dir build -j8 passing after each cluster

### Phase 3: Expand bir_validate and bir_printer

- Port `ir_validate.cpp` checks into `bir_validate.cpp` as new instruction
  kinds come online
- Port `ir_printer.cpp` emission into `bir_printer.cpp` in lockstep

### Phase 4: Migrate emitters

- Update `x86/codegen/emit.hpp` and `aarch64/codegen/emit.hpp` to include
  `bir.hpp` instead of `ir.hpp`
- Change emitter code to consume BIR structures; this is mechanical after
  Phase 1 because BIR now mirrors the ir.* shape
- `bir_to_backend_ir.cpp` becomes dead code and is deleted

### Phase 5: Delete legacy IR

- Remove `lir_to_backend_ir.cpp`, `lir_to_backend_ir.hpp`
- Remove `bir_to_backend_ir.cpp`, `bir_to_backend_ir.hpp`
- Remove `ir.hpp`, `ir.cpp`, `ir_printer.*`, `ir_validate.*`
- Remove the legacy routing branch from `backend.cpp`
- Full ctest run: must stay at 2123/2123

## Constraints

- Do not delete ir.* files until Phase 4 is complete and emitters are migrated
- Do not break the bounded fallback window until the corresponding pattern
  cluster passes full regression
- Do not introduce a third IR format; the goal is one canonical backend IR
- Emitter migration (Phase 4) is the true safety gate — not lir_to_bir coverage

## Acceptance Criteria

- [ ] `bir.hpp` covers the same type and instruction surface as `ir.hpp` did
- [ ] `lir_to_bir.cpp` handles all patterns previously in `lir_to_backend_ir.cpp`
- [ ] `x86/codegen/emit.hpp` and `aarch64/codegen/emit.hpp` include only `bir.hpp`
- [ ] `ir.hpp`, `ir.cpp`, `ir_printer.*`, `ir_validate.*` are deleted
- [ ] `lir_to_backend_ir.cpp`, `bir_to_backend_ir.cpp` are deleted
- [ ] `backend.cpp` has a single lowering route (no legacy fallback branch)
- [ ] Full ctest suite passes at 2123/2123

## Non-Goals

- Changing the BIR data model to be semantically different from ir.* — the
  goal is format consolidation, not IR redesign
- Optimizing the backend IR while migrating — keep changes mechanical
- Touching LIR or HIR as part of this work

## Good First Step

Run Phase 0: strip the `ir.hpp` include from one emitter file and collect the
compilation errors. This gives the exact gap list with line-level precision and
prevents the plan from being based on a static audit that may be stale.
