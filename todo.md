# Current Packet

Status: Active
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Migrate HIR Late Instantiation Consumers

## Just Finished

Completed the Step 5 HIR late-instantiation audit slice.

Classification:

- Semantic identity:
  `src/frontend/hir/hir_ir.hpp::SpecializationKey` is still a serialized
  `std::string canonical` with equality, ordering, and hashing over that
  string. `make_specialization_key` serializes `TypeBindings` through
  `canonical_type_str`; HIR function/struct/global template instantiation,
  deferred-instance recording, seed dedup, registry lookup, and record-owner
  template identity all consume this key.
- Semantic identity:
  `src/frontend/hir/compile_time_engine.hpp::make_pending_template_type_key`
  deduplicates deferred template-type work with a rendered-ish string assembled
  from `encode_pending_type_ref`, `type_suffix_for_mangling`, binding names,
  spans, and context text. It prefers structured `TypeSpec` fields when present
  but falls back to `TemplateArgRef::debug_text` for unresolved value args and
  non-structured type args.
- Semantic identity:
  late materialization and type-resolution paths still use
  `TemplateArgRef::debug_text` to bind unresolved template params, type packs,
  and NTTP values. Main routes are
  `materialization.cpp::type_binding_name_candidates`,
  `find_bound_type_for_param_ref`, `find_bound_type_pack_for_param_ref`,
  `resolve_explicit_string_arg`, `resolve_explicit_typed_arg`, and
  `type_resolution.cpp::apply_bindings`.
- Compatibility fallback:
  `debug_text` lookup is often a fallback after structured carriers such as
  template-param index/text id, owner metadata, concrete `TypeSpec`, or
  `array_size_expr` evaluation are tried. These fallbacks are still reachable
  and semantic, but the migration should preserve behavior while moving the
  lookup authority to structured param/value carriers.
- Compatibility fallback:
  rendered template-origin lookup remains in
  `record_pending_template_type` and `realize_template_struct` when owner
  metadata is incomplete. These paths are explicitly guarded by structured
  owner-key checks and should not be the first Step 5 packet.
- Display/dump:
  `encode_template_arg_ref_hir`, `encode_template_arg_ref_list_hir`,
  `encode_template_arg_debug_compat_hir`, and
  `encode_template_type_arg_ref_hir` are display/debug encoders when their
  output is assigned back into `TemplateArgRef::debug_text` or used for
  diagnostics. They become semantic only when their output feeds
  `SpecializationKey`, pending-type dedup, or later string parsing.
- Display/dump:
  `Function::spec_key`, `HirTemplateInstantiation::spec_key`,
  `HirRecordOwnerTemplateIdentity::specialization_key`, and
  `structured_key->spec_key.canonical` have dump/display uses, but the same
  fields also participate in semantic lookup and must be split or renamed
  during migration.
- Dead/unreachable:
  no obviously dead HIR `debug_text`, `canonical_type_str`, or
  `make_specialization_key` consumer was identified in the audited surface.

## Suggested Next

Next coherent packet: migrate HIR `SpecializationKey` first. Introduce a
structured HIR specialization-key payload in `hir_ir.hpp` that stores template
owner/name plus ordered type/value argument identity, keeps the existing
`canonical` string only as a display/compatibility mirror, and updates equality,
ordering, hashing, `make_specialization_key`, and the direct HIR call sites in
`compile_time_engine.hpp`, `hir_types.cpp`, `hir_functions.cpp`,
`impl/templates/global.cpp`, and `impl/templates/materialization.cpp`.

## Watchouts

- Do not start Step 5 by changing `encode_template_arg_ref_hir` alone; that
  would only reformat the string key and risks overfit. The first non-overfit
  packet should make `SpecializationKey` compare/hash structured arguments
  while retaining `canonical` for diagnostics or compatibility.
- `canonical_type_str` has one non-instantiation semantic use in
  `value_args.cpp` for `is_same` trait evaluation. It is type-equality
  semantics but not template-instantiation identity; keep it out of the first
  packet unless the new structured comparator can be reused directly.
- Materialization/type-resolution `debug_text` consumers are a second packet:
  they need param-index/text-id and deferred-NTTP carrier threading, not just
  string renaming.
- Pending template-type dedup is a third packet candidate after
  `SpecializationKey`: replace `make_pending_template_type_key` with a
  structured key that recursively keys `TemplateArgRef` by kind plus structured
  `TypeSpec` or value/deferred-expression payload.

## Proof

Audit-only scratchpad update. No implementation files changed, no build/test
command was required, and `test_after.log` was intentionally not touched.
Recommended proving subset for the first implementation packet: build/compile
proof plus focused HIR template tests covering function-template dedup,
template-struct instantiation lookup, deferred member typedef/type realization,
and NTTP template args; include at least one formatting-sensitive pair that
would collide or split under rendered `canonical_type_str`.
