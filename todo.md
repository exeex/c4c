# Current Packet

Status: Complete
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: none
Current Step ID: Step 2.4.4.6
Current Step Title: Delete Or Park The Remaining Member-Typedef Mirror

## Just Finished

Step 2.4.4.6 deleted the remaining rendered scoped member-typedef mirror:
`dependent_record_member_typedefs_by_key` was removed from parser template
state, and the full-member-key overload of
`find_dependent_record_member_typedef_type()` now only decomposes the structured
member key to the owner `QualifiedNameKey` plus member `TextId` carrier.

The parser/Sema audit found no remaining source readers/writers of
`dependent_record_member_typedefs_by_key`, `owner::member`, or
`source_tag::member` member-typedef semantic storage after this change.

## Suggested Next

Supervisor/plan-owner should decide whether Step 2.4.4.6 completes the current
runbook slice, or whether the next packet should target another parser/Sema
rendered-string semantic route under idea 139.

## Watchouts

- `c4c-clang-tool-ccdb` is installed, but the AST query attempt could not load
  `/workspaces/c4c/src/frontend/parser/impl/core.cpp` because
  `build/compile_commands.json` was absent in this workspace during the packet;
  the member-typedef mirror audit used targeted source search instead.
- `record_member_typedef_infos_by_key` still uses the existing structured full
  member key for alias-template RHS sidecar metadata; this packet only changed
  member typedef availability, not alias-template RHS metadata ownership.
- `clang-format` is not installed in this environment, so no automatic format
  pass was available.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus
1108/1108 passing `^cpp_` tests.
