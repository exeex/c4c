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
  `backend_codegen_route_riscv64_defined_pointer_global_array_offset_defaults_to_bir`
  `backend_codegen_route_riscv64_defined_pointer_global_pointer_defaults_to_bir`
  `backend_codegen_route_riscv64_defined_pointer_global_pointer_offset_defaults_to_bir`
  `backend_codegen_route_riscv64_defined_pointer_global_pointer_pointer_defaults_to_bir`
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
- pointer-valued globals initialized from other pointer-valued globals keep the
  resolved base-global address plus constant in-bounds offset instead of
  dropping to raw LLVM fallback
- pointer-valued globals whose initializers take the address of another
  pointer-valued global keep that intermediate global object address explicit,
  then still recover the pointee's resolved addressed-global data after the
  extra load instead of falling back to raw LLVM IR
- pointer-valued globals initialized from pointer-global constant-offset
  expressions keep the resolved pointee stride instead of preserving the
  frontend's byte-gep artifact
- no new direct route, rendered-text matcher, or tiny case-family special path
  is introduced

## Latest Packet Progress

- completed:
  preserved pointer-global object addresses as addressable globals instead of
  collapsing them into their pointee address too early; a new riscv64 route
  proof now covers `int **ggp = &gp; return (*ggp)[1];` through semantic BIR as
  `bir.load_global ptr @ggp`, then `bir.load_global ptr @gp`, then
  `bir.load_global i32 @arr, offset 8` rather than raw LLVM fallback
  corrected recursive pointer-global address resolution so constant-offset
  aliases scale through the resolved pointee stride instead of preserving the
  frontend's `getelementptr inbounds (i8, ptr @gp, i64 N)` byte-gep artifact;
  `defined_pointer_global_pointer_offset.c` now proves
  `int *gp = &arr[1]; int *gpp = gp + 1; return gpp[1];` lowers through BIR as
  `bir.load_global ptr @gpp` followed by `bir.load_global i32 @arr, offset 12`
  on the riscv64 route surface rather than the incorrect byte-offset result
- remaining next:
  keep backlog item 4 on honest addressed-global coverage; broader pointer
  global forms beyond recursively resolved constant in-bounds aliases and
  richer addressed global data shapes are still outside this finished slice
- proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_riscv64_branch_if_eq_defaults_to_bir|backend_codegen_route_riscv64_extern_global_array_defaults_to_bir|backend_codegen_route_riscv64_defined_global_array_defaults_to_bir|backend_codegen_route_riscv64_defined_global_array_pointer_defaults_to_bir|backend_codegen_route_riscv64_defined_pointer_global_array_defaults_to_bir|backend_codegen_route_riscv64_defined_pointer_global_array_offset_defaults_to_bir|backend_codegen_route_riscv64_defined_pointer_global_pointer_defaults_to_bir|backend_codegen_route_riscv64_defined_pointer_global_pointer_offset_defaults_to_bir|backend_codegen_route_riscv64_defined_pointer_global_pointer_pointer_defaults_to_bir|backend_codegen_route_riscv64_defined_string_global_char_defaults_to_bir|backend_codegen_route_riscv64_string_literal_char_defaults_to_bir|backend_codegen_route_riscv64_global_char_pointer_diff_defaults_to_bir|backend_codegen_route_riscv64_global_int_pointer_diff_defaults_to_bir|backend_codegen_route_riscv64_global_int_pointer_roundtrip_defaults_to_bir|backend_codegen_route_riscv64_return_eq_defaults_to_bir|backend_codegen_route_riscv64_return_ult_defaults_to_bir)$'`
- proof log:
  `test_after.log`

## Parked While This Packet Is Active

- broader call-lane expansion beyond the current minimal direct-call coverage
- intrinsic/runtime operation lowering
- real stack layout, liveness, and regalloc
- target backend ingestion rewrites beyond the shared BIR/prepare spine
