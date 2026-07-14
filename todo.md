# Current Packet

Status: Active
Source Idea Path: ideas/open/760_lir_string_constructor_deprecation_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make the va_list runtime-text boundary explicit

## Just Finished

- Step 2 complete: added `LirTypeRef::runtime_text` and used it only for
  `build_type_decls` generated storage-array and padding type text; migrated
  that function's va_list `i32` and `ptr` field types to `LirBuiltinType`.
  Generated LIR text and behavior are unchanged.

## Suggested Next

- Step 3: choose one subsequent coherent closed-set literal call-site group
  from the Step 1 inventory, preserve dynamic-text boundaries, and select its
  focused proof before making the next migration.

## Watchouts

- Do not remove or globally silence string construction warnings.
- Do not begin a repository-wide migration or rewrite HIR `TypeSpec` lowering.
- Keep dynamic aggregate, vector, struct, function, and generated array/padding
  text paths supported through explicit local boundaries where appropriate.
- Do not weaken verifier behavior, tests, or expected output.
- Do not retry a globally deprecated `const char*` constructor: it produces
  broad warning noise despite source-compatible compilation.

## Proof

- `cmake --build --preset default` passed (exit 0).
- `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`
  passed (exit 0); proof log: `test_after.log`.
