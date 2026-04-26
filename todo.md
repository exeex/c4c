Status: Active
Source Idea Path: ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory BIR Aggregate Layout Authority

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory of BIR aggregate layout authority.

Authority model:
- `src/backend/bir/lir_to_bir/types.cpp` builds `BackendStructuredLayoutTable`
  from structured `LirStructDecl`s plus legacy `type_decls`.
- `lookup_backend_aggregate_type_layout(...)` is the shared structured-aware
  lookup. For named `%struct` types, it returns the structured layout only when
  `legacy_found`, `structured_found`, `parity_checked`, and `parity_matches`
  are all true. Otherwise it falls back to `compute_aggregate_type_layout(...)`
  over legacy `type_decls`.
- Inline aggregate text and array element recursion can be structured-ready only
  through callers that pass `structured_layouts`; named structs remain
  parity-gated by design.

Path inventory:
- Globals: parity-gated for aggregate globals through
  `lower_minimal_global(..., structured_layouts)`, `lookup_global_layout(...)`,
  aggregate size/alignment, and `lower_aggregate_initializer(...)`. Global
  pointer/GEP initializers also pass structured layouts through
  `parse_global_address_initializer(...)`. Fallback-only overloads still exist
  for legacy-only callers.
- Locals: parity-gated for aggregate `alloca`, sret/byval local scratch, leaf
  slot collection, aggregate copy, scalar byte-offset facts, and local array
  views through `declare_local_aggregate_slots(...)`,
  `collect_sorted_leaf_slots(...)`, and local-slot helpers that receive
  `structured_layouts_`. Dynamic alloca remains scalar/function-pointer only
  and is not an aggregate layout path.
- GEP/addressing: parity-gated for local aggregate GEP, global aggregate GEP,
  relative GEP target resolution, byte-offset projection, repeated aggregate
  extent discovery, dynamic aggregate arrays, and pointer-address provenance
  where the call path passes `structured_layouts_`. Fallback-only overloads
  remain for helper reuse without a table.
- Memory ops: parity-gated for aggregate loads/stores, local aggregate copies,
  `memcpy`, and `memset` through `lower_byval_aggregate_layout(...)`,
  `lower_intrinsic_aggregate_layout(...)`, and immediate local memop helpers
  with `structured_layouts_`. Runtime/instruction memops still require immediate
  non-volatile sizes and local/known aggregate provenance.
- Calls: parity-gated for sret returns, byval parameters, aggregate call
  returns, direct calls, and indirect calls through `lower_return_info_from_type`,
  `lower_function_params_with_layouts`, and call-argument lowering. Direct and
  indirect aggregate arguments are lowered as pointer/byval memory ABI once the
  aggregate layout is accepted.
- Variadic arguments: variadic call detection itself is parsed from typed call
  signatures and is layout-neutral; aggregate variadic call arguments use the
  same parity-gated byval aggregate argument path as other calls.
- `va_arg`: parity-gated for aggregate `va_arg` through
  `lower_byval_aggregate_layout(va_arg.type_str, type_decls_,
  &structured_layouts_)`, local aggregate slot declaration, and an
  `llvm.va_arg.aggregate` call. Scalar `va_arg` is layout-neutral.

Fallback-only surfaces to keep visible:
- `lookup_backend_aggregate_type_layout(...)` falls back to legacy layout for a
  named structured type whenever parity is missing or mismatched; no covered
  named `%struct` path is structured-first without parity today.
- Helper overloads accepting no `BackendStructuredLayoutTable` are still
  legacy-only: `lower_byval_aggregate_layout(text, type_decls)`,
  `parse_global_address_initializer(text, type_decls)`,
  `lower_aggregate_initializer(..., type_decls, ...)`,
  local scalar-offset/array-slot helpers without structured layouts, and
  repeated-extent/addressing helper overloads without structured layouts.
- `lower_function_decl(...)` and `lower_function_params_fallback(...)` call
  signature/param lowering with an empty or legacy-only `TypeDeclMap` and no
  structured table; these are fallback-only declaration/signature paths.
- Paths with no structured table cannot cover named structs after legacy
  `type_decls` removal, even if equivalent structured `LirStructDecl`s exist.

## Suggested Next

Start `plan.md` Step 2 by making declaration/signature aggregate ABI lowering
structured-aware instead of fallback-only. First target:
`src/backend/bir/lir_to_bir/calling.cpp` / `call_abi.cpp` paths that currently
call `lower_return_info_from_type(..., TypeDeclMap{}, ..., nullptr)` or
`lower_function_params_fallback(..., TypeDeclMap{}, ...)`.

## Watchouts

- Keep legacy `type_decls` available as fallback and parity evidence.
- Do not remove legacy parsing helpers during this runbook.
- Treat `--dump-bir` tests as guards for lowered BIR facts, not as a BIR
  printer render-context migration.
- Do not downgrade expectations or add named-case shortcuts.
- The first implementation slice should preserve parity-gated behavior for
  named `%struct` layouts; changing `lookup_backend_aggregate_type_layout` to
  structured-first globally would affect globals, locals, GEP/addressing,
  memops, calls, variadic aggregate args, and `va_arg` at once.

## Proof

No build required for this inventory-only packet. Proof command:
`git diff --check -- todo.md`.
