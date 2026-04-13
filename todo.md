# Backend Reboot From BIR Spine

Status: Active
Source Idea: ideas/open/46_backend_reboot_bir_spine.md
Source Plan: plan.md

## Current Active Item

- active reset:
  the old x86-specific recovery idea is closed and archived because the repo is
  now on a backend-wide reboot path rather than an x86-only repair path
- active route:
  semantic BIR is the new truth surface, `prepare` owns target legality, and
  target backends should eventually ingest prepared BIR only
- current packet:
  extend the semantic local-memory BIR lane to cover local array addressing so
  `local_array.c` lowers via BIR instead of falling back to LLVM text
- current proving surface:
  `tests/c/internal/backend_case/branch_if_eq.c`
  `tests/c/internal/backend_case/local_array.c`
- packet update:
  `src/backend/lowering/lir_to_bir_module.cpp` now extends the
  local-memory lane to cover simple local arrays by expanding
  hoisted `[N x i32]` allocas into element slots, resolving the
  `getelementptr` chain for constant local indices back to those
  element slots, and lowering the resulting typed load/store/add
  path to semantic BIR while preserving clean `branch_if_eq` BIR
- latest proof:
  `bash -lc 'cmake --build build -j2 && printf "=== branch_if_eq ===\n" && ./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/branch_if_eq.c -o /tmp/branch_if_eq_x86.ll && cat /tmp/branch_if_eq_x86.ll && printf "\n=== local_array ===\n" && ./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/local_array.c -o /tmp/local_array_x86.ll && cat /tmp/local_array_x86.ll' > test_after.log 2>&1`
- latest proof log:
  `test_after.log`

## Immediate Target

- widen the semantic BIR spine from scalar control flow into local memory
- keep hoisted `alloca` on the shared BIR lane instead of reintroducing any
  direct route
- leave global/call lowering and target prepare work parked until the local
  slot contract is stable

## Done Condition For The Active Packet

- `branch_if_eq.c` still lowers to clean BIR after the local-array work
- `local_array.c` lowers to BIR instead of falling back to LLVM text
- no new direct route, testcase matcher, or rendered-text probe is introduced

## Parked While This Packet Is Active

- broader memory/global/call lowering
- real stack layout, liveness, and regalloc
- target backend ingestion rewrites beyond the shared BIR/prepare spine
