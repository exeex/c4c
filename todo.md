# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate HIR Type And Record Consumers

## Just Finished

Step 3 - Migrate HIR Type And Record Consumers migrated the deferred member
typedef resolver in
`src/frontend/hir/impl/templates/type_resolution.cpp::resolve_struct_member_typedef_if_ready`.
When a `TypeSpec` carries `record_def`, the resolver now treats that structured
record owner as authoritative after also trying template-origin metadata; it no
longer falls through to rendered `TypeSpec::tag`/`struct_def_nodes_` lookup
after a structured `record_def` miss. Rendered owner-tag lookup remains the
explicit compatibility path for TypeSpecs that lack `record_def` and template
origin metadata.

Focused coverage in `frontend_hir_lookup_tests` adds a stale rendered spelling
fixture where `record_def` points at an owner without the requested member
typedef while the stale rendered tag names a different owner that does define
the alias. The resolver now rejects the stale rendered fallback and leaves the
pending owner type intact.

## Suggested Next

Continue Step 3 with another bounded HIR `TypeSpec::tag` consumer where
structured owner metadata is already present. A good next packet is one
template/type route that still compares or indexes by rendered `tag`, such as
`src/frontend/hir/compile_time_engine.hpp` TypeSpec template-argument matching
or a narrow `src/frontend/hir/impl/templates/type_resolution.cpp` caller that
can pass `record_def`, `tag_text_id`, owner keys, or template metadata instead
of only rendered owner spelling.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- The deferred member typedef resolver still intentionally uses rendered
  `resolve_struct_member_typedef_type(tag, ...)` for no-metadata compatibility
  and for concrete realized template-origin base traversal after origin
  materialization.
- The default preset used for this packet does not register
  `frontend_hir_tests`; focused coverage for this route was therefore added to
  `frontend_hir_lookup_tests`, which the delegated regex compiles and runs.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Keep downstream LIR/BIR/backend carrier gaps as separate follow-up ideas
  instead of broadening this runbook.
- Do not attempt `TypeSpec::tag` field deletion in Step 3; removal belongs to
  the later deletion/probe and removal steps.
- Treat a `TypeSpec::tag` deletion build as a temporary probe only until the
  runbook reaches the removal step.

## Proof

Step 3 delegated proof passed with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

Result: build passed and 60/60 selected tests passed. Proof log:
`test_after.log`. `git diff --check` passed.
