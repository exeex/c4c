Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Forwarding and Lowerer Lookup Paths

# Current Packet

## Just Finished

Completed the delegated Step 5 full-suite regression repair of `plan.md`.
The remaining `llvm_gcc_c_torture_src_20011008_3_c` abort came from folding
the runtime `flags` parameter through an enum-domain canonical spelling bridge.
`ConstEvalEnv` now keeps enum structured lookups on their source structured
keys only, while preserving the `link_name_texts` bridge needed for named
const-int globals. Present structured-map channels also keep miss authority
for complete keys instead of reopening rendered fallback just because a map is
empty.

## Suggested Next

Supervisor should review and commit this coherent Step 5 repair slice, then
choose the next Step 5 forwarding/lowerer packet if more direct rendered/TextId
lookup paths remain.

## Watchouts

- Do not treat rendered names, mangled names, or raw `TextId` alone as complete
  semantic identity when owner-aware metadata is available.
- Do not weaken tests or convert capability work into expectation-only changes.
- Keep routine implementation progress in this file rather than rewriting the
  source idea.
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
- `Lowerer::lookup_nttp_binding` no longer requires legacy mirrors for a
  complete structured hit when the query carries matching `TextId` metadata.
- `ConstEvalEnv` still treats rendered maps as no-metadata compatibility only;
  the new key bridge converts through `link_name_texts` and still ends in a
  structured map lookup.
- Deferred compile-time global sync now preserves structured global const-int
  authority for reduced globals such as `tile` and `cost`, so static-assert
  verification does not depend on flat rendered fallback.
- Do not use the `link_name_texts` canonical spelling bridge for enum
  constants; enum names can collide with locals, parameters, and fields in C,
  and those collisions must not recover through global enum maps.
- Empty structured maps retain covered miss authority for complete structured
  keys. This keeps a present structured channel from silently falling through
  to rendered/TextId compatibility after domain setup.
- The C torture case now keeps `(!(flags & 1) ? 1 : elp->u.l.ntxns)` dynamic
  instead of folding `flags` through an unrelated enum constant.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(llvm_gcc_c_torture_src_20011008_3_c|frontend_parser_lookup_authority_tests|cpp_c4_static_assert_if_constexpr_branch_unlocks_later|cpp_c4_static_assert_multistage_shape_chain|cpp_hir_.*|cpp_positive_sema_.*template.*|cpp_positive_sema_.*qualified_.*template.*)$'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The delegated proof
passed all 326 selected tests, including `llvm_gcc_c_torture_src_20011008_3_c`,
the lookup-authority tests, the consteval/static-assert tests, the `cpp_hir_.*`
coverage, and the positive template/qualified-template sema coverage. Also ran
the focused check
`ctest --test-dir build -j --output-on-failure -R '^(llvm_gcc_c_torture_src_20011008_3_c|frontend_parser_lookup_authority_tests|cpp_c4_static_assert_if_constexpr_branch_unlocks_later|cpp_c4_static_assert_multistage_shape_chain|cpp_hir_template_parameter_binding_key_structured_metadata)$'`,
which passed all five selected tests.
