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
  continue backlog item 4 from defined string-backed byte globals into
  pointer-valued globals whose initializers are honest addresses of already
  addressable globals, so addressed reads can stay in semantic BIR without
  reintroducing direct-route recovery
- candidate proving surface:
  `backend_codegen_route_riscv64_branch_if_eq_defaults_to_bir`
  `backend_codegen_route_riscv64_extern_global_array_defaults_to_bir`
  `backend_codegen_route_riscv64_defined_global_array_defaults_to_bir`
  `backend_codegen_route_riscv64_defined_global_array_pointer_defaults_to_bir`
  `backend_codegen_route_riscv64_defined_pointer_global_array_defaults_to_bir`
  `backend_codegen_route_riscv64_defined_string_global_char_defaults_to_bir`
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
- defined string-backed byte globals also lower through semantic BIR and allow
  addressed byte reads without dropping to the separate string-pool-only path
- defined pointer-valued globals initialized from already addressable globals
  lower through semantic BIR and allow addressed reads from the referenced
  global data without falling back to raw LLVM IR
- no new direct route, rendered-text matcher, or tiny case-family special path
  is introduced

## Latest Packet Progress

- completed:
  extended semantic BIR globals beyond raw scalar and array data by carrying a
  direct global-symbol initializer for `ptr` globals, then proving with
  `defined_pointer_global_array.c` that `int *gp = arr; return gp[1];` lowers
  through semantic BIR as a direct `bir.load_global ptr @gp` followed by
  addressed `bir.load_global i32 @arr, offset 4` on the riscv64 route surface
  without a fallback route or testcase-shaped matcher
- remaining next:
  keep backlog item 4 on honest addressed-global coverage; broader pointer
  global forms beyond direct named-global initializers and richer addressed
  global data shapes are still outside this finished slice
- proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_riscv64_branch_if_eq_defaults_to_bir|backend_codegen_route_riscv64_extern_global_array_defaults_to_bir|backend_codegen_route_riscv64_defined_global_array_defaults_to_bir|backend_codegen_route_riscv64_defined_global_array_pointer_defaults_to_bir|backend_codegen_route_riscv64_defined_string_global_char_defaults_to_bir|backend_codegen_route_riscv64_string_literal_char_defaults_to_bir|backend_codegen_route_riscv64_global_char_pointer_diff_defaults_to_bir|backend_codegen_route_riscv64_global_int_pointer_diff_defaults_to_bir|backend_codegen_route_riscv64_global_int_pointer_roundtrip_defaults_to_bir|backend_codegen_route_riscv64_defined_pointer_global_array_defaults_to_bir|backend_codegen_route_riscv64_return_eq_defaults_to_bir|backend_codegen_route_riscv64_return_ult_defaults_to_bir)$'`
- proof log:
  `test_after.log`

## Parked While This Packet Is Active

- broader call-lane expansion beyond the current minimal direct-call coverage
- intrinsic/runtime operation lowering
- real stack layout, liveness, and regalloc
- target backend ingestion rewrites beyond the shared BIR/prepare spine
