Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Use Structured Keys for Pending and Specialization Identity

# Current Packet

## Just Finished

Completed the next bounded Step 4 packet of `plan.md`: specialization identity
now has a structured construction path that emits
`SpecializationArgumentIdentity::parameter_key` when owner-aware type/NTTP
binding mirrors completely and count-compatibly cover the legacy rendered maps.
The direct template-call resolution path in `expr/call.cpp` now returns the
structured type/NTTP maps it already builds, prefers the structured
`SpecializationKey` for instance lookup, and keeps a legacy comparison fallback
for registry seeds still produced by the legacy `record_seed` path.

Concrete changes:
- Added `try_make_structured_specialization_key`, which preserves
  `SpecializationKey::canonical` as the legacy display/compatibility mirror
  while using structured parameter keys for equality/hash identity when all
  mirrored bindings are complete and value-compatible.
- Added an optional structured type-binding return path to
  `merge_explicit_and_deduced_type_bindings` and
  `merge_explicit_and_ctx_deduced_type_bindings`, including default-filled
  bindings.
- Routed direct template-call instance lookup through the structured key when
  complete, falling back to the legacy key when structured mirrors are missing
  or mismatched.
- Added focused coverage for complete structured specialization-key
  construction plus missing/mismatched structured-map fallback.

## Suggested Next

Continue Step 4 by moving the seed/instance production side, especially
`InstantiationRegistry::record_seed` and nearby specialization materialization
paths, to accept structured type/NTTP binding mirrors so new registry entries
can be born with the same structured `SpecializationKey` that direct call
resolution now prefers.

## Watchouts

- Do not treat rendered names, mangled names, or raw `TextId` alone as complete
  semantic identity when owner-aware metadata is available.
- Do not weaken tests or convert capability work into expectation-only changes.
- Keep routine implementation progress in this file rather than rewriting the
  source idea.
- `mangle_template_name`, `format_pending_template_type_key_for_display`,
  `encode_pending_type_ref`, and `SpecializationKey::canonical` are
  display/compatibility outputs; do not make them replacement semantic keys.
- Existing pending-template state mutation still records the legacy
  `TypeBindings` / `NttpBindings` key outside the
  `ctx-deduction-template-call-result` complete-metadata path.
- The call-result switch intentionally requires structured type/NTTP binding
  counts to match the legacy maps, so enclosing context bindings without
  structured mirrors remain legacy-keyed instead of partially structured.
- Struct-method, non-method function, template-global initializer, and
  template-argument materialization expression `FunctionCtx` creation paths now
  have structured mirror handoff when their owner metadata is complete.
- Structured `SpecializationArgumentIdentity` entries currently keep
  `parameter_name` as display/fallback data only; equality and hashing use the
  structured key when complete metadata is present, and ordering follows the
  same identity rule.
- A structured-vs-legacy key difference is expected while legacy maps remain
  authoritative; the useful signal here is that structured identities are
  constructible, complete, and share the same pending static context.
- `make_hir_template_parameter_binding_key` rejects incomplete owner/parameter
  metadata, but callers still own deciding whether the source owner is the
  correct semantic authority for a binding domain.
- `populate_structured_template_binding_mirrors` intentionally mirrors only
  bindings that the supplied template owner can identify by complete
  owner/parameter metadata; legacy maps remain populated for fallback.
- The new structured-aware seed/resolve overload keeps realization inputs as
  the legacy `TypeBindings` / `NttpBindings`; structured keys affect only
  pending dedup/progress identity when complete and count-compatible.
- Direct template-call lookup is structured-first, but still compares against a
  legacy key when registry entries came from the current legacy-only seed path;
  remove that compatibility comparison only after the seed path is structured.
- `try_make_structured_specialization_key` intentionally rejects missing or
  mismatched structured mirrors by returning `nullopt`; callers should keep
  using the legacy key in that case instead of fabricating partial identity.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The selected subset
ran 322 tests with 100% passing after the build, including the updated
`cpp_hir_template_parameter_binding_key_structured_metadata`.
