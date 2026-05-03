# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, ran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` to the
pre-probe buildable state.

First remaining fixture/test compile boundary:
`tests/frontend/frontend_parser_tests.cpp.o` fails first at
`tests/frontend/frontend_parser_tests.cpp:4322` because
`arg.type.tag` still reads the deleted `TypeSpec::tag` member in
`test_parser_template_type_arg_uses_visible_scope_local_alias()`.

## Suggested Next

Migrate the first fixture/test residual group in
`tests/frontend/frontend_parser_tests.cpp` away from direct `TypeSpec::tag`
reads/writes, starting at the line 4322 boundary and keeping the replacement
semantic: prefer `tag_text_id` or the existing structured record/type authority
where available, and only preserve spelling fallback where the test explicitly
proves fallback behavior.

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
cmake --build build --target frontend_parser_tests > test_after.log 2>&1
```

Result: build failed while the `TypeSpec::tag` member was temporarily removed,
then `ast.hpp` was restored. The first compile boundary is the direct fixture
read at `tests/frontend/frontend_parser_tests.cpp:4322`.
