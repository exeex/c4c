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
  make scalar semantic BIR credible for params, compare, branch, select, and
  return without introducing any new direct-route workaround
- current proving surface:
  `tests/c/internal/backend_case/branch_if_eq.c`
  `tests/c/internal/backend_route_case/single_param_select_eq.c`
- packet update:
  `src/backend/lowering/lir_to_bir_module.cpp` now lowers the
  `single_param_select_eq` diamond-plus-phi shape to semantic `bir.select`,
  preserves clean `branch_if_eq` BIR, and reconstructs minimal integer params
  from `signature_text` when `LirFunction.params` is still empty
- latest proof:
  `bash -lc 'cmake --build build -j2 && printf "=== branch_if_eq ===\n" && ./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/branch_if_eq.c -o /tmp/branch_if_eq_x86.ll && cat /tmp/branch_if_eq_x86.ll && printf "\n=== single_param_select_eq ===\n" && ./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_route_case/single_param_select_eq.c -o /tmp/single_param_select_eq_x86.ll && cat /tmp/single_param_select_eq_x86.ll' > test_after.log 2>&1`
- latest proof log:
  `test_after.log`

## Immediate Target

- finish param-aware scalar lowering in `src/backend/lowering/lir_to_bir_module.cpp`
- lower `diamond + phi` ternary shape to semantic `bir.select`
- keep semantic `i1` in BIR and let `prepare/legalize.cpp` own widening to
  `i32` for x86/i686/aarch64/riscv64

## Done Condition For The Active Packet

- `branch_if_eq.c` still lowers to clean BIR after the param/select work
- `single_param_select_eq.c` lowers to BIR instead of falling back to LLVM text
- no new direct route, testcase matcher, or rendered-text probe is introduced

## Parked While This Packet Is Active

- broader memory/global/call lowering
- real stack layout, liveness, and regalloc
- target backend ingestion rewrites beyond the shared BIR/prepare spine
