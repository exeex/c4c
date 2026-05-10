Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Use Structured Keys for Pending and Specialization Identity

# Current Packet

## Just Finished

Completed the next bounded Step 4 packet of `plan.md`: structured enclosing
template binding mirrors now propagate from `FunctionCtx` into the
`ctx-deduction-template-call-result` pending-template structured key path when
complete owner-aware metadata is available. Incomplete enclosing mirrors still
leave the structured key under-counted, so the existing legacy state-key path
remains active.

Concrete changes:
- Added structured type and NTTP mirror maps to `FunctionCtx`.
- Populated struct-method `FunctionCtx` structured mirrors from the enclosing
  template struct owner when available, with a method-template fallback for
  method-owned bindings.
- Merged enclosing structured mirrors with callee structured call/deduction
  bindings before building the `ctx-deduction-template-call-result`
  `PendingTemplateTypeKey`.
- Added focused coverage proving missing enclosing structured mirrors keep the
  legacy state-key path, while complete enclosing mirrors allow structured
  state-key authority.

## Suggested Next

Continue Step 4 by either wiring structured mirrors for non-method function and
global initializer `FunctionCtx` creation sites, or selecting the next
pending-template/specialization identity mutation point that already has
complete structured binding metadata.

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
- This packet populates structured enclosing mirrors for struct-method lowering
  where `struct_def_nodes_[struct_tag]` exposes the template struct owner. Other
  `FunctionCtx` creation paths still need their own metadata handoff before
  they can avoid the count-gated fallback.
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

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The selected subset
ran 322 tests with 100% passing after the build, including
`cpp_hir_template_parameter_binding_key_structured_metadata`.
