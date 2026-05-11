Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Use Structured Keys for Pending and Specialization Identity

# Current Packet

## Just Finished

Completed the delegated Step 4 packet of `plan.md`: added a focused
`frontend_hir_tests.cpp` regression proving that two `Box<T>` struct template
primaries with the same rendered template and parameter spelling but different
owner metadata materialize structured explicit type-argument mirrors, prepare
different `TemplateStructInstanceKey` / structured specialization argument
identities, and still preserve equal legacy display canonical text.

## Suggested Next

Have the supervisor decide whether Step 4 is complete enough to move to the
next plan step or whether another narrowly scoped identity collision regression
is still needed.

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
- Direct template-call and call-result type lookup are structured-first, but
  still compare against a legacy key for compatibility with older or incomplete
  seed paths.
- `try_make_structured_specialization_key` intentionally rejects missing or
  mismatched structured mirrors by returning `nullopt`; callers should keep
  using the legacy key in that case instead of fabricating partial identity.
- `InstantiationRegistry::record_seed` still preserves `mangled_name` output
  from legacy binding maps; structured specialization keys affect dedup and
  lookup identity only when complete.
- `prepare_template_struct_instance` now follows the same complete-metadata
  requirement; incomplete struct argument mirrors still use the legacy
  specialization identity.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The selected subset
ran 322 tests with 100% passing after the build.
