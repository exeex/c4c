Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Binding Identity Authority

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: inventoried HIR template binding-string
authority and recorded the first conversion boundary for implementation.

Classification:
- `TypeBindings` is still semantic authority when constructed by
  `build_call_bindings`, deduction helpers, selected struct/function patterns,
  template seeds/instances, pending template work items, materialization, and
  lowerer `FunctionCtx::tpl_bindings`. `tpl_bindings_by_text` is only a local
  TextId mirror for some lookups.
- `NttpBindings` is still semantic authority for NTTP call binding creation,
  forwarding fallback, specialization selection, pending work, template seeds
  and instances, consteval environments, and lowerer `FunctionCtx`.
  `NttpTextBindings` is a useful TextId compatibility bridge, not complete
  owner-aware identity.
- `NttpTextBindings` is currently a bridge keyed by parameter `TextId` for
  forwarded NTTPs and consteval/lowerer lookups. It lacks owner, parameter
  index, and domain metadata, so same-spelled parameters can still collide
  across owners if promoted to cross-owner authority.
- `SpecializationArgumentIdentity::parameter_name` participates directly in
  equality, ordering, and hashing. `make_specialization_key`,
  `make_pending_template_type_binding_identities`, and
  `make_pending_template_nttp_binding_identities` fill it from rendered string
  map keys, so specialization and pending-template dedup still depend on
  rendered parameter names.

First conversion boundary:
- Start at `src/frontend/hir/hir_ir.hpp` by introducing a structured HIR
  template parameter binding key beside the legacy string map aliases. The key
  should carry parameter kind, owner namespace/context or owner text identity,
  parameter index, and parameter spelling `TextId`; raw `TextId` alone is not
  sufficient.

Lookup/hash/dedup boundaries:
- Type binding lookup boundary: `build_call_bindings`,
  `bind_deduced_type`, `find_enclosing_type_binding_for_deduction`,
  `populate_template_type_text_bindings`, and lowerer lookups through
  `ctx.tpl_bindings` / `ctx.tpl_bindings_by_text`.
- NTTP lookup boundary: `build_call_nttp_bindings`,
  `build_call_nttp_text_bindings`, `eval_template_arg_expr_with_nttp_bindings`,
  `lookup_nttp_binding`, and consteval `ConstEvalEnv` NTTP forwarding.
- Specialization hash boundary: `SpecializationArgumentIdentity` equality,
  ordering, and `specialization_argument_identity_hash`, plus both
  `make_specialization_key` overloads.
- Pending-template dedup boundary: `make_pending_template_type_key`,
  `PendingTemplateTypeKey`, `PendingTemplateTypeKeyHash`,
  `record_pending_template_type`, and the pending/resolved key sets in
  `CompileTimeState`.
- Function-template dedup boundary: `InstantiationRegistry::record_seed` and
  `realize_seeds` already prefer `primary_def + SpecializationKey`, but the
  `SpecializationKey` arguments still carry rendered parameter-name identity.

## Suggested Next

Implement the first bounded packet for Step 2: add the structured HIR template
parameter binding key type, equality/ordering/hash support, and structured
type/NTTP binding map aliases in `hir_ir.hpp` without changing lookup behavior.
Document legacy `TypeBindings`, `NttpBindings`, and `NttpTextBindings` as
compatibility mirrors until construction and lookup paths are converted. Use
this matching CTest route for the next Step 2 implementation packet:
`ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)'`.

## Watchouts

- Do not treat rendered names, mangled names, or raw `TextId` alone as complete
  semantic identity when owner-aware metadata is available.
- Do not weaken tests or convert capability work into expectation-only changes.
- Keep routine implementation progress in this file rather than rewriting the
  source idea.
- `mangle_template_name`, `format_pending_template_type_key_for_display`,
  `encode_pending_type_ref`, and `SpecializationKey::canonical` are
  display/compatibility outputs; do not make them replacement semantic keys.
- The first implementation packet should not rewire deduction or pending dedup
  yet. It should only make the key/map vocabulary available and documented.

## Proof

Ran delegated proof command:
`ctest --test-dir build -N -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)' > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. This corrects the
prior Step 1 proof guidance: the old `^frontend_hir_tests$` regex matched no
tests in this build.
