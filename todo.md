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
  add the first semantic global-load BIR lane so `global_load.c` and
  `global_load_zero_init.c` lower via BIR instead of falling back to LLVM text
- current proving surface:
  `tests/c/internal/backend_case/branch_if_eq.c`
  `tests/c/internal/backend_case/global_load.c`
  `tests/c/internal/backend_case/global_load_zero_init.c`
- packet update:
  `src/backend/lowering/lir_to_bir_module.cpp` now lowers the first
  minimal scalar global lane by lowering supported `LirGlobal` entries
  into module-level BIR globals and mapping direct `load` / `store`
  operands rooted at `@global` into `bir.load_global` /
  `bir.store_global`; this covers initialized and zero-initialized
  scalar globals while preserving the clean `branch_if_eq` path
- latest proof:
  `bash -lc 'cmake --build build -j2 && printf "=== branch_if_eq ===\n" && ./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/branch_if_eq.c -o /tmp/branch_if_eq_x86.ll && cat /tmp/branch_if_eq_x86.ll && printf "\n=== global_load ===\n" && ./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/global_load.c -o /tmp/global_load_x86.ll && cat /tmp/global_load_x86.ll && printf "\n=== global_load_zero_init ===\n" && ./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/global_load_zero_init.c -o /tmp/global_load_zero_x86.ll && cat /tmp/global_load_zero_x86.ll' > test_after.log 2>&1`
- latest proof log:
  `test_after.log`
- latest proof result:
  `branch_if_eq.c`, `global_load.c`, and `global_load_zero_init.c` all
  now emit semantic BIR instead of LLVM-text fallback on the delegated
  x86_64 route
- testcase packet:
  added `tests/c/internal/backend_case/global_store.c` as the minimal
  repo-owned scalar global-write proving surface for the next packet;
  this case writes a constant into `g_counter` and returns the updated
  scalar global value without introducing pointer or aggregate behavior
- testcase packet proof:
  `bash -lc 'cmake --build build -j2 && ./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/global_store.c -o /tmp/global_store_x86.ll && cat /tmp/global_store_x86.ll' > test_after.log 2>&1`
- testcase packet proof log:
  `test_after.log`

## Immediate Target

- widen the semantic BIR spine from scalar/local-memory/call flow into globals
- keep scalar global accesses on the shared BIR lane instead of reintroducing
  any direct route or LLVM-text fallback
- leave broader aggregate/addressed global lowering and target prepare work
  parked until the minimal scalar-global contract is stable

## Done Condition For The Active Packet

- `branch_if_eq.c` still lowers to clean BIR after the global work
- `global_load.c` and `global_load_zero_init.c` lower to BIR instead of
  falling back to LLVM text
- no new direct route, testcase matcher, or rendered-text probe is introduced

## Parked While This Packet Is Active

- broader aggregate/pointer global lowering beyond minimal scalar global loads
- real stack layout, liveness, and regalloc
- target backend ingestion rewrites beyond the shared BIR/prepare spine
