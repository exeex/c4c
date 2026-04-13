# Backend Reboot From BIR Spine

Status: Active
Source Idea: ideas/open/46_backend_reboot_bir_spine.md
Source Plan: plan.md

## Current Active Item

- active route:
  semantic BIR is the new truth surface, `prepare` owns target legality, and
  target backends should eventually ingest prepared BIR only
- runbook repair:
  `plan.md` now carries the ordered remaining capability backlog so future
  executor packets can be chosen from a durable semantic queue rather than
  rediscovering the next testcase family from chat
- current capability family:
  backlog item 4, broader global data and addressed globals
- current packet shape:
  continue backlog item 4 from zero-init and round-tripped addressed globals
  into defined global data materialization, starting with simple non-zero array
  initializers and addressed reads that can still lower honestly through
  semantic BIR
- candidate proving surface:
  `backend_codegen_route_riscv64_branch_if_eq_defaults_to_bir`
  `backend_codegen_route_riscv64_extern_global_array_defaults_to_bir`
  `backend_codegen_route_riscv64_string_literal_char_defaults_to_bir`
  `backend_codegen_route_riscv64_global_char_pointer_diff_defaults_to_bir`
  `backend_codegen_route_riscv64_global_int_pointer_diff_defaults_to_bir`
  `backend_codegen_route_riscv64_global_int_pointer_roundtrip_defaults_to_bir`
  use BIR route proofs here because `src/backend/backend.cpp` still prints
  prepared BIR on successful lowering; asm runtime/object checks remain a later
  backend-ingestion milestone, not the proof surface for this packet

## Immediate Target

- keep packet selection attached to the ordered semantic backlog in `plan.md`
- continue from scalar globals into addressed global data instead of jumping to
  unrelated one-off cases
- avoid reintroducing testcase-shaped routing while broadening the global lane

## Done Condition For The Active Packet

- `branch_if_eq.c` still lowers to clean BIR
- addressed-global reads still lower through semantic BIR instead of
  LLVM-text fallback
- existing same-global pointer-difference and global pointer-roundtrip cases
  stay on the semantic BIR route surface
- simple non-zero defined global arrays and their addressed reads lower
  through semantic BIR on the riscv64 route surface
- no new direct route, rendered-text matcher, or tiny case-family special path
  is introduced

## Latest Packet Progress

- completed:
  extended the addressed-global lane beyond direct reads by carrying
  same-global `ptrtoint` aliases through lowering, folding constant pointer
  differences into honest integer distances, then taught semantic lowering to
  carry direct-global address round-trips through local integer and pointer
  slots so `global_int_pointer_roundtrip.c` now lowers straight to
  `bir.load_global` on the riscv64 route surface without adding a fallback
  route or testcase-shaped matcher
- remaining next:
  keep backlog item 4 on honest addressed-global coverage; defined array
  initializers, richer string/data materialization, and addressed global loads
  beyond constant-distance arithmetic are still outside this finished slice
- proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_riscv64_branch_if_eq_defaults_to_bir|backend_codegen_route_riscv64_extern_global_array_defaults_to_bir|backend_codegen_route_riscv64_string_literal_char_defaults_to_bir|backend_codegen_route_riscv64_global_char_pointer_diff_defaults_to_bir|backend_codegen_route_riscv64_global_int_pointer_diff_defaults_to_bir|backend_codegen_route_riscv64_global_int_pointer_roundtrip_defaults_to_bir|backend_codegen_route_riscv64_return_eq_defaults_to_bir|backend_codegen_route_riscv64_return_ult_defaults_to_bir)$'`
- proof log:
  `test_after.log`

## Parked While This Packet Is Active

- broader call-lane expansion beyond the current minimal direct-call coverage
- intrinsic/runtime operation lowering
- real stack layout, liveness, and regalloc
- target backend ingestion rewrites beyond the shared BIR/prepare spine
