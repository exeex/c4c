# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp` and re-ran the compile-boundary probe after the
template-base parser fixture migration. The field was restored after the probe.

The first remaining compile boundary is in
`tests/frontend/frontend_parser_tests.cpp`: line 3529 still reads
`alias_ts->tag` in
`test_parser_top_level_typedef_uses_unresolved_identifier_type_head_fallback`.
The same compile wave then reports more direct frontend parser fixture/assertion
accesses, starting at lines 4178, 4253, 4316, 4341, 4452, 4808, 5330, 5649,
5723, 5799, 5894, 5979, and later template-instantiation/record-definition
checks around lines 6136-6325.

## Suggested Next

Migrate the next frontend parser test residuals off direct `TypeSpec::tag`
access, beginning with the line 3529
`test_parser_top_level_typedef_uses_unresolved_identifier_type_head_fallback`
boundary, then re-run the temporary deletion probe.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The focused frontend parser test still has the known pre-existing
  `namespace owner resolution should use the method owner TextId before rendered
  owner spelling` failure.
- The next compile boundary ownership is
  `tests/frontend/frontend_parser_tests.cpp`; this packet inspected but did not
  edit that file because it was listed as Do Not Touch except inspection.
- `src/frontend/parser/ast.hpp` is restored to its pre-probe state; the field is
  not deletion-ready yet.

## Proof

Proof output is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: command exited nonzero during `frontend_parser_tests.cpp` compilation
with `TypeSpec::tag` temporarily removed. `test_after.log` records the probe
output, beginning with:
`/workspaces/c4c/tests/frontend/frontend_parser_tests.cpp:3529:23: error:
'const struct c4c::TypeSpec' has no member named 'tag'`.

After restoring `src/frontend/parser/ast.hpp`, a normal
`cmake --build --preset default` completed successfully without overwriting
`test_after.log`.
