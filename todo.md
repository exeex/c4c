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
  add the first semantic direct-call BIR lane so `call_helper.c` and
  `two_arg_helper.c` lower via BIR instead of falling back to LLVM text
- current proving surface:
  `tests/c/internal/backend_case/branch_if_eq.c`
  `tests/c/internal/backend_case/call_helper.c`
  `tests/c/internal/backend_case/two_arg_helper.c`
- packet update:
  `src/backend/lowering/lir_to_bir_module.cpp` now lowers the first
  direct-call lane by mapping minimal direct global `LirCallOp` sites
  into `bir.call`, including the zero-arg helper shape used by
  `call_helper.c`; the remaining zero-arg gap was the declaration-side
  single-`void` parameter sentinel, which is now treated as a true
  zero-arg function while preserving clean `branch_if_eq` BIR
- latest proof:
  `bash -lc 'cmake --build build -j2 && printf "=== branch_if_eq ===\n" && ./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/branch_if_eq.c -o /tmp/branch_if_eq_x86.ll && cat /tmp/branch_if_eq_x86.ll && printf "\n=== call_helper ===\n" && ./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/call_helper.c -o /tmp/call_helper_x86.ll && cat /tmp/call_helper_x86.ll && printf "\n=== two_arg_helper ===\n" && ./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/two_arg_helper.c -o /tmp/two_arg_helper_x86.ll && cat /tmp/two_arg_helper_x86.ll' > test_after.log 2>&1`
- latest proof log:
  `test_after.log`
- latest proof result:
  `branch_if_eq.c`, `call_helper.c`, and `two_arg_helper.c` all now emit
  semantic BIR instead of LLVM-text fallback on the delegated x86_64 route

## Immediate Target

- widen the semantic BIR spine from scalar/local-memory flow into direct calls
- keep function calls on the shared BIR lane instead of reintroducing any
  direct route or LLVM-text fallback
- leave broader global lowering and target prepare work parked until the
  minimal direct-call contract is stable

## Done Condition For The Active Packet

- `branch_if_eq.c` still lowers to clean BIR after the direct-call work
- `call_helper.c` and `two_arg_helper.c` lower to BIR instead of falling back
  to LLVM text
- no new direct route, testcase matcher, or rendered-text probe is introduced

## Parked While This Packet Is Active

- broader memory/global/call lowering beyond minimal direct calls
- real stack layout, liveness, and regalloc
- target backend ingestion rewrites beyond the shared BIR/prepare spine
