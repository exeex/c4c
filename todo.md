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
  extend backlog item 4 from simple struct-backed globals into mutable
  pointer-field coverage, so direct pointer-field stores and pointer-field
  alias stores/readback keep honest semantic BIR addresses instead of dropping
  back to LLVM-text route escape
- candidate proving surface:
  `backend_codegen_route_riscv64_defined_global_array_defaults_to_bir`
  `backend_codegen_route_riscv64_defined_global_array_store_defaults_to_bir`
  `backend_codegen_route_riscv64_extern_global_array_defaults_to_bir`
  `backend_codegen_route_riscv64_defined_string_global_store_defaults_to_bir`
  `backend_codegen_route_riscv64_defined_pointer_global_array_defaults_to_bir`
  `backend_codegen_route_riscv64_defined_pointer_global_pointer_defaults_to_bir`
  `backend_codegen_route_riscv64_global_int_pointer_roundtrip_defaults_to_bir`
  `backend_codegen_route_riscv64_defined_global_struct_store_defaults_to_bir`
  `backend_codegen_route_riscv64_anonymous_global_struct_fields_defaults_to_bir`
  `backend_codegen_route_riscv64_named_global_struct_designated_init_defaults_to_bir`
  `backend_codegen_route_riscv64_named_pointer_global_struct_designated_init_defaults_to_bir`
  `backend_codegen_route_riscv64_named_pointer_global_struct_pointer_store_defaults_to_bir`
  `backend_codegen_route_riscv64_named_pointer_global_struct_pointer_field_alias_store_defaults_to_bir`
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
- nested integer-array globals keep explicit addressed stores on the semantic
  BIR route for direct array syntax, extern-array syntax, and pointer-derived
  aliases rather than falling back once the access is a write
- simple struct-backed globals keep direct scalar field stores and reads on the
  semantic BIR route, and designated pointer fields initialized from global
  addresses stay aliasable enough for the first dereference to remain semantic
- mutable struct pointer fields keep direct stores and local alias stores on
  the semantic BIR route, and the post-store readback keeps the updated
  addressed-global pointee instead of reverting to static initializer knowledge
- no new direct route, rendered-text matcher, or tiny case-family special path
  is introduced

## Latest Packet Progress

- completed:
  pointer-valued stores through addressed pointer-global slots now stay on the
  semantic BIR route: when a pointer load preserves a pointer-global object
  address such as `@gp`, later `store ptr` through that alias lowers as
  `bir.store_global @gp, ptr %...` instead of dropping to raw LLVM IR
  pointer-global loads now preserve both the pointer-global object alias chain
  and the current known addressed-global pointee after an in-function pointer
  store, so a later `load ptr, ptr @gp` reuses the just-stored addressed-global
  target rather than the static initializer
  new riscv64 route proofs now cover `*ggp = gq; return gp[1];` and
  `**gggp = gq; return gp[1];` through semantic BIR as explicit
  `bir.store_global @gp, ptr %...` plus the post-store addressed-global reload
  from `@arr, offset 8` rather than raw LLVM fallback
  `bir.store_global` now prints non-zero byte offsets, so BIR route proofs can
  distinguish addressed writes from base-global writes instead of hiding the
  semantic target slot in the textual proof surface
  new riscv64 route proofs now cover `gp[1] = 9; return arr[1];`,
  `(*ggp)[1] = 9; return arr[2];`, and `gp[1] = 'o'; return g_text[1];`
  through semantic BIR as offset-preserving addressed-global stores plus
  matching addressed-global reloads rather than raw LLVM fallback
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
  pointer-global object addresses now survive `ptrtoint` / `inttoptr`
  roundtrips instead of only already-resolved addressed-global data pointers;
  the strengthened `global_int_pointer_roundtrip.c` route proof now covers
  `(**(int ***)(long)&ggp)[1]` through semantic BIR as `bir.load_global ptr
  @ggp`, then `bir.load_global ptr @gp`, then `bir.load_global i32 @arr,
  offset 4` rather than falling back once the cast roundtrip preserved the
  intermediate pointer-global object address
  string-backed addressed globals now use the same extra pointer-global
  indirection lane as integer arrays; new riscv64 route proofs cover
  `char **ggp = &gp; return (*ggp)[1];` and `(*ggp)[1] = 'o'; return g_text[1];`
  through semantic BIR as `bir.load_global ptr @ggp`, then
  `bir.load_global ptr @gp`, then addressed `bir.load_global i8 @g_text,
  offset 1` / `bir.store_global @g_text, offset 1, i8 ...` rather than
  dropping to raw LLVM fallback
  pointer-global object-address roundtrip stores were already semantically
  supported; new riscv64 route proofs now cover `*(int **)(long)&gp = gq;
  return gp[0];` and `**(int ***)(long)&ggp = gq; return gp[1];` through
  semantic BIR as `bir.store_global @gp, ptr ...` followed by the later
  `bir.load_global ptr @gp` and addressed `bir.load_global i32 @arr, offset 4`
  / `offset 8` reloads rather than raw LLVM fallback
  direct addressed stores on nested integer-array globals now stay on the
  semantic BIR route too; new riscv64 route proofs cover `arr[1][0] = 9;
  return arr[1][0];`, `ext_arr[1][0] = 9; return ext_arr[1][0];`, and
  `int *p = &arr[0][0]; p[2] = 9; return arr[1][0];` as explicit
  `bir.store_global @arr/@ext_arr, offset 8, i32 9` plus matching addressed
  reloads rather than raw LLVM fallback
  simple struct-backed globals now stay on the semantic BIR route too; new
  riscv64 route proofs cover direct scalar field stores on `v.x` / `v.y`,
  anonymous and named designated-init scalar field reads at `@s` offsets 0/4/8,
  and a designated pointer field read from `struct S { int a; int *p; }`
  through semantic BIR as `bir.load_global ptr @s, offset 8` followed by
  `bir.load_global i32 @x` rather than dropping back to LLVM-text fallback
  aggregate global lowering now decodes simple struct layouts from LIR type
  declarations, preserves honest byte offsets for struct-field GEPs, and keeps
  pointer-field global initializers as named pointer slots so the first
  dereference can recover the addressed global's scalar lane instead of losing
  alias information at the aggregate boundary
  addressed pointer-field stores inside struct-backed globals now preserve
  runtime alias information too; new riscv64 route proofs cover both
  `s.p = gp; return *s.p;` and `int **pp = &s.p; *pp = gp; return **pp;`
  through semantic BIR as `bir.store_global @s, offset 8, ptr %...` followed
  by the later `bir.load_global ptr @s, offset 8` and addressed
  `bir.load_global i32 @y` instead of falling back to the static initializer
  or raw LLVM output
- remaining next:
  keep backlog item 4 on honest addressed-global coverage; broader pointer
  global forms beyond recursively resolved constant in-bounds aliases and the
  now-covered pointer-value stores into addressed pointer-global slots and
  pointer-global object-address roundtrip stores, plus addressed global
  aggregates beyond simple scalar/pointer-field structs, multi-step pointer
  field mutation/readback combinations beyond the first direct and aliased
  store lane, and any still-unknown addressed-global aggregate shapes, are
  still outside this finished slice
- proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_riscv64_defined_global_array_defaults_to_bir|backend_codegen_route_riscv64_defined_global_array_store_defaults_to_bir|backend_codegen_route_riscv64_extern_global_array_defaults_to_bir|backend_codegen_route_riscv64_defined_string_global_store_defaults_to_bir|backend_codegen_route_riscv64_defined_pointer_global_array_defaults_to_bir|backend_codegen_route_riscv64_defined_pointer_global_pointer_defaults_to_bir|backend_codegen_route_riscv64_global_int_pointer_roundtrip_defaults_to_bir|backend_codegen_route_riscv64_defined_global_struct_store_defaults_to_bir|backend_codegen_route_riscv64_anonymous_global_struct_fields_defaults_to_bir|backend_codegen_route_riscv64_named_global_struct_designated_init_defaults_to_bir|backend_codegen_route_riscv64_named_pointer_global_struct_designated_init_defaults_to_bir|backend_codegen_route_riscv64_named_pointer_global_struct_pointer_store_defaults_to_bir|backend_codegen_route_riscv64_named_pointer_global_struct_pointer_field_alias_store_defaults_to_bir)$' > test_after.log 2>&1`
- proof log:
  `test_after.log`

## Parked While This Packet Is Active

- broader call-lane expansion beyond the current minimal direct-call coverage
- intrinsic/runtime operation lowering
- real stack layout, liveness, and regalloc
- target backend ingestion rewrites beyond the shared BIR/prepare spine
