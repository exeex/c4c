Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Forwarding and Lowerer Lookup Paths

# Current Packet

## Just Finished

Completed the delegated Step 5 packet of `plan.md`: converted builtin
`sizeof`/`alignof` type-query resolution to reuse the structured-aware
`Lowerer::find_template_type_binding_for_call` path, so complete `TypeSpec`
template-parameter owner metadata consults `FunctionCtx::structured_tpl_bindings`
before stale TextId/rendered mirrors and complete structured misses do not fall
through to those legacy maps. Added a focused `frontend_parser_tests.cpp`
regression for builtin query resolution covering both the structured hit and the
authoritative structured miss.

## Suggested Next

Continue Step 5 by checking any remaining forwarding/lowerer lookup paths that
still consume enclosing type bindings through legacy-only maps, using the same
complete-key-before-rendered-fallback rule.

## Watchouts

- Do not treat rendered names, mangled names, or raw `TextId` alone as complete
  semantic identity when owner-aware metadata is available.
- Do not weaken tests or convert capability work into expectation-only changes.
- Keep routine implementation progress in this file rather than rewriting the
  source idea.
- `frontend_hir_tests` is not enabled in the default build preset, so focused
  regression coverage for this packet lives in `frontend_parser_tests.cpp`.
- The template-call type-argument lookup now treats complete `TypeSpec`
  template-parameter owner metadata as covered structured metadata; stale
  `tag`, `tag_text_id`, `template_param_text_id`, or rendered-name maps must not
  be consulted after that structured miss.
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
- Builtin `sizeof`/`alignof` type-query lookup now shares that structured-first
  helper; future changes should keep complete owner metadata authoritative
  rather than reintroducing ad hoc TextId/rendered fallback in builtin paths.
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

Also ran the focused regression target directly:
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`

Result: command exited 0 with 1/1 test passing.
