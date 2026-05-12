Status: Active
Source Idea Path: ideas/open/201_hir_template_registry_structured_generated_paths.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Template Registry Authority Paths

# Current Packet

## Just Finished

Step 1: Inventory Template Registry Authority Paths.

Inventory across the four hot files found these registry authority paths:

- `src/frontend/hir/impl/expr/call.cpp`
  - `try_expand_pack_call_arg` uses `ct_state_->find_template_def("forward")`
    for the builtin forwarding helper. Classification: no-metadata
    compatibility for a compiler-provided rendered helper; retained only until
    the forwarding helper has an owned declaration identity in the generated
    call path.
  - `lower_call_expr` template-call branch gates on
    `ct_state_->has_template_def(n->left->name)` and then reopens
    `ct_state_->find_template_def(n->left->name)`. Classification:
    metadata-rich semantic authority for generated template call lowering, but
    currently rendered-name keyed. Limitation: the call node can build
    structured parameter bindings after `tpl_fn` is found, but the initial
    definition lookup itself is still rendered spelling authority.
  - `lower_call_expr` deduced-template branch uses
    `ct_state_->find_template_def(n->left->name)` to rebuild
    `TemplateCallInfo`. Classification: metadata-rich semantic authority for
    generated call metadata/replay; same rendered-name limitation.
  - Nearby structured helpers already build
    `HirTemplateTypeBindings`, `HirTemplateNttpBindings`, and structured
    specialization keys once a `tpl_fn` is available. These helpers are
    sufficient for a narrow fence; the missing carrier is the primary template
    identity preserved in `TemplateCallInfo`.
- `src/frontend/hir/impl/templates/deduction.cpp`
  - `try_infer_template_call_result_for_deduction` uses
    `ct_state_->find_template_def(call_node->left->name)` before building
    structured call/deduction bindings and pending-template structured identity
    keys. Classification: metadata-rich semantic authority for deduction result
    inference, currently entered through rendered spelling.
  - Nearby structured helpers include structured nominal matching, structured
    binding parity observation, structured pending-template keys, and
    `record_pending_template_type_with_identity_key`. Limitation: all of those
    depend on first recovering `callee_def` from rendered text.
- `src/frontend/hir/hir_build.cpp`
  - `collect_template_function_definitions` registers primaries by rendered
    name and structured declaration key. Classification: authoritative registry
    population; rendered name is retained as compatibility mirror.
  - `collect_function_template_specializations` uses
    `ct_state_->find_template_def(item->name)` before registering a
    specialization. Classification: metadata-capable semantic authority, but
    currently rendered primary lookup. Removal condition: route specialization
    collection through a primary declaration/key carrier when parser/sema
    exposes that owner.
  - `get_template_param_order_from_instances`, `record_template_seed`,
    `resolve_template_call_name`, `collect_template_instantiations`,
    `collect_consteval_template_instantiations`, and
    `instantiate_deferred_template` all use
    `ct_state_->find_template_def(name)` to obtain `primary_def` for seed,
    specialization, mangling, and deferred lowering. Classification:
    metadata-rich semantic authority for seed/collection/retry, with rendered
    compatibility still providing the primary lookup.
  - `materialize_hir_template_defs` iterates registered definitions and writes
    `Module::template_defs[name]`; this is preservation/display/deferred
    bookkeeping, not lookup authority by itself. Removal condition: when replay
    metadata carries complete structured owner identity, `Module::template_defs`
    can remain a preservation map only.
- `src/frontend/hir/impl/compile_time/engine.cpp`
  - `make_engine_consteval_env` wires structured template-struct primary lookup
    through `find_template_struct_def(owner.tpl_struct_origin_key)`.
    Classification: structured authority already present for template-struct
    consteval normalization.
  - Deferred template replay first checks
    `ct_state->has_template_def(tci.source_template)`, checks
    `ct_state->is_consteval_template(tci.source_template)`, reads
    `module.find_template_def_by_rendered_preservation_name(tci.source_template)`,
    then uses `ct_state->find_template_def(tci.source_template)` and
    `record_deferred_instance(tci.source_template, ...)`. Classification:
    metadata-rich semantic authority for generated HIR retry, but currently
    driven by rendered `TemplateCallInfo::source_template`. Limitation: the
    preservation map is explicitly documented as non-authoritative, yet replay
    still falls back to rendered compile-time registry lookup because
    `TemplateCallInfo` has no primary owner identity.

Retained compatibility paths:

- Owner: `CompileTimeState` rendered `template_fn_defs_` and `has/find_template_def(name)`.
  Limitation: valid only for no-metadata call sites and compatibility bridges.
  Removal condition: generated call, deduction, seed, and replay callers carry
  a complete primary template declaration identity or equivalent stable owner
  key.
- Owner: `Module::template_defs` and
  `find_template_def_by_rendered_preservation_name`. Limitation: preservation,
  display, parameter ordering, and deferred bookkeeping only; it must not
  resurrect semantic authority after a complete structured miss. Removal
  condition: deferred replay can build bindings/metadata from structured
  `TemplateCallInfo` owner identity without consulting rendered template name
  as authority.
- Owner: builtin `"forward"` pack expansion helper in `call.cpp`. Limitation:
  compiler-generated no-metadata compatibility. Removal condition: the helper
  is registered with a declaration/key carrier usable by generated calls.

First narrow implementation slice:

- Extend generated template call metadata to preserve the primary template
  identity already known in `call.cpp` when constructing `TemplateCallInfo`,
  then update deferred replay in `compile_time/engine.cpp` to prefer that
  identity for `has_template_def`, `is_consteval_template`, primary-def
  recovery, specialization keys, and `record_deferred_instance`.
- Keep rendered-name lookup only when the metadata has no complete structured
  owner. This fences one generated call/retry path first and avoids a broad
  rewrite of deduction, seeding, or the whole template registry.

## Suggested Next

Execute Step 2 by implementing the narrow generated call/replay fence in
`src/frontend/hir/impl/expr/call.cpp`,
`src/frontend/hir/impl/compile_time/engine.cpp`, and the minimal HIR metadata
type needed to carry primary template identity.

## Watchouts

- Do not treat rendered template spelling as semantic authority after a
  metadata-rich structured miss.
- Do not downgrade supported generated-template routes or rewrite expectations
  as a substitute for capability repair.
- Keep retained string-only registry paths explicitly classified with owner,
  limitation, and removal condition.
- `TemplateCallInfo` currently has only rendered source text/text-id plus
  argument bindings; replay cannot fail closed on a structured primary miss
  until that metadata grows an owner identity.
- Deduction and seed/collection callers remain live Step 3 work after the
  first call/replay fence lands.

## Proof

Inventory-only `todo.md` update; no build or test proof was required, and no
`test_after.log` was created or modified.
