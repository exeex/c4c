# Current Packet

Status: Active
Source Idea Path: ideas/open/172_type_identity_authority_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Cross-Domain Risk Map

## Just Finished

Step 4 - Cross-Domain Risk Map completed as a synthesis-only audit from the
completed Step 1 sema, Step 2 HIR, and Step 3 LIR/BIR/backend findings. No
implementation behavior, tests, `plan.md`, or source idea files were changed.

Cross-domain authority map:

- Syntax payload: `TypeSpec` remains the common syntax-and-metadata carrier
  from parser/sema through HIR and into older LIR surfaces. Safe syntax payload
  includes declarator shape, qualifiers, template-origin payload, unresolved
  array expressions, and final LLVM spelling fields. Risk begins when those
  fields feed equality, binding, record lookup, layout, or ABI decisions.
  Collision priority is high for function pointers, arrays, records, and
  template-origin fields because Step 1 found partial raw `TypeSpec`
  equivalence and Step 2 found HIR ordinary type refs still `TypeSpec`-first.
  Nearby evidence includes `cpp_hir_sema_canonical_symbol_structured_metadata`,
  `cpp_hir_sema_consteval_type_utils_structured_metadata`,
  `cpp_hir_template_function_deduction_binding`, and
  `frontend_lir_function_signature_type_ref`.
- Resolved type identity: sema owns the strongest resolved identity with
  `CanonicalType`, `CanonicalTypeIdentity`, `ResolvedTypeTable`,
  `types_equal`, owner-aware template parameter ids, and structured
  declaration/name ids. HIR preserves some of that identity for callable
  function-pointer signatures and owner-aware template bindings, but ordinary
  `QualType`, field, local, global, and parameter paths still mostly carry
  `TypeSpec`. LIR adds `LirTypeRef` and optional `StructNameId`, but
  `LirTypeRef::operator==` still compares rendered text. Priority is high for
  any migration that expects ids to guard dedup/equality before these
  comparison sites stop treating equal spelling as equal identity. Existing
  probes include the frontend structured-metadata tests, HIR template binding
  tests, and `frontend_lir_global_type_ref`, `frontend_lir_extern_decl_type_ref`,
  and `frontend_lir_call_type_ref`.
- Layout identity: sema/HIR can identify records through `record_def`,
  `HirRecordOwnerKey`, owner indexes, and computed `HirStructDef` layout, but
  layout still crosses multiple rendered bridges: HIR `base_tags`,
  `Module::struct_defs` rendered tags, LIR `type_decls`, BIR
  `BackendStructuredLayoutTable` keyed by final `%struct...` spelling, and
  recursive `AggregateTypeLayout` field text. This is the highest collision
  and stale-compatibility risk because one stale spelling can change size,
  align, GEP, aggregate copies, globals, and downstream stack layout. Nearby
  route outputs/tests: `cpp_hir_record_packed_aligned_layout`,
  `cpp_hir_record_field_array_layout`,
  `cpp_hir_builtin_layout_query_sizeof_type`,
  `backend_lir_to_bir_notes`, `backend_prepare_structured_context`,
  `local_aggregate_member_offsets.c`, `packed_local_member_offsets.c`,
  `nested_param_member_array.c`, and `global_nested_struct_array_store.c`.
- ABI class: sema mangling and display names are output, but ABI class becomes
  semantic in LIR/BIR/backend. BIR lowers scalars to `bir::TypeKind` and
  records `CallArgAbiInfo`, `CallResultAbiInfo`, byval/sret flags, size, and
  alignment, but aggregate classification still depends on spelling-keyed
  layout before structured metadata fully stabilizes. The AArch64 direct LIR
  route is the largest remaining spelling-authority island, parsing
  `type_decls`, signature text, return/arg type strings, and instruction type
  strings for HFA, direct-GP, sret, memory-value, stack, and global decisions.
  Priority is high because backend observability is strong and regressions
  become concrete ABI/route changes. Nearby tests/routes include
  `backend_aarch64_signature_metadata`, `backend_lir_to_bir_notes`,
  `aggregate_param_return_pair.c`, `aggregate_return_pair.c`,
  `aggregate_param_return_pair_fn_param.c`,
  `indirect_aggregate_param_return_pair.c`,
  `byval_helper_payload_8_to_13.c`, `byval_helper_payload_9_to_14.c`, and
  `byval_helper_mixed_hfa_payload.c`.
- Display spelling: sema canonical formatting, HIR dump keys, mangled template
  names, LIR printer strings, BIR printer strings, LLVM type spellings, SSA
  names, block labels, slot names, inline asm text, call target text, and
  route-local temporary names are acceptable display/final-output surfaces when
  paired with structured ids or metadata. Priority is low unless a display
  string is reused for equality, lookup, layout, ABI class, storage size, or
  route selection. Evidence tests should keep their output expectations strong
  rather than being weakened: HIR dump regex tests, frontend LIR type-ref
  tests, backend route-output cases, and BIR/prepared printer tests are useful
  observability points.
- Diagnostics: `format_canonical_type`, `format_canonical_result`,
  pending-template display keys, template argument debug text, layout mismatch
  notes, LIR verify messages, and BIR validation errors are diagnostic
  surfaces. They should not become new identity authorities. Priority is
  medium only where diagnostics recover from missing metadata by rendering a
  fallback and that same helper is later reused by lookup or compatibility
  code. Nearby evidence includes HIR pending-template tests such as
  `cpp_hir_template_deferred_nttp_expr`,
  `cpp_hir_template_deferred_nttp_static_member_expr`, and LIR structured
  layout verification output.
- Compatibility bridges: rendered fallbacks are still necessary at old
  boundaries: consteval rendered maps, legacy type binding bridges,
  parser/HIR record-tag maps, HIR rendered owner recovery, `base_tags`,
  `struct_defs`, LIR `type_decls`, missing-`LinkNameId` extern maps,
  `signature_text` parsing, BIR legacy `TypeDeclMap`, and AArch64 direct
  route type-string parsing. Priority is highest where compatibility behavior
  is stale but still accepts generated-LIR paths, not just hand-built legacy
  fixtures. Follow-up work should fail closed for metadata-rich misses and
  shrink bridges behind structured identity instead of adding new spelling
  parser branches.

Prioritized gaps:

1. Aggregate layout identity across HIR -> LIR -> BIR/backend. Highest risk:
   spelling collisions or stale `type_decls` can affect layout, ABI class,
   stack layout, globals, and copies after structured record identity already
   exists upstream.
2. Aggregate ABI classification at the BIR and AArch64 boundaries. High risk:
   classification depends on spelling-derived layout before byval/sret/HFA and
   size/align metadata become durable structured facts.
3. `TypeSpec`-first HIR ordinary type refs and raw `TypeSpec` equivalence.
   High collision risk for function pointers, arrays, typedefs, template
   records, and no-metadata record carriers.
4. `LirTypeRef` structured payload with text-only equality. Medium-high risk:
   ids cannot reliably defend dedup or mirror checks while equal spelling masks
   missing or mismatched `StructNameId`.
5. Template record owner identity carrying serialized
   `SpecializationKey::canonical`. Medium-high stale-compatibility risk:
   structured specialization identity exists, but record/layout owner keys
   still embed a rendered component.
6. Name-identity compatibility bridges that gate type migration. Medium risk:
   missing `LinkNameId`, rendered extern dedup, raw initializer symbol
   fallbacks, and rendered member/base maps block clean type-identity
   classification when ids are absent. Present ids generally fail closed; the
   blocker is no-id compatibility, not the id-backed path.
7. Display and diagnostics reuse. Lower direct risk, but helpers must stay out
   of semantic authority paths during migration.

Name-identity migration blockers flagged:

- Generated LIR producers must populate `LinkNameId`, `StructNameId`,
  structured signature refs, and structured layout declarations consistently
  before backend bridges can reject raw spelling safely.
- HIR record/layout migration depends on complete `HirRecordOwnerKey` and
  member/base owner metadata; missing owner data still forces rendered
  `struct_defs`, `base_tags`, and tag/member lookup bridges.
- Template record identity cannot be treated as fully structured while
  `HirRecordOwnerTemplateIdentity` stores the rendered
  `SpecializationKey::canonical` string as part of the owner key.
- Function/global name ids are mostly stable, but no-id extern declarations
  and compatibility imports still leave raw names as semantic bridges.

## Suggested Next

Proceed to Step 5 by writing bounded follow-up implementation ideas for the
highest-risk gaps, starting with aggregate layout identity through the
HIR/LIR/BIR boundary and aggregate ABI classification. Each idea should name
its structured authority source, the compatibility bridge it shrinks, focused
proof routes, and reviewer reject signals for testcase-overfit fixes.

## Watchouts

- Do not weaken existing output or route expectations to match current
  spelling-authority behavior. Existing tests should remain evidence and proof
  targets, not acceptance downgrades.
- Treat AArch64 direct LIR emission separately from BIR/prealloc: both see
  aggregate layout/ABI risk, but their authority sources and proofs differ.
- Avoid follow-up ideas that merely add another rendered-name parser branch.
  The durable direction is structured identity first with compatibility bridges
  isolated and fail-closed for metadata-rich misses.
- Name-id migration gaps are blockers only where missing ids force raw spelling
  to decide type/layout/ABI behavior; id-backed display spelling is not itself
  a type-identity bug.

## Proof

- No tests executed; the delegated packet explicitly required a synthesis-only
  `todo.md` risk map and no test execution.
- Evidence used: prior completed `todo.md` packet summaries from commits
  `f69a00f0f` (Step 1 sema), `dfa82789e` (Step 2 HIR), and the active Step 3
  LIR/BIR/backend audit summary in `todo.md`.
- Targeted route/test discovery used `tests/frontend/CMakeLists.txt`,
  `tests/cpp/internal/hir_case/CMakeLists.txt`, `tests/backend/CMakeLists.txt`,
  and backend case filenames to cite nearby existing tests and route outputs.
