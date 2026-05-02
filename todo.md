# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries added a structured-derived
owner bridge for deferred member typedef/template-owner resolution in
`src/frontend/hir/impl/templates/member_typedef.cpp` and
`src/frontend/hir/impl/templates/type_resolution.cpp`. For pending
`typename wrapper<T>::alias` style owners, the resolver now materializes
template arguments from the existing template owner carriers, selects the
template struct pattern, derives the instantiated owner name from the primary
template plus resolved arguments, and attempts member-typedef lookup through
that structured-derived owner before falling back to rendered `TypeSpec::tag`.

This is a structured-derived instantiated-owner spelling bridge into the
existing string-keyed member typedef registry, not the final structured
owner-key migration. Direct `owner_ts.tag` / `ts->tag` lookups remain explicit
no-metadata compatibility for routes that still lack a complete structured
owner carrier. Existing member-owner signature coverage remains green,
including `cpp_hir_template_member_owner_signature_local`,
`cpp_hir_template_member_owner_field_and_local`, and
`cpp_hir_template_member_owner_decl_and_cast`.

## Suggested Next

Continue Step 4 with one bounded frontend/HIR compile-failure cluster
migration. Suggested next packet: retry the bounded
`src/frontend/hir/hir_functions.cpp::substitute_signature_template_type`
signature/template-binding slice now that deferred member-typedef owners have a
structured-derived bridge before rendered tag fallback.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- Step 4 is a probe, not the final field removal. Do not commit a broken
  deletion build.
- Restore the worktree to a buildable state after recording probe failures.
- The first probe stopped in frontend/HIR compilation; later deletion probes
  may reveal additional parser/Sema/backend failures after these clusters are
  migrated or demoted.
- The parser `ast.hpp` helper cluster from the first probe is cleared for
  `encode_template_arg_debug_ref` and `typespec_mentions_template_param`;
  plain rendered template-parameter tag detection is no longer compatibility in
  that helper. `frontend_parser_lookup_authority_tests` now asserts this
  tag-only/no-carrier rejection.
- The HIR pending-type-ref display/debug cluster is cleared for
  `encode_pending_type_ref`; plain rendered tag spelling is intentionally not
  preserved there when the future no-tag field removal lands.
- The HIR canonical specialization-key display cluster is cleared for
  `canonical_type_str`; plain rendered tag spelling is intentionally not
  preserved there when the future no-tag field removal lands.
- The HIR layout TypeSpec lookup route is cleared for
  `find_struct_def_for_layout_type`; complete structured owner misses no longer
  fall back to rendered `struct_defs`.
- The HIR ABI/final-spelling mangling route is cleared for
  `type_suffix_for_mangling`; tag-TextId-only suffixes intentionally become
  deterministic id suffixes unless a record definition supplies final spelling.
- The HIR function specialization selector no longer matches tag-only nominal
  TypeSpecs after structured matching declines to decide.
- The HIR builtin layout route is cleared for
  `LayoutQueries::find_struct_layout`; complete structured owner misses no
  longer fall back to rendered `struct_defs`, and the no-metadata rendered
  `ts.tag` fallback has been intentionally removed.
- The HIR builtin query type route is cleared for
  `resolve_builtin_query_type`; `template_param_text_id` lookup is authoritative
  when present, and tag-only/no-metadata substitution is intentionally not
  preserved for the future no-tag field removal.
- The fifth deletion probe still stops in frontend/HIR compilation; no
  LIR/BIR/backend downstream metadata gap has been reached yet.
- `builtin.cpp` is cleared from the current first deletion-probe failure set.
  Remaining first-probe clusters are HIR call-lowering helpers and mixed HIR
  semantic/display consumers in `hir_functions.cpp`, `hir_lowering_core.cpp`,
  `hir_build.cpp`, and `hir_types.cpp`.
- Deferred member typedef owner resolution now has a structured-derived
  instantiated-owner spelling bridge in `member_typedef.cpp` and
  `type_resolution.cpp`; rendered `tag` fallback remains explicit
  compatibility for no-carrier routes, and this is not yet a final structured
  owner-key carrier.
- `build_template_mangled_name` may still use rendered nominal `TypeSpec::tag`
  for struct/union/enum/typedef template arguments; add stale-rendered proof or
  migrate that helper before removing remaining deferred-member fallbacks.
- `hir_functions.cpp::substitute_signature_template_type` should now be
  retried as a separate bounded packet, with the member-owner signature tests
  kept in the focused proof set.
- Do not create downstream follow-up ideas until a probe reaches LIR/BIR/backend
  failures after frontend/HIR compile blockers are cleared.
- Classify each failure as parser/HIR-owned, compatibility/display/final
  spelling, or downstream metadata gap instead of fixing broad clusters inside
  the probe packet.
- Do not create new follow-up ideas for parser/HIR work that still belongs in
  this runbook.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Keep downstream LIR/BIR/backend carrier gaps as separate follow-up ideas
  instead of broadening this runbook.
- Treat any `TypeSpec::tag` deletion build as temporary until Step 5.

## Proof

Step 4 deferred member typedef owner bridge proof passed with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|frontend_hir_lookup_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

Result: build passed and 62/62 selected CTest tests passed. Proof log:
`test_after.log`. `git diff --check` passed.
