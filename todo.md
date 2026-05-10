Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Use Structured Keys for Pending and Specialization Identity

# Current Packet

## Just Finished

Completed the next bounded Step 4 packet of `plan.md`: function-template seed
and instance production can now record structured `SpecializationKey` identity
when complete structured type/NTTP binding mirrors are available, and
`infer_call_result_type` uses the same structured-first lookup policy as direct
call lowering.

Concrete changes:
- Added structured type/NTTP binding mirrors to `TemplateSeedWorkItem` and
  `TemplateInstance`.
- Extended `InstantiationRegistry::record_seed` to build a structured
  specialization key through `try_make_structured_specialization_key` when the
  supplied mirrors are complete and count-compatible; incomplete metadata keeps
  the legacy specialization key path.
- Threaded structured maps from the main direct, deduced, enclosing, and
  consteval template seed collection paths.
- Updated `infer_call_result_type` to prefer structured instance lookup and
  retain the same legacy fallback used by direct call lowering.
- Added focused registry coverage proving realized instances born from
  structured seed metadata carry structured specialization argument keys while
  preserving legacy display canonical text.

## Suggested Next

Continue Step 4 by auditing remaining struct/global specialization materialize
paths that still construct `SpecializationKey` only from legacy rendered maps,
or add an end-to-end same-spelled-owner collision test before deciding whether
Step 4 is ready to move on.

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

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The selected subset
ran 322 tests with 100% passing after the build, including the updated
`cpp_hir_template_parameter_binding_key_structured_metadata`.
