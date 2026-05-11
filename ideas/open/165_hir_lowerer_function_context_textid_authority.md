# HIR Lowerer Function Context TextId Authority

Status: Open
Created: 2026-05-11

Parent Ideas:
- `ideas/closed/161_hir_template_binding_domain_key_authority.md`
- `ideas/closed/162_link_name_id_backend_symbol_authority.md`

## Goal

Audit and convert HIR lowerer function-context lookup tables that still use
rendered strings as ordinary authority for local semantic lookup.

`Lowerer::FunctionCtx` still has string-keyed maps for locals, params, static
locals, function-pointer signatures, local constants, labels, and pack params.
Some of these are route-local display handles, but local/param/global semantic
lookups should prefer TextId or structured local-domain keys when AST metadata
is available.

## Why This Idea Exists

Idea 161 moved HIR template bindings to owner-aware keys, and idea 162 moved
link-visible backend symbols to `LinkNameId`. The remaining HIR lowerer
function-local maps are closer to lexical scope authority:

- `locals`
- `params`
- `static_globals`
- `local_fn_ptr_sigs`
- `param_fn_ptr_sigs`
- `local_const_bindings`
- `pack_params`
- `label_blocks`

Labels and some generated local handles may remain route-local strings. Name
lookup for source locals, params, static locals, local constants, and function
pointer signatures should not depend on rendered strings when TextId metadata
exists.

## In Scope

- Inventory `Lowerer::FunctionCtx` string maps and classify each as semantic
  source lookup, generated route-local handle, label control-flow handle,
  diagnostic/display, or compatibility bridge.
- Add TextId or structured local-domain maps for source locals, params, static
  locals, local constants, and function-pointer signatures where metadata is
  present.
- Prefer structured/TextId lookup before rendered string lookup for covered
  source-level names.
- Keep route-local SSA/local-slot labels and generated backend handles as
  strings when they do not represent source semantic identity.
- Add tests where same-spelled source names in different local domains or
  shadowing scopes resolve through structured local identity instead of raw
  string coincidence.
- Document retained string maps with owner, limitation, and removal condition.

## Out Of Scope

- Template binding maps already covered by idea 161.
- Link-visible function/global symbols already covered by idea 162.
- Replacing block labels or generated local slot names with global symbol ids.
- Full local SSA/value-id redesign.

## Acceptance Criteria

- FunctionCtx string maps are classified.
- Covered local/param/static/local-const/function-pointer signature lookups use
  TextId or structured local-domain metadata when complete.
- Route-local generated names remain strings only where they are not semantic
  source identity.
- Retained rendered-name fallbacks are documented compatibility bridges.
- Focused HIR tests cover shadowing or same-spelled local lookup behavior.

## Reviewer Reject Signals

- A source local/param lookup still uses rendered string first when TextId
  metadata is complete.
- Route-local labels/slots are incorrectly treated as module-level symbols.
- A structured miss reopens rendered lookup for covered metadata.
- Tests only check emitted spelling rather than lookup identity.
