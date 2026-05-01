# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: none
Current Step ID: Step 2.5
Current Step Title: Preserve Parser Const-Int Boundary And HIR Blocker

## Just Finished

Step 2.5 completed the parser const-int boundary audit and parser-scope
cleanup. Parser-owned constant-evaluation callers that still passed only the
rendered struct-tag bridge now route through `eval_const_int_with_parser_tables()`
or pass `binding_state_.const_int_bindings` alongside the tag map, preserving
`TextId`-keyed named constant lookup where parser metadata is available.

The parked HIR rendered-name `eval_const_int` compatibility overload remains in
place and was not touched; migrating `hir::NttpBindings` metadata is still out
of scope for this parser packet.

## Suggested Next

Supervisor should review and commit this Step 2.5 parser-scope slice, then
decide whether the active runbook has any remaining parser-owned cleanup or
needs plan-owner/reviewer routing before any HIR metadata work proceeds.

## Watchouts

- Direct `eval_const_int` calls remaining in parser-owned files are either the
  helper implementation/recursion, already pass `const_int_bindings`, or are
  tag-only structural constant cases; the HIR calls under `src/frontend/hir`
  still intentionally use the rendered-name compatibility overload.
- Do not delete the rendered-name compatibility overload until the HIR
  `NttpBindings` metadata carrier exists under the separate HIR metadata idea.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus
1108/1108 passing `^cpp_` tests.
