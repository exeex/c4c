Status: Active
Source Idea Path: ideas/open/130_sema_hir_ast_ingress_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Sema AST Ingress

# Current Packet

## Just Finished

Completed Step 1 Sema AST ingress audit.

Classification notes:

- Structured authority: `SemaStructuredNameKey` is declared in
  `src/frontend/sema/validate.hpp:24` and built from AST structured fields in
  `sema_local_name_key`, `sema_structured_name_key`, and
  `sema_symbol_name_key` at `src/frontend/sema/validate.cpp:46`,
  `src/frontend/sema/validate.cpp:54`, and
  `src/frontend/sema/validate.cpp:72`. These use `TextId`,
  `namespace_context_id`, `is_global_qualified`, and qualifier `TextId`
  segments rather than rendered spelling.
- Structured authority mirrors: validation currently keeps legacy
  `std::string` maps as the returned lookup path, but mirrors key-based maps
  for globals, functions, overload sets, enum constants, consteval functions,
  locals, struct members, and template parameters. Concrete sites include
  `bind_global`/`mirror_function_decl`/`record_consteval_function` at
  `src/frontend/sema/validate.cpp:645`, `src/frontend/sema/validate.cpp:654`,
  and `src/frontend/sema/validate.cpp:673`; local binding and lookup at
  `src/frontend/sema/validate.cpp:800` and `src/frontend/sema/validate.cpp:853`;
  enum keys at `src/frontend/sema/validate.cpp:938` and
  `src/frontend/sema/validate.cpp:952`; record/member key mirrors at
  `src/frontend/sema/validate.cpp:1094`, `src/frontend/sema/validate.cpp:1140`,
  and `src/frontend/sema/validate.cpp:1189`.
- Sema authority: `ResolvedTypeTable` is the Sema-produced AST-node keyed
  type authority, declared in `src/frontend/sema/canonical_symbol.hpp:126`.
  It is produced by `record_param_types`, `record_local_decl_types`,
  `collect_top_level_symbol`, and `build_canonical_symbols` at
  `src/frontend/sema/canonical_symbol.cpp:216`,
  `src/frontend/sema/canonical_symbol.cpp:230`,
  `src/frontend/sema/canonical_symbol.cpp:262`, and
  `src/frontend/sema/canonical_symbol.cpp:414`.
- Boundary handoff: `analyze_program_base` passes
  `result.canonical.resolved_types` into HIR at
  `src/frontend/sema/sema.cpp:18`. HIR stores the pointer in
  `build_initial_hir`/`Lowerer::resolved_types_` at
  `src/frontend/hir/hir_build.cpp:675` and
  `src/frontend/hir/impl/lowerer.hpp:124`, then consumes it for callable
  declaration type recovery in `Lowerer::fn_ptr_sig_from_decl_node` at
  `src/frontend/hir/hir_types.cpp:162`.
- Compatibility fallback: legacy string maps and lookups are intentionally
  still authoritative for return values in this pass, with structured maps used
  for parity comparison. Examples are `lookup_function_by_name` at
  `src/frontend/sema/validate.cpp:706`, `lookup_symbol` at
  `src/frontend/sema/validate.cpp:853`, and static-member/field legacy helpers
  at `src/frontend/sema/validate.cpp:1158` and
  `src/frontend/sema/validate.cpp:1205`.
- Display/generated text: canonical formatting, debug summaries, mangling, and
  type-symbol linker fallback are display or generated-output text rather than
  AST ingress authority. Relevant sites include
  `format_canonical_type`/`format_canonical_result` at
  `src/frontend/sema/canonical_symbol.cpp:425` and
  `src/frontend/sema/canonical_symbol.cpp:484`, plus mangling fallbacks around
  `src/frontend/sema/canonical_symbol.cpp:866`,
  `src/frontend/sema/canonical_symbol.cpp:932`,
  `src/frontend/sema/canonical_symbol.cpp:952`, and
  `src/frontend/sema/canonical_symbol.cpp:1031`.
- Suspicious Sema-only follow-up target: method/record owner and static member
  paths still resolve through rendered tag/member strings before consulting
  structured mirrors. Focused target:
  `resolve_owner_in_namespace_context`/`enclosing_method_owner_struct` and
  `lookup_struct_static_member_type` at `src/frontend/sema/validate.cpp:1231`,
  `src/frontend/sema/validate.cpp:1258`, and
  `src/frontend/sema/validate.cpp:1175`.
- Suspicious compatibility fallback to preserve for later review: unqualified
  variable lookup can fall back from parser-canonicalized `n->name` to
  `n->unqualified_name` for local-scope recovery at
  `src/frontend/sema/validate.cpp:2074`. This is documented in-code as a
  parser normalization compatibility path, not a structured authority path.

## Suggested Next

Proceed to Step 2: inspect HIR AST ingress for `NamespaceQualifier`, `TextId`
construction, `ResolvedTypeTable` override points, and HIR-side string
authority/fallback paths.

## Watchouts

Do not treat the Sema string maps as newly approved long-term authority; this
packet classifies them as compatibility fallback because the current behavior
still returns through legacy lookups. Keep the likely follow-up narrow around
rendered method-owner/member resolution instead of broad `std::string` removal.

## Proof

Not run per delegated proof: read-only audit and `todo.md` update only; no
build proof or `test_after.log` required for this packet.
