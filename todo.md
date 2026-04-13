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
  extend backlog item 4 from simple struct-backed globals into nested
  aggregate-address coverage, so struct-contained array fields and nested
  pointer fields keep honest semantic BIR addresses for direct reads, direct
  stores, and pointer-derived alias stores instead of dropping back to
  LLVM-text route escape; root-level aggregate arrays, including pointer-field
  elements, are now in scope too so arrays-of-structs can use the same
  addressed-global lane instead of being rejected at module-global lowering
  time
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
  `backend_codegen_route_riscv64_nested_global_struct_array_read_defaults_to_bir`
  `backend_codegen_route_riscv64_nested_global_struct_array_store_defaults_to_bir`
  `backend_codegen_route_riscv64_nested_global_struct_array_alias_store_defaults_to_bir`
  `backend_codegen_route_riscv64_global_struct_pointer_array_read_defaults_to_bir`
  `backend_codegen_route_riscv64_global_struct_pointer_array_store_defaults_to_bir`
  `backend_codegen_route_riscv64_global_struct_pointer_array_alias_store_defaults_to_bir`
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
- nested struct-backed globals keep contained integer-array field reads,
  direct stores, pointer-derived alias stores, nested pointer-field reads,
  nested pointer-field stores, and nested pointer-field alias stores on the
  semantic BIR route instead of dropping back once aggregate GEPs need more
  than one structural descent
- mutable struct pointer fields keep direct stores and local alias stores on
  the semantic BIR route, and the post-store readback keeps the updated
  addressed-global pointee instead of reverting to static initializer knowledge
- root-level aggregate arrays with pointer-valued fields keep direct reads,
  direct stores, and pointer-derived alias stores on the semantic BIR route
  instead of dropping back once the aggregate root is an array element
- aggregate pointer fields initialized from pointer-global aliases or resolved
  in-bounds pointer-global alias chains keep honest addressed-global reloads
  on the semantic BIR route for plain struct roots, nested struct roots, and
  root-level aggregate arrays instead of rejecting those initializers before
  function lowering starts
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
  nested aggregate GEPs rooted in addressed globals now stay on the semantic
  BIR route too; relative addressed-global GEP lowering no longer stops after
  one scalar-style index, so new riscv64 route proofs cover
  `return s.inner.xs[2];`, `s.inner.xs[1] = 9; return s.inner.xs[1];`, and
  `int *p = &s.inner.xs[0]; p[2] = 9; return s.inner.xs[2];` as direct
  `bir.load_global i32 @s, offset 8` plus addressed
  `bir.store_global @s, offset 4/8, i32 9` rather than raw LLVM fallback
  nested addressed-global pointer fields now have explicit route coverage too;
  new riscv64 route proofs cover `return *s.inner.p;`, `s.inner.p = gp;
  return *s.inner.p;`, and `int **pp = &s.inner.p; *pp = gp; return **pp;`
  through semantic BIR as `bir.load_global ptr @s, offset 8` /
  `bir.store_global @s, offset 8, ptr ...` followed by the later
  `bir.load_global i32 @x/@y` rather than raw LLVM fallback
  multi-step addressed-global pointer-field rewrites now have explicit route
  coverage too; new riscv64 route proofs cover `s.p = gp; *pp = gq; return
  *s.p;` and `s.inner.p = gp; *pp = gq; return *s.inner.p;` through semantic
  BIR as two ordered `bir.store_global @s, offset 8, ptr ...` updates, a later
  `bir.load_global ptr @s, offset 8`, and the final addressed-global reload
  from `@z` rather than stale initializer knowledge or raw LLVM fallback
  root-level aggregate global arrays now lower through the same aggregate lane
  as struct roots instead of being rejected before GEP lowering; new riscv64
  route proofs cover `pairs[1].y` and `pairs[1].x = 9; return pairs[1].x;`
  through semantic BIR as `bir.load_global i32 @pairs, offset 12` and
  `bir.store_global @pairs, offset 8, i32 9` plus the matching addressed
  reload rather than raw LLVM fallback
  root-level aggregate arrays with pointer-valued fields now have explicit
  route coverage too; new riscv64 route proofs cover `return *pairs[1].p;`,
  `pairs[1].p = gp; return *pairs[1].p;`, and `int **pp = &pairs[1].p;
  *pp = gp; return **pp;` through semantic BIR as `bir.load_global ptr @pairs,
  offset 24`, `bir.store_global @pairs, offset 24, ptr ...`, and the later
  addressed-global reload from `@y` rather than raw LLVM fallback
  aggregate pointer-field initializers now reuse pointer-global alias
  resolution instead of requiring direct `&global` roots only; plain struct
  roots, nested struct roots, and root-level array-of-struct roots can now
  statically initialize pointer fields from `gp` / `gpp`-style globals and
  still reload the resolved addressed-global target through semantic BIR
  rather than rejecting those initializers during module-global lowering
  new riscv64 route proofs now cover `struct S s = {.p = gp, .a = 1}; return
  s.p[1];`, `struct Outer s = {1, {gp}}; return *s.inner.p;`, and
  `struct Pair pairs[2] = {{1, &x}, {2, gpp}}; return *pairs[1].p;`
  through semantic BIR as `bir.load_global ptr @s/@pairs, offset ...`
  followed by addressed-global reloads from `@arr, offset 4/8` rather than
  raw LLVM fallback or aggregate-initializer rejection
  deeper root-level nested aggregate arrays now have explicit route coverage
  too; new riscv64 route proofs cover `groups[1].inner.xs[2]`,
  `groups[1].inner.xs[1] = 9; return groups[1].inner.xs[1];`, and
  `int *p = &groups[0].inner.xs[0]; p[5] = 9; return groups[1].inner.xs[2];`
  through semantic BIR as direct `bir.load_global i32 @groups, offset 20` and
  addressed `bir.store_global @groups, offset 16/20, i32 9` rather than raw
  LLVM fallback, confirming the existing addressed-global aggregate lowering
  scales across array-root then struct-field then nested-array descent without
  testcase-shaped routing
- remaining next:
  keep backlog item 4 on honest addressed-global coverage; broader pointer
  global forms beyond recursively resolved constant in-bounds aliases and the
  now-covered pointer-value stores into addressed pointer-global slots,
  pointer-global object-address roundtrip stores, static aggregate
  initializers that truly need pointer-global object addresses such as `&gp`,
  and deeper root aggregate arrays with pointer-valued fields are still
  outside this finished slice
- proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_codegen_route_riscv64_.*global.*defaults_to_bir$' > test_after.log 2>&1`
- proof log:
  `test_after.log`

## Parked While This Packet Is Active

- broader call-lane expansion beyond the current minimal direct-call coverage
- intrinsic/runtime operation lowering
- real stack layout, liveness, and regalloc
- target backend ingestion rewrites beyond the shared BIR/prepare spine
