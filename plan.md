# BIR Full Coverage and Legacy IR Removal

Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Activated from: ideas/closed/40_target_profile_and_execution_domain_foundation.md

## Purpose

Finish idea 41 in single-threaded cleanup mode. The repo is past the
"inventory" stage: public legacy entry points are already locked down, the
first BIR call/global/string/local-slot seams exist, and active tests have
mostly stopped including legacy IR utilities directly. The remaining work is to
finish the BIR contract, switch the last real consumers, and then delete the
legacy backend IR without widening scope again.

## Goal

Reach a state where `BackendModule` is no longer a live production contract for
x86/aarch64 emission or shared call decoding, making `lir_to_backend_ir.*` and
`ir.*` deletable as follow-through instead of load-bearing infrastructure.

## Core Rule

Expand BIR only where a concrete legacy seam still needs it, then migrate that
consumer immediately. Do not add speculative BIR shape, do not reopen
subagent-style parallel lanes, and do not delete legacy files until their last
real consumer is gone.

## Read First

- [`ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md`](/workspaces/c4c/ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md)
- [`src/backend/bir.hpp`](/workspaces/c4c/src/backend/bir.hpp)
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
- [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)

## Current State

Already complete on the branch:

- shared backend public `BackendModule` emission entry is gone
- [`src/backend/backend.cpp`](/workspaces/c4c/src/backend/backend.cpp) no
  longer directly includes `lir_to_backend_ir.hpp`
- [`src/backend/backend.hpp`](/workspaces/c4c/src/backend/backend.hpp) no
  longer exposes `BackendModule`
- parked legacy backend-IR tests are out of the tree
- BIR now has bounded direct-call, declared-direct-call, `globals`,
  `string_constants`, and `local_slots` shape
- active test consumers no longer directly include `ir_printer.hpp` /
  `ir_validate.hpp`
- [`src/backend/lowering/extern_lowering.hpp`](/workspaces/c4c/src/backend/lowering/extern_lowering.hpp)
  and [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
  have already shed some dead legacy header exposure

Still blocking closure:

- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  still exposes many `parse_backend_minimal_*_module(const BackendModule&)`
  seams
- x86/aarch64 emitters still have many private `BackendModule` helpers
- [`src/backend/lowering/lir_to_backend_ir.*`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
  is still live lowering glue
- [`src/backend/ir.hpp`](/workspaces/c4c/src/backend/ir.hpp),
  [`src/backend/ir.cpp`](/workspaces/c4c/src/backend/ir.cpp),
  [`src/backend/ir_printer.hpp`](/workspaces/c4c/src/backend/ir_printer.hpp),
  [`src/backend/ir_printer.cpp`](/workspaces/c4c/src/backend/ir_printer.cpp),
  [`src/backend/ir_validate.hpp`](/workspaces/c4c/src/backend/ir_validate.hpp),
  [`src/backend/ir_validate.cpp`](/workspaces/c4c/src/backend/ir_validate.cpp)
  still exist
- LLVM rescue behavior still exists in
  [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
  and [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)

## Non-Goals

- do not reintroduce worker lanes or `todoA.md` style packets
- do not use `riscv64` passthrough behavior as the primary oracle for backend
  coverage
- do not delete LLVM rescue paths until backend-native coverage is actually
  ready for that cut
- do not physically delete `ir.*` or `lir_to_backend_ir.*` while emitter or
  shared helper code still needs them

## Working Model

Treat the remaining work as three serial buckets:

1. finish bounded BIR contract gaps that correspond to real consumers
2. switch `call_decode` and emitter helpers to the new BIR shape
3. delete the now-dead legacy lowering / legacy IR files

The current highest-value missing contracts are:

- `extern`
- address / global classification
- local-slot read/write shape

Those unlock the next real consumer migrations in:

- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)

## Execution Rules

- keep each change slice behavior-preserving unless the source idea explicitly
  requires a behavior change
- prefer converting one concrete legacy seam at a time over broad rewrites
- when a new BIR field/helper lands, migrate at least one real consumer before
  moving on to the next schema addition
- keep header-dependency trimming opportunistic but secondary to real contract
  migration
- use targeted backend validation while the broad external aarch64 `c-testsuite`
  signal remains noisy; only use broad `ctest -R backend` when it is informative

## Ordered Steps

### Step 1: Finish the next bounded BIR contract

Goal:
- add only the missing BIR shape needed for the next live legacy seam

Primary targets:
- [`src/backend/bir.hpp`](/workspaces/c4c/src/backend/bir.hpp)
- [`src/backend/bir_printer.cpp`](/workspaces/c4c/src/backend/bir_printer.cpp)
- [`src/backend/bir_validate.cpp`](/workspaces/c4c/src/backend/bir_validate.cpp)
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)

Concrete actions:
- extend BIR for `extern`, address/global classification, and local-slot
  read/write only as needed
- add or update the smallest parser/validator/printer support needed to make
  the new contract concrete
- keep additions narrow enough that the next consumer switch is obvious

Completion check:
- a new BIR-facing helper exists for a currently-legacy call/global/local-slot
  seam
- targeted objects that own the new shape compile

### Step 2: Switch shared helper seams off `BackendModule`

Goal:
- migrate `call_decode` first, because it is the narrowest shared seam still
  propagating `BackendModule`

Primary targets:
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- [`src/backend/lowering/call_decode.cpp`](/workspaces/c4c/src/backend/lowering/call_decode.cpp)
- [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp) if cleanup is needed after the switch

Concrete actions:
- replace one legacy `parse_backend_minimal_*_module(...)` family at a time
  with BIR-facing helpers
- remove dead includes and dead adapter-only helpers as each family moves
- keep `lir_to_backend_ir.*` only as long as some real caller still depends on
  it

Completion check:
- at least one previously `BackendModule`-only parser family is now BIR-first
- any corresponding dead legacy helper or include has been removed

### Step 3: Migrate the first emitter helper clusters

Goal:
- start consuming the new BIR contract inside the native emitters

Primary targets:
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)

Concrete actions:
- follow the migration order already identified in file comments
- prefer the first clusters that are unlocked by the new BIR contract:
  string/global/external-call/local-slot related helpers
- remove thin `BackendModule` pass-through wrappers whenever a BIR-native
  helper replaces them

Completion check:
- at least one real emitter helper cluster on each active target no longer
  requires `BackendModule`
- the file-level `BackendModule` footprint measurably shrinks

### Step 4: Collapse the legacy lowering route

Goal:
- make `lir_to_backend_ir.*` obviously dead before deleting it

Primary targets:
- [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
- [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
- [`src/backend/backend.cpp`](/workspaces/c4c/src/backend/backend.cpp)

Concrete actions:
- remove or stub the remaining public/semipublic legacy lowering entry points
- move any last useful logic into BIR-facing code first
- verify no active consumer still needs `LirAdapterError` or `BackendModule`
  from this route

Completion check:
- `lir_to_backend_ir.*` has no live non-legacy consumer
- the repo can be prepared for physical deletion of the legacy lowering route

### Step 5: Delete legacy IR files

Goal:
- remove `ir.*` only after the last production consumer is gone

Primary targets:
- [`src/backend/ir.hpp`](/workspaces/c4c/src/backend/ir.hpp)
- [`src/backend/ir.cpp`](/workspaces/c4c/src/backend/ir.cpp)
- [`src/backend/ir_printer.hpp`](/workspaces/c4c/src/backend/ir_printer.hpp)
- [`src/backend/ir_printer.cpp`](/workspaces/c4c/src/backend/ir_printer.cpp)
- [`src/backend/ir_validate.hpp`](/workspaces/c4c/src/backend/ir_validate.hpp)
- [`src/backend/ir_validate.cpp`](/workspaces/c4c/src/backend/ir_validate.cpp)

Concrete actions:
- delete the files and remove any final references from build targets
- keep equivalent functionality in `bir_printer.*` / `bir_validate.*`
  where needed
- rerun targeted backend validation after the deletion

Completion check:
- the legacy IR files are gone from the build graph and source tree
- no active backend code or active test includes them

### Step 6: Remove backend-to-LLVM fallback rescue paths

Goal:
- remove only the automatic backend-to-LLVM fallback path for `--codegen asm`
  when native backend lowering fails
- keep LLVM codegen itself available as an explicit codegen path; this step is
  not a full LLVM removal

Primary targets:
- [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
- [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)

Concrete actions:
- remove backend-to-LLVM rescue behavior only after native backend coverage is
  good enough
- delete fallback-only diagnostics and normalization helpers once the rescue
  path is gone
- keep explicit backend failures when backend coverage is still missing instead
  of silently switching codegen implementations

Completion check:
- `--codegen asm` no longer rescues unsupported backend slices through LLVM
- explicit LLVM-driven flows still work when LLVM is selected on purpose
- remaining failures are explicit backend errors, not silent fallback success

## Validation

Default validation ladder for each slice:

- owned object builds first
- then narrow backend binaries or tests directly touched by the slice
- then broader backend validation when the signal is useful

Preferred commands:

```bash
cmake --build build -j8
./build/backend_bir_tests
./build/backend_shared_util_tests
ctest --test-dir build -R '^backend_runtime_(call_helper|extern_global_array)$' --output-on-failure
```

Use a broader backend sweep only when it gives actionable signal:

```bash
ctest --test-dir build -R backend --output-on-failure
```

## Close Readiness

This plan is ready to close only when:

- `call_decode` and emitter helper seams are BIR-first
- `lir_to_backend_ir.*` and `ir.*` are deleted
- active tests no longer rely on legacy backend IR utilities
- LLVM rescue behavior is removed, or explicitly split into a separate active
  idea before closing idea 41
