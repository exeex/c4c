# RV64 Aggregate Global Storage Runbook

Status: Active
Source Idea: ideas/open/309_rv64_aggregate_global_storage.md

## Purpose

Build the focused RV64 backend foundation for aggregate, array, union, and
struct global storage so prepared global-symbol accesses emit data symbols and
offset loads/stores instead of falling through to undefined-main or generic
unsupported behavior.

## Goal

Make the representative aggregate global storage path work for simple
zero-initialized aggregate or array symbols and integer-sized field accesses,
starting with the `src/00024.c` shape.

## Core Rule

Repair the prepared RV64 global storage mechanism, not a named testcase.
Do not special-case symbol names, fixed offsets, or c-testsuite filenames.

## Read First

- ideas/open/309_rv64_aggregate_global_storage.md
- build/rv64_c_testsuite_probe_v2/representative_evidence.md
- build/rv64_c_testsuite_probe_v2/classification.md
- build/rv64_c_testsuite_probe_v2/repair_order.md
- build/rv64_c_testsuite_probe_v2/classification_work/buckets/aggregate_global_storage.txt
- src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp
- src/backend/mir/riscv/codegen/prepared_global_memory_emit.hpp
- src/backend/mir/riscv/codegen/prepared_module_emit.cpp
- src/backend/bir/lir_to_bir/globals.cpp
- src/backend/bir/lir_to_bir/global_initializers.cpp
- tests/backend/CMakeLists.txt

## Current Targets

- Minimal representative: `src/00024.c`
- Focused backend cases:
  - aggregate or array global storage data emission
  - prepared global-symbol loads and stores using byte offsets
- Nearby bucket candidates for triage:
  - `src/00047.c`
  - `src/00048.c`
  - `src/00050.c`
  - `src/00091.c`
  - `src/00115.c`
  - `src/00146.c`
  - `src/00148.c`
  - `src/00163.c`
  - `src/00176.c`

## Non-Goals

- Do not repair the shared `.text`-only output contract here.
- Do not claim pointer-valued global initializers, function-pointer storage, or
  relocation semantics beyond aggregate data layout.
- Do not include floating global representation or floating load/use lowering.
- Do not include string literal storage or external call repair in the
  acceptance claim.
- Do not claim the full 19-case bucket passes without mechanism-level proof
  and nearby-case triage.
- Do not weaken supported-path expectations or classify aggregate cases away
  from the missing RV64 capability.

## Working Model

- Prepared BIR already records global-symbol memory accesses with byte offsets.
- RV64 data emission must preserve symbol spelling, section placement,
  alignment, and zero-initialized storage size for simple aggregate and array
  globals.
- Offset field load/store lowering should consume prepared global-symbol facts
  and emit normal RV64 address materialization plus offset memory operations.
- Full c-testsuite bucket sweeps are triage evidence; acceptance should rely on
  focused backend tests plus a narrow representative subset.

## Execution Rules

- Keep each implementation packet narrow enough for `cmake --build --preset default`
  plus a focused backend test subset.
- For code-changing backend packets, use the supervisor-selected proof command;
  default narrow routing is `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Add tests before or alongside behavior changes when they define a new backend
  contract.
- Preserve existing scalar i32 global behavior while broadening aggregate or
  array storage support.
- If a needed fix belongs to pointer globals, floating globals, strings,
  external calls, or the shared output contract, record it in `todo.md` and stop
  for supervisor routing instead of absorbing it into this plan.

## Ordered Steps

### Step 1: Establish Aggregate Global Storage Baseline and Scope

Goal: confirm the current RV64 aggregate-global failure shape and identify the
smallest backend contract needed before implementation.

Primary targets:

- build/rv64_c_testsuite_probe_v2/classification_work/buckets/aggregate_global_storage.txt
- build/rv64_c_testsuite_probe_v2/representative_evidence.md
- src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp
- tests/backend/CMakeLists.txt

Actions:

- Inspect representative evidence for `src/00024.c` and confirm the prepared
  global-symbol offsets that must survive to RV64 assembly.
- Inspect the current RV64 prepared global storage path and identify whether
  data emission, offset load/store lowering, or both are rejecting simple
  aggregate storage.
- Choose the first focused backend test shape for zero-initialized aggregate or
  array storage and integer field load/store behavior.
- Keep any prerequisite output-contract gap as a blocker note rather than
  repairing it here.

Completion check:

- `todo.md` records the exact first implementation target, owned files, and
  narrow proof command for the executor packet that will add or repair the
  focused backend contract.

### Step 2: Add Focused Backend Coverage for Aggregate Data Symbols

Goal: prove RV64 emits storage for a simple zero-initialized aggregate or array
global without relying on c-testsuite classification changes.

Primary targets:

- tests/backend/CMakeLists.txt
- tests/backend/case/
- RV64 backend test helpers already used by `backend_rv64_runtime_*` cases

Actions:

- Add the narrowest backend test case that requires a named aggregate or array
  global symbol to appear in RV64 output.
- Assert section placement, alignment when observable, symbol label spelling,
  and `.zero` size for zero-initialized storage.
- Keep pointer globals, floating globals, strings, and external calls outside
  the test contract.

Completion check:

- The focused backend test fails on the missing aggregate storage behavior
  before the implementation and passes after the corresponding code packet.

### Step 3: Emit Simple Aggregate and Array Global Storage

Goal: broaden RV64 prepared global data emission from scalar-only storage to
simple aggregate or array global storage represented in prepared BIR.

Primary targets:

- src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp
- src/backend/mir/riscv/codegen/prepared_global_memory_emit.hpp
- src/backend/bir/bir.hpp
- src/backend/bir/lir_to_bir/globals.cpp
- src/backend/bir/lir_to_bir/global_initializers.cpp

Actions:

- Reuse existing BIR global size, alignment, initializer element, and link-name
  facts instead of reparsing rendered LLVM text in RV64 codegen.
- Accept simple zero-initialized aggregate or array globals whose byte size and
  alignment are known.
- Emit `.bss` or equivalent zero storage for the full aggregate size with the
  preserved global label.
- Keep the existing scalar i32 global path working.

Completion check:

- Focused backend coverage proves the aggregate or array data symbol and zero
  storage are emitted, and scalar i32 global tests still pass.

### Step 4: Lower Prepared Global-Symbol Offset Loads and Stores

Goal: make RV64 prepared global memory access lowering handle integer-sized
field offsets inside the simple aggregate or array storage from Step 3.

Primary targets:

- src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp
- src/backend/mir/riscv/codegen/prepared_function_emit.cpp
- src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp
- tests/backend/CMakeLists.txt
- tests/backend/case/

Actions:

- Use prepared memory access records with `PreparedAddressBaseKind::GlobalSymbol`
  and byte offsets as the authority for addressed loads and stores.
- Support integer-sized field loads and stores that fit the current RV64 offset
  immediate constraints.
- Reject unsupported widths, volatility, thread-local storage, or unknown
  address-space cases explicitly through existing fallback behavior.
- Add focused coverage that stores to two aggregate fields and reads them back
  through offset loads.

Completion check:

- Focused backend coverage proves both offset stores and offset loads against a
  prepared aggregate global symbol.

### Step 5: Recheck Representative Bucket Cases

Goal: verify the repaired mechanism against the representative c-testsuite case
and nearby aggregate bucket candidates without claiming unrelated scopes.

Primary targets:

- src/00024.c
- build/rv64_c_testsuite_probe_v2/classification_work/buckets/aggregate_global_storage.txt
- tests/backend/CMakeLists.txt
- test_before.log
- test_after.log

Actions:

- Re-run the supervisor-selected narrow representative subset that includes
  `src/00024.c` after the focused backend tests pass.
- Sample nearby aggregate cases from the candidate list and record which are
  repaired, still blocked by out-of-scope features, or blocked by prerequisite
  output-contract behavior.
- Do not rewrite classification artifacts as the proof of progress.
- Leave any pointer, floating, string, external-call, or full-bucket follow-up
  as separate scoped work.

Completion check:

- `src/00024.c` no longer fails due to missing aggregate global data emission or
  dropped offset accesses, focused backend tests pass, and nearby candidate
  status is recorded without overclaiming the full bucket.
