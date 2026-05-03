# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, ran the delegated `c4cll` build probe, captured
the result in `test_after.log`, and restored `ast.hpp` to the pre-probe
buildable state.

First remaining compile boundary: none in the delegated `c4cll` target. With
`TypeSpec::tag` removed, `cmake --build build --target c4cll` rebuilt and
linked successfully.

## Suggested Next

Run the next deletion probe against the smallest supervisor-selected target
that compiles the fixture/test surface, likely `frontend_parser_tests`, so any
remaining direct `TypeSpec::tag` residual outside the `c4cll` target becomes
the next owned boundary.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The delegated `c4cll` proof does not compile parser fixture tests, so clean
  target build output is not final repository deletion readiness.
- The focused frontend parser test previously had the known pre-existing
  `namespace owner resolution should use the method owner TextId before rendered
  owner spelling` failure.

## Proof

Proof output is recorded in `test_after.log`.

```sh
cmake --build build --target c4cll > test_after.log 2>&1
```

Result: build completed successfully while the `TypeSpec::tag` member was
temporarily removed. No compile error boundary was present in this proof scope.
