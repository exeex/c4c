Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Backend Layout Authority

# Current Packet

## Just Finished

Step 1 - Inventory Backend Layout Authority completed the report-only
inventory of backend aggregate layout consumers that still depend on
`module.type_decls` through the BIR `TypeDeclMap`.

Backend-owned BIR inventory:

- Lookup-only: `src/backend/bir/lir_to_bir/module.cpp` builds
  `TypeDeclMap` with `build_type_decl_map(context.lir_module.type_decls)`;
  `src/backend/bir/lir_to_bir/types.cpp` parses rendered declarations in
  `build_type_decl_map` and resolves named `%struct` bodies in
  `resolve_type_decl_body`.
- Size/alignment: `compute_aggregate_type_layout` in
  `src/backend/bir/lir_to_bir/types.cpp` is the shared size/alignment source;
  direct BIR consumers include `aggregate.cpp` local aggregate slot creation
  and copies, `globals.cpp` aggregate global storage sizing,
  `memory/local_slots.cpp` local aggregate slots and scalar-array views,
  `memory/addressing.cpp` byte-offset projections and repeated extents, and
  `memory/provenance.cpp` scalar subobject facts.
- ABI classification: `aggregate.cpp::lower_byval_aggregate_layout`,
  `call_abi.cpp::lower_return_info_from_type`,
  `call_abi.cpp::lower_function_params`, `cfg.cpp` aggregate phi planning,
  `calling.cpp` aggregate call/byval/variadic paths, and
  `memory/intrinsics.cpp` memcpy/memset aggregate helpers use the legacy layout
  facts for byval size/alignment, sret, aggregate returns, and variadic
  aggregate handling. BIR currently does not have the MIR-only HFA/direct-GP
  parser surface; HFA/direct-GP references found are MIR residue.
- Initializer lowering: `global_initializers.cpp` uses
  `compute_aggregate_type_layout`, `parse_global_address_initializer`, and
  recursive aggregate initializer lowering; `globals.cpp::lower_minimal_global`
  uses the same helpers for aggregate global byte storage and pointer
  initializer offsets.
- Memory addressing: `memory/addressing.cpp`, `memory/local_gep.cpp`,
  `memory/provenance.cpp`, and `memory/intrinsics.cpp` use layout facts for
  byte-offset projection, child-index projection, repeated aggregate extents,
  byte-storage reinterpretation, scalar leaf facts, dynamic aggregate arrays,
  and provenance-preserving pointer addressing.
- Load/store: `memory/local_slots.cpp` lowers aggregate stores, aggregate
  loads, tracked pointer slot loads/stores, dynamic local aggregate
  load/store, scalar array collection, and local aggregate aliases using the
  legacy layout facts; `aggregate.cpp` provides sorted leaf slots and
  aggregate copy helpers used by those paths.

BIR/backend-owned paths are under `src/backend/bir/lir_to_bir/` and receive
layout through the single `TypeDeclMap` built in `module.cpp`. MIR-only
compile-target residue is isolated to `src/backend/mir/aarch64/codegen/emit.cpp`,
including `gen_resolve_type_decl_body`, `gen_struct_layout`,
`gen_type_layout`, `gen_try_parse_hfa_type`,
`gen_is_direct_gp_aggregate_type`, `gen_type_is_sret_aggregate_type`, and
`gen_emit_global_init`; per idea 113, those are exclusion candidates, not
migration targets for this route.

## Suggested Next

First concrete conversion packet: introduce the structured backend layout table
at the BIR lookup authority boundary, adjacent to
`build_type_decl_map(context.lir_module.type_decls)` and
`compute_aggregate_type_layout`. Convert the lookup-only surface first by
building structured layout facts from `context.lir_module.struct_decls`, then
compare each structured entry against the existing legacy `TypeDeclMap` result.
The fallback/parity surface is the current legacy path:
`build_type_decl_map` plus `compute_aggregate_type_layout`; consumers should
continue receiving legacy-equivalent `AggregateTypeLayout` facts until parity
checks are wired for the touched path.

## Watchouts

Do not migrate `src/backend/mir/`; if MIR `.cpp` files are the only compile
blocker, treat them as compile-target exclusion candidates.

Do not remove `module.type_decls`; structured layout must keep legacy fallback
and parity observation.

Do not start with ABI or memory lowering. Converting the lookup-only authority
first gives every later consumer one fallback/parity surface instead of adding
one-off checks in call ABI, globals, local slots, addressing, or intrinsics.

## Proof

Report-only packet; no build required and no `test_after.log` produced.
Sanity proof commands:

- `git diff --name-only`
- `git diff --check`
