# Current Packet

Status: Active
Source Idea Path: ideas/open/760_lir_string_constructor_deprecation_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repeat bounded groups and track remaining boundaries

## Just Finished

- Step 4 complete: migrated only the selected-byval memcpy pointer-authority
  `ptr` fields for the byval parameter and destination alloca in
  `populate_selected_byval_parameter_materialization_authority` to
  `LirBuiltinType::Pointer`. Selected-authority behavior and dynamic paths are
  unchanged.

## Suggested Next

- Step 5: validate the accepted migration state with fresh focused
  LIR/frontend/backend coverage, then choose closure or an explicit follow-up
  route.

## Watchouts

- Do not remove or globally silence string construction warnings.
- Do not begin a repository-wide migration or rewrite HIR `TypeSpec` lowering.
- Keep dynamic `ret_ty` and other aggregate, vector, struct, function, and
  selected-authority paths supported through explicit local boundaries where
  appropriate.
- Do not weaken verifier behavior, tests, or expected output.
- Do not retry a globally deprecated `const char*` constructor: it produces
  broad warning noise despite source-compatible compilation.

## Proof

- `cmake --build --preset default` passed (exit 0).
- `ctest --test-dir build -j --output-on-failure -R '^backend_lir_selected_pointer_authority$' > test_after.log`
  passed (exit 0); proof log: `test_after.log`.
