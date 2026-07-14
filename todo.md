# Current Packet

Status: Active
Source Idea Path: ideas/open/760_lir_string_constructor_deprecation_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Repair the targeted warning-inventory route

## Just Finished

- Step 5 validation complete: accepted slices `a1d6fd79a`, `c4c021559`, and
  `896779447` retain their focused passing proofs; a final fresh build plus
  `^(frontend_lir_|backend_lir_)` passed 6/6, and the accepted full baseline
  was 3034/3034 passing. Closure was rejected because the source's targeted
  warning-inventory criterion remains unmet.

## Suggested Next

- Step 6: establish and prove a narrowly scoped targeted deprecation-warning
  inventory boundary for one remaining construction category, without
  restoring the rejected global `const char*` annotation.

## Watchouts

- Do not remove or globally silence string construction warnings.
- Do not begin a repository-wide migration or rewrite HIR `TypeSpec` lowering.
- The global `const char*` warning experiment was rejected because it emitted
  widespread unrelated warnings; the repaired route must remain local.
- Keep dynamic aggregate, vector, struct, function, and selected-authority
  paths supported through explicit local boundaries where appropriate.
- Do not weaken verifier behavior, tests, or expected output.
- Do not retry a globally deprecated `const char*` constructor: it produces
  broad warning noise despite source-compatible compilation.

## Proof

- Accepted Step 5 evidence: fresh `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|backend_lir_)'`
  passed 6/6; the accepted full baseline was 3034/3034 passing.
