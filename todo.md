Status: Active
Source Idea Path: ideas/open/153_parser_sema_record_tag_compatibility_table_retirement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Parser Record Tag Authority

# Current Packet

## Just Finished

Step 1 inventory completed for parser rendered-record-tag authority.

Production inventory:

- `src/frontend/parser/impl/parser_state.hpp:85` defines `defined_struct_tags`.
  Classification: compatibility mirror / legacy rendered definition-name set.
  Owner: `ParserDefinitionState`; semantic risk comes from callers that use the
  set for record completion or duplicate-definition decisions.
- `src/frontend/parser/impl/parser_state.hpp:88` defines `struct_tag_def_map`.
  Classification: compatibility mirror by contract; semantic risk comes from
  callers that consult it before structured carriers or Sema record-domain
  bindings.
- `src/frontend/parser/impl/types/struct.cpp:2501-2505` reads
  `struct_tag_def_map` in `parse_record_tag_setup` for a non-body record tag and
  returns the existing complete definition by rendered `qtag`.
  Classification: semantic authority. Owner route: parser record carrier
  production should prefer structured tag binding / Sema record-domain lookup
  before this map.
- `src/frontend/parser/impl/types/struct.cpp:2564-2576` reads and writes
  `defined_struct_tags` in `parse_record_tag_setup` to decide redefinition or
  C++ shadow naming by rendered `qtag`.
  Classification: semantic authority. Owner route: parser record definition
  registration should use structured record-domain identity before rendered set
  membership.
- `src/frontend/parser/impl/types/struct.cpp:2733-2734` writes
  `struct_tag_def_map[source_tag]` and `[sd->name]` in
  `register_record_definition`.
  Classification: compatibility mirror / final display spelling mirror. Owner
  route: keep only after structured tag bindings and record metadata are updated.
- `src/frontend/parser/impl/types/template.cpp:142-143` passes
  `struct_tag_def_map` to `resolve_record_type_spec` only for TextId-less
  legacy carriers after structured template static-member base matching fails.
  Classification: temporary fallback for incomplete metadata.
- `src/frontend/parser/impl/types/template.cpp:213-223` returns
  `struct_tag_def_map` only when static-member initializer layout contains a
  TextId-less legacy carrier; structured carriers reject the fallback.
  Classification: temporary fallback / compatibility bridge.
- `src/frontend/parser/impl/types/template.cpp:735-738` inserts an explicit full
  specialization under `*out_mangled` and records the rendered mangled tag in
  `defined_struct_tags`.
  Classification: semantic authority for template instantiated record
  registration. Owner route: template instantiated record registration.
- `src/frontend/parser/impl/types/template.cpp:750-766` uses
  `struct_tag_def_map.count/find(*out_mangled)` to deduplicate injected parse,
  fill `out_resolved.record_def`, and report success.
  Classification: semantic authority for template instantiated record lookup.
  Owner route: template instantiated record lookup should prefer structured
  instantiation keys and `record_def` before mangled rendered map keys.
- `src/frontend/parser/impl/support.cpp:180-182` exposes
  `Parser::has_defined_struct_tag` as a rendered-set query.
  Classification: compatibility/test hook unless used by production callers;
  no production caller was found in the inventory.
- `src/frontend/parser/impl/support.cpp:185-187` passes
  `struct_tag_def_map` into parser constant evaluation.
  Classification: compatibility bridge; `resolve_record_type_spec_for_constant_layout`
  restricts map recovery to TextId-less legacy/final-spelling carriers.
- `src/frontend/parser/impl/support.cpp:216-218` writes `struct_tag_def_map` in
  `register_struct_definition_for_testing`.
  Classification: test hook.
- `src/frontend/parser/impl/support.cpp:560-590` reads an optional
  compatibility map in `resolve_record_type_spec`.
  Classification: mixed compatibility bridge. It prefers direct
  `TypeSpec::record_def`, then structured context/tag TextId scans, then legacy
  rendered spelling; callers determine whether passing the parser map is safe.
- `src/frontend/parser/impl/support.cpp:593-612` reads the optional map in
  `resolve_record_type_spec_for_constant_layout`.
  Classification: temporary fallback for incomplete metadata; structured
  carriers are deliberately blocked from map recovery.
- `src/frontend/parser/impl/declarations.cpp:655-660` reads
  `struct_tag_def_map` for complete object declaration checks only after
  rejecting structured record carriers.
  Classification: temporary fallback for incomplete TextId-less metadata.
- `src/frontend/parser/impl/types/base.cpp:2893-2903` reads
  `struct_tag_def_map.find(mangled)` after
  `ensure_template_struct_instantiated_from_args` fails to return
  `resolved_owner.record_def` for qualified template member typedef lookup.
  Classification: semantic fallback. Owner route: member typedef path /
  template instantiated record lookup.
- `src/frontend/parser/impl/types/base.cpp:5136-5140` and `6620-6624` pass
  `struct_tag_def_map` to `resolve_record_type_spec` while recursively looking
  up static members in base records.
  Classification: compatibility bridge; safe only when the `TypeSpec` has
  direct/structured identity or is a legacy no-carrier fallback.
- `src/frontend/parser/impl/types/base.cpp:5202-5215` first asks
  `resolve_record_type_spec(owner_ts, &struct_tag_def_map)` after structured
  instantiation, then falls back to `find(owner_mangled)`.
  Classification: semantic fallback for template static-member NTTP owner
  lookup.
- `src/frontend/parser/impl/types/base.cpp:6696-6700` passes
  `struct_tag_def_map` to `resolve_record_type_spec(owner_ts, ...)` after
  structured instantiation.
  Classification: compatibility bridge, but it is on the same owner lookup
  route and should be checked with the `5202-5215` fallback.
- `src/frontend/parser/impl/types/base.cpp:7118-7125` reads
  `struct_tag_def_map.find(base_mangled)` inside
  `materialize_structured_base_record`, then validates the candidate against
  primary/template arguments.
  Classification: compatibility bridge with structured validation.
- `src/frontend/parser/impl/types/base.cpp:7514-7520`,
  `7658-7667`, and `7675-7685` read `struct_tag_def_map.find(base_mangled)` for
  instantiated template bases only when no structured carrier is available or
  instantiation failed.
  Classification: temporary fallback for incomplete metadata.
- `src/frontend/parser/impl/types/base.cpp:8097-8105` writes instantiated
  records to `struct_tag_def_map[mangled]`, inserts `mangled` into
  `defined_struct_tags`, and reads the map for existing instantiations.
  Classification: semantic authority for template instantiated record
  registration and lookup.
- `src/frontend/parser/impl/types/base.cpp:8191-8195` reads
  `struct_tag_def_map.find(owner_legacy_tag)` for member typedef resolution only
  when instantiated owner lacks `TypeSpec::record_def`.
  Classification: semantic fallback. Owner route: member typedef path /
  template instantiated record lookup.

Test hooks / coverage already seeding map drift:

- `tests/frontend/frontend_parser_lookup_authority_tests.cpp` contains stale
  rendered map probes for static-member lookup, initializer layout,
  `__builtin_types_compatible_p`, direct `record_def` authority, and sibling
  record disambiguation.
- `tests/frontend/frontend_parser_tests.cpp` contains compatibility-map coverage
  for legacy tags, structured record context matching, template instantiated
  bases, and direct map insertion/erasure behavior.
- `tests/frontend/cpp_hir_parser_*` includes residual metadata tests that seed
  stale rendered tags to prove structured metadata wins in HIR-facing helpers.

## Suggested Next

First implementation packet: demote ordinary parser record tag setup authority
before template/member-typedef cleanup.

Owned files for the packet:

- `src/frontend/parser/impl/types/struct.cpp`
- focused tests in `tests/frontend/frontend_parser_lookup_authority_tests.cpp`
  and/or `tests/frontend/frontend_parser_tests.cpp`

Packet scope:

- In `parse_record_tag_setup`, replace the non-template rendered `qtag`
  `struct_tag_def_map` complete-definition read and the `defined_struct_tags`
  duplicate/shadow decision with a structured lookup path based on existing tag
  type bindings / qualified record metadata where available.
- Keep `register_record_definition` map writes as explicit compatibility
  mirrors after structured registration succeeds.
- Add a stale/ambiguous rendered-tag test showing that structured parser/Sema
  record identity wins over `struct_tag_def_map` and `defined_struct_tags` for
  ordinary record setup.

## Watchouts

- Do not treat rendered record tags, mangled names, or raw `TextId` alone as semantic record identity.
- Do not claim progress by renaming `struct_tag_def_map` or `defined_struct_tags` while preserving rendered string authority.
- Do not weaken tests, mark supported record cases unsupported, or rewrite expectations only to match changed rendered names.
- Keep diagnostic, display, legacy AST mirror, and explicit compatibility uses separate from semantic lookup authority.
- Prefer Sema record-domain lookup or complete structured parser carriers over testcase-shaped matching.
- Template instantiated records still have clear semantic rendered-key authority
  at `template.cpp:735-766` and `base.cpp:8097-8105`; leave those for the later
  template packet instead of mixing them into the first ordinary-record packet.
- Several support/declaration/base paths are already guarded as TextId-less
  legacy fallbacks. Preserve those guards unless a code packet deliberately
  replaces the fallback with a structured/Sema owner lookup.

## Proof

Inventory-only lifecycle/todo slice; no build proof run and no `test_after.log`
created.

Supervisor-selected proof command for the first code packet:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_hir_parser_core_.*structured_metadata|cpp_hir_parser_type_base_.*structured_metadata|cpp_hir_member_typedef_.*structured_metadata|cpp_hir_template_.*structured_metadata)$'
```

The proof command matches the inventoried first packet because it covers parser
lookup authority, parser core/base structured metadata, member typedef metadata,
and template structured metadata buckets that can observe record-tag authority
drift.
