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
  add the first semantic local-memory BIR lane so simple hoisted-alloca
  functions lower via `local_slots/load_local/store_local` instead of falling
  back to LLVM text
- current proving surface:
  `tests/c/internal/backend_case/branch_if_eq.c`
  `tests/c/internal/backend_case/local_temp.c`
- packet update:
  `src/backend/lowering/lir_to_bir_module.cpp` now lowers the
  first semantic local-memory lane by recognizing hoisted
  `LirFunction.alloca_insts`, materializing BIR local slots from
  `alloca`, and lowering typed `store/load` to `bir.store_local` /
  `bir.load_local` while preserving clean `branch_if_eq` BIR
- latest proof:
  `bash -lc 'cmake --build build -j2 && printf "=== branch_if_eq ===\n" && ./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/branch_if_eq.c -o /tmp/branch_if_eq_x86.ll && cat /tmp/branch_if_eq_x86.ll && printf "\n=== local_temp ===\n" && ./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/local_temp.c -o /tmp/local_temp_x86.ll && cat /tmp/local_temp_x86.ll' > test_after.log 2>&1`
- latest proof log:
  `test_after.log`

## Immediate Target

- widen the semantic BIR spine from scalar control flow into local memory
- keep hoisted `alloca` on the shared BIR lane instead of reintroducing any
  direct route
- leave global/call lowering and target prepare work parked until the local
  slot contract is stable

## Done Condition For The Active Packet

- `branch_if_eq.c` still lowers to clean BIR after the local-memory work
- `local_temp.c` lowers to BIR instead of falling back to LLVM text
- no new direct route, testcase matcher, or rendered-text probe is introduced

## Parked While This Packet Is Active

- broader memory/global/call lowering
- real stack layout, liveness, and regalloc
- target backend ingestion rewrites beyond the shared BIR/prepare spine
