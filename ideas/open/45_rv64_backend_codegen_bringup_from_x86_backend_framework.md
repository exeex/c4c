# RV64 Translated Codegen Integration

Status: Open
Last Updated: 2026-04-10

## Why This Idea

The repository already contains a translated `src/backend/riscv/` tree,
including assembler, codegen, linker, and target entry files. The main problem
is no longer "translate the Rust files." The translated files are already in
the tree. The real problem is that they are not meaningfully integrated:

- the current build does not appear to compile any `src/backend/riscv/*.cpp`
  units
- `src/backend/riscv/codegen/mod.cpp` is still a structural mirror, not a real
  dispatcher
- `src/backend/riscv/codegen/emit.cpp` exposes only a narrow helper subset and
  explicitly notes that the shared `RiscvCodegen` / `CodegenState` C++ surface
  does not exist yet
- the current `src/backend/riscv/` sources are largely a one-to-one translation
  from `ref/claudes-c-compiler/src/backend/riscv/`, so they should be treated
  as translated inventory rather than as a backend that already compiles or
  runs inside this repository

So the right framing for RV64 is the same one now used for x86 translated-code
work: treat the translated inventory as real source assets that must be wired
into the build and active backend path, instead of re-describing the problem as
greenfield bring-up.

The first real blocker is not "missing translation coverage." It is the absence
of a common RV64 codegen integration surface. Before broad feature work can be
done, the translated RV64 files need a shared header boundary similar to
`src/backend/x86/codegen/x86_codegen.hpp` so they can compile together.

## Source Context

- `src/backend/riscv/mod.cpp`
- `src/backend/riscv/codegen/mod.cpp`
- `src/backend/riscv/codegen/emit.cpp`
- `src/backend/riscv/codegen/*.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/`
- `src/backend/x86/codegen/`
- `src/backend/x86/codegen/x86_codegen.hpp`
- `plan.md`

## Current State

The `src/backend/riscv/` tree already contains translated source for:

- assembler
- codegen
- linker
- target entry

Current observed integration state:

- translated codegen siblings exist for ALU, memory, calls, returns, prologue,
  globals, casts, variadic, atomics, floating-point, intrinsics, inline asm,
  `i128`, `f128`, and peephole support
- the build does not currently list `src/backend/riscv/*.cpp` sources
- the RV64 codegen surface is therefore present as inventory but not active
- the translated RV64 units do not yet compile against a shared C++ header
  contract analogous to `src/backend/x86/codegen/x86_codegen.hpp`
- there is no documented or validated runtime lane yet for executing produced
  RV64 binaries, so bring-up cannot stop at "it compiles"

This means the highest-leverage work is not "translate more Rust," but:

- define the first real RV64 integration boundary
- create a common RV64 codegen header so translated units can compile against a
  shared surface
- bring translated units into the build in bounded slices
- make `riscv/codegen/emit.cpp` depend on real translated siblings instead of
  staying a narrow source mirror
- turn RV64 bring-up into a test-backed lane instead of a parked source dump

## Initial Integration Inventory

The active inventory for this idea starts from the translated RV64 surface that
already exists in-tree.

### Compact Summary

- translated top-level RV64 codegen units already present: `19`
- translated assembler subtree already present
- translated linker subtree already present
- current observed build integration: no explicit `src/backend/riscv/*.cpp`
  sources appear to be wired into `CMakeLists.txt`
- immediate conclusion: RV64 is blocked by integration, not by missing
  translation work

### Translated top-level RV64 codegen units

- `src/backend/riscv/codegen/alu.cpp`
- `src/backend/riscv/codegen/asm_emitter.cpp`
- `src/backend/riscv/codegen/atomics.cpp`
- `src/backend/riscv/codegen/calls.cpp`
- `src/backend/riscv/codegen/cast_ops.cpp`
- `src/backend/riscv/codegen/comparison.cpp`
- `src/backend/riscv/codegen/emit.cpp`
- `src/backend/riscv/codegen/f128.cpp`
- `src/backend/riscv/codegen/float_ops.cpp`
- `src/backend/riscv/codegen/globals.cpp`
- `src/backend/riscv/codegen/i128_ops.cpp`
- `src/backend/riscv/codegen/inline_asm.cpp`
- `src/backend/riscv/codegen/intrinsics.cpp`
- `src/backend/riscv/codegen/memory.cpp`
- `src/backend/riscv/codegen/mod.cpp`
- `src/backend/riscv/codegen/peephole.cpp`
- `src/backend/riscv/codegen/prologue.cpp`
- `src/backend/riscv/codegen/returns.cpp`
- `src/backend/riscv/codegen/variadic.cpp`

### Additional translated RV64 subsystems already present

- assembler subtree under `src/backend/riscv/assembler/`
- linker subtree under `src/backend/riscv/linker/`
- target entry at `src/backend/riscv/mod.cpp`

This list is the starting execution queue for RV64 integration. Future RV64
work should consume items from this inventory rather than treating the tree as
missing translation work.

## Primary Bring-Up Constraint

The current RV64 code is mostly a one-to-one translation of
`ref/claudes-c-compiler/src/backend/riscv/`. That is useful as source
material, but it is not sufficient for repository integration.

The first concrete blocker is the absence of a shared C++ header for RV64
codegen ownership. The x86 backend already established the pattern with
`src/backend/x86/codegen/x86_codegen.hpp`: sibling translation units compile
against a common declaration surface instead of depending on ad hoc
`emit.cpp`-local declarations.

RV64 needs the same transitional boundary:

- a `riscv_codegen.hpp`-style shared header, modeled after the x86 surface
- shared forward declarations and enums needed by translated RV64 slices
- a stable place for `RiscvCodegen`, state, helper types, and cross-file method
  declarations
- enough API discipline that translated files can be compiled incrementally
  instead of remaining isolated mirrors

Without that header seam, the RV64 tree will continue to be source inventory
that cannot be compiled or meaningfully connected.

## First Activation Slice

If this idea becomes active, the first slice should stay narrow:

1. create the first shared RV64 codegen header, using
   `src/backend/x86/codegen/x86_codegen.hpp` as the model for the transitional
   integration seam
2. confirm the exact `CMakeLists.txt` delta needed to compile the first RV64
   codegen cluster
3. choose the minimum ownership cluster beneath `riscv/codegen/emit.cpp`
   instead of the whole tree
4. prefer this order for the first cluster:
   - `mod.cpp`
   - `emit.cpp`
   - `returns.cpp`
   - `prologue.cpp`
   - `asm_emitter.cpp`

Rationale:

- the first real unlock is a common header, not a larger translation import
- these files are the shortest path to a real RV64 codegen entry boundary
- they avoid widening immediately into memory, calls, atomics, and floating
  point
- they match the same integration-first logic now used by idea 43

## Goal

Turn the translated RV64 backend surface into a real staged integration lane:

- establish a shared RV64 codegen header boundary so translated files can
  compile together
- compile translated RV64 units in controlled slices
- establish a real codegen ownership boundary
- make `riscv/codegen/emit.cpp` and sibling units part of an active backend path
- connect backend functionality slice by slice in the same spirit as the x86
  bring-up path
- prove each connected slice with focused tests
- ultimately run the RV64 backend `c-testsuite` lane successfully, using
  `qemu` as the execution environment for produced binaries

## Non-Goals

- do not treat this as a greenfield RV64 rewrite
- do not translate more Rust just to create more parked inventory
- do not widen this into a full all-target backend redesign
- do not silently absorb unrelated x86 or shared-BIR cleanup work
- do not claim RV64 support is done merely because files compile; bring-up is
  incomplete until feature wiring and execution-backed tests exist
- do not require assembler/linker/runtime integration for every subsystem up
  front before the first codegen seam lands

## Working Model

- use the x86 translated-code integration lane as the primary process model
- prefer integrating existing translated units over inventing new local
  stand-ins
- create one transitional shared-header seam first, then move translated units
  behind it
- move one bounded RV64 integration seam at a time
- treat assembler and linker as follow-on consumers unless codegen integration
  truly requires them
- keep the first ownership transfers centered on `riscv/codegen/`
- require tests for each newly connected slice instead of deferring all
  validation to the end

## Test Strategy

RV64 validation needs to grow in the same order as functionality:

- compile tests prove the shared header and translation-unit wiring are valid
- targeted backend or asm-shape tests prove each newly connected feature slice
- backend execution tests prove produced RV64 binaries behave correctly
- the final acceptance gate is the RV64 backend `c-testsuite` lane

Because RV64 binaries do not run natively in the usual host environment here,
the execution lane must use `qemu`. That requirement should be treated as part
of bring-up scope, not as a later infrastructure detail.

## Proposed Plan

### Step 1: Audit translated RV64 coverage

Goal: make the translated RV64 inventory explicit by role and current
integration state.

Completion check:

- the translated-unit inventory is explicit
- the first integration slice is chosen from that inventory

### Step 2: Introduce a shared RV64 codegen header

Goal: create the common C++ integration seam that the translated RV64 units
currently lack.

Concrete actions:

- add a shared RV64 codegen header modeled after
  `src/backend/x86/codegen/x86_codegen.hpp`
- move the minimum shared declarations out of `emit.cpp`-local context and into
  that header
- define enough forward declarations, enums, helper types, and method
  declarations for the first RV64 slice to compile together

Completion check:

- translated RV64 units compile against a common header surface
- the RV64 codegen surface no longer depends on ad hoc local declarations as
  its primary integration mechanism

### Step 3: Bring translated RV64 codegen units into the build

Goal: stop treating RV64 codegen as parked source inventory.

Concrete actions:

- update the build to compile the first bounded RV64 codegen slice
- resolve the minimum shared header/type surface needed for that slice
- avoid broad rewrites; connect only the units needed for the active seam

Completion check:

- at least one translated RV64 codegen unit or cluster is compiled
- the chosen slice is reachable from a real RV64 backend entry boundary

### Step 4: Establish a real RV64 codegen ownership seam

Goal: make `riscv/codegen/emit.cpp` depend on real translated siblings instead
of acting as a narrow helper mirror.

Completion check:

- at least one bounded RV64 codegen path uses real sibling units
- the active RV64 entry surface is no longer pure structural inventory

### Step 5: Connect backend features in x86-style slices

Goal: wire RV64 functionality the same way the x86 backend was brought up,
through bounded ownership slices instead of a single giant merge.

Concrete actions:

- connect prologue / epilogue and return lowering first
- then connect memory, ALU, comparisons, calls, globals, casts, variadic,
  atomics, floating-point, inline asm, intrinsics, `i128`, `f128`, and
  peephole support in leverage order
- update dispatcher / entry wiring as each slice becomes real

Completion check:

- the RV64 backend no longer stops at a minimal compile-only seam
- each connected subsystem has an explicit ownership point and active call path

### Step 6: Add tests per feature slice

Goal: make RV64 bring-up measurable and monotonic.

Concrete actions:

- add targeted tests as each subsystem is connected instead of postponing test
  coverage
- use compile-only, asm-shape, and backend execution tests where appropriate
- keep tests aligned with the active slice so failures identify the missing
  integration step directly

Completion check:

- each newly connected RV64 slice has at least one focused test
- RV64 progress can be tracked by increasing tested feature coverage, not just
  by file count

### Step 7: Bring up end-to-end RV64 execution under qemu

Goal: validate produced RV64 binaries in a real execution loop.

Concrete actions:

- define the required `qemu`-based execution path for RV64 test runs
- integrate the produced executable / linker path far enough that backend tests
  can run binaries, not only inspect emitted assembly
- document the RV64 runtime dependency explicitly so the execution lane is part
  of the plan rather than an unstated follow-on

Completion check:

- RV64 backend-produced binaries can be executed through `qemu`
- at least one backend execution test uses the RV64 runtime lane successfully

### Step 8: Run the RV64 c-testsuite backend lane to green

Goal: finish bring-up by making the backend test lane pass, not merely by
compiling translated source files.

Concrete actions:

- expand from focused feature tests to the RV64 backend `c-testsuite` lane
- drive failures down by missing feature area, using the earlier slice tests as
  regression guards
- treat this as the final acceptance gate for the idea

Completion check:

- the RV64 backend `c-testsuite` lane passes
- remaining failures, if any, are explicitly outside the scope of this idea
