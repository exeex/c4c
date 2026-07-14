# Current Packet

Status: Active
Source Idea Path: ideas/open/760_lir_string_constructor_deprecation_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate a subsequent closed-set call-site group

## Just Finished

- Step 3 complete: migrated only the two no-expression `void` return paths
  and the null-pointer return path in `StmtEmitter::emit_control_flow_stmt`
  from static `LirTypeRef` literals to `LirBuiltinType` construction. Dynamic
  `ret_ty` construction and all other literal sites remain unchanged.

## Suggested Next

- Step 4: select the next bounded inventory category, migrate only its
  closed-set literals or add explicit local runtime-text boundaries, and run
  its focused proof.

## Watchouts

- Do not remove or globally silence string construction warnings.
- Do not begin a repository-wide migration or rewrite HIR `TypeSpec` lowering.
- Keep dynamic `ret_ty` and other aggregate, vector, struct, and function text
  paths supported through explicit local boundaries where appropriate.
- Do not weaken verifier behavior, tests, or expected output.
- Do not retry a globally deprecated `const char*` constructor: it produces
  broad warning noise despite source-compatible compilation.

## Proof

- `cmake --build --preset default` passed (exit 0).
- `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$' > test_after.log`
  passed (exit 0); proof log: `test_after.log`.
