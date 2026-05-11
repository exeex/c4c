# HIR Rendered Registry Mirror Retirement Audit

Status: Closed
Created: 2026-05-11
Closed: 2026-05-11

Parent Ideas:
- `ideas/closed/161_hir_template_binding_domain_key_authority.md`
- `ideas/closed/162_link_name_id_backend_symbol_authority.md`

## Goal

Audit the remaining HIR rendered registries and retire or clearly fence any
string-keyed mirrors that are no longer needed as ordinary semantic authority.

This covers HIR module and compile-time registries that still expose rendered
maps beside structured mirrors, such as `Module::struct_defs`, `fn_index`,
`global_index`, `CompileTimeState` template/consteval registries, and lowerer
record/template registries.

## Why This Idea Exists

Several HIR registries now have structured mirrors after ideas 161 and 162,
but rendered maps still exist for compatibility, dumps, insertion order, or
legacy callers. That can be fine, but the remaining boundary should be made
explicit so future work does not accidentally treat rendered strings as the
primary semantic key again.

Candidate rendered registries include:

- `Module::struct_defs`, `struct_def_owner_index`, `fn_index`, `global_index`
- `CompileTimeState::template_fn_defs_`, `template_struct_defs_`,
  `template_struct_specializations_`, `consteval_fn_defs_`, `enum_consts_`,
  and `const_int_bindings_`
- Lowerer registries such as `struct_def_nodes_`, `template_struct_defs_`,
  `template_global_defs_`, `struct_static_member_decls_`, `struct_methods_`,
  constructor/destructor maps, and overload maps

## In Scope

- Inventory rendered registries in HIR module, compile-time state, and lowerer
  state.
- For each registry, classify it as structured authority, rendered
  compatibility mirror, insertion-order/display storage, diagnostic support,
  or route-local generated-name storage.
- Retire rendered maps that now have complete structured replacements and no
  compatibility caller.
- For retained maps, ensure lookup APIs prefer structured keys and only use
  rendered maps for explicit no-metadata compatibility.
- Add parity or fail-closed checks where both structured and rendered paths can
  resolve conflicting entities.
- Add focused tests for same-spelled records/templates/globals/functions in
  different structured domains.

## Out Of Scope

- Reworking template binding internals from idea 161.
- Reworking final backend `LinkNameId` transport from idea 162.
- Replacing route-local generated names such as SSA values, labels, slots, or
  dump-only strings.
- Removing user-facing diagnostics or final display spelling.

## Acceptance Criteria

- Remaining HIR rendered registries are classified.
- Covered lookup APIs prefer structured keys and do not silently fall back to
  rendered maps after complete structured misses.
- Unneeded rendered mirrors are removed or narrowed.
- Retained mirrors have owner, limitation, and removal condition documented.
- Tests prove structured-domain separation for at least records/templates and
  function/global lookup paths still using rendered mirrors.

## Closure Summary

The active runbook completed registry classification, retirement, narrowing,
and fencing across HIR module registries, compile-time registries, and lowerer
registries. Retained rendered maps are documented as compatibility,
display/order, diagnostic, no-metadata, or route-local generated-name bridges
rather than ordinary semantic authority for complete structured callers.

Final closure proof used the focused HIR registry coverage in
`test_before.log` and regenerated `test_after.log` with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests)$' > test_after.log 2>&1`.
Both logs passed `frontend_hir_tests` and `frontend_hir_lookup_tests` with no
failures. The close-time regression guard passed with
`--allow-non-decreasing-passed` because the focused baseline was already fully
green.

## Reviewer Reject Signals

- A rendered registry remains undocumented while being used as ordinary
  semantic authority.
- A complete structured miss falls back to a rendered map without an explicit
  no-metadata boundary.
- The route removes strings needed for display/order but leaves semantic lookup
  unchanged.
- Tests only update dump text.
