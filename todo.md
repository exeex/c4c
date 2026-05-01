# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: none
Current Step ID: Step 2.5
Current Step Title: Preserve Parser Const-Int Boundary And HIR Blocker

## Just Finished

Step 2.4.4.6 completed the member-typedef mirror cleanup. It deleted the
remaining rendered scoped member-typedef mirror:
`dependent_record_member_typedefs_by_key` was removed from parser template
state, and the full-member-key overload of
`find_dependent_record_member_typedef_type()` now only decomposes the structured
member key to the owner `QualifiedNameKey` plus member `TextId` carrier.

The parser/Sema audit found no remaining source readers/writers of
`dependent_record_member_typedefs_by_key`, `owner::member`, or
`source_tag::member` member-typedef semantic storage after this change.

## Suggested Next

Delegate Step 2.5: Preserve Parser Const-Int Boundary And HIR Blocker.

The next packet should verify that parser-owned const-int lookup remains on
`TextId` or structured maps where available, while keeping the HIR
`NttpBindings` metadata migration parked under the existing HIR metadata idea.
This is primarily a boundary-preservation packet unless the supervisor chooses
a specific parser-owned const-int cleanup target.

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
- Step 2.5 must not edit `src/frontend/hir`; deleting the rendered-name
  `eval_const_int` compatibility overload belongs to the HIR metadata idea if
  that carrier migration is selected.
- Do not treat the parked HIR blocker as completed parser/Sema work.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus
1108/1108 passing `^cpp_` tests.

Lifecycle update: advanced the active packet pointer to Step 2.5. No code
validation was run for this todo-only lifecycle change.
