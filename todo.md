# Current Packet

Status: Active
Source Idea Path: ideas/open/832_hir_aggregate_owner_function_parameter_crash_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: State the aggregate-owner parameter-lowering invariant

## Just Finished

- Step 2 repaired the LIR-owned aggregate function type relation at
  `lir_owned_type_spec`. Before any owner lookup, the LIR copy strips the
  non-owned AST `record_def` and qualifier pointers, resolves only its
  retained structured owner key against `Module::struct_def_owner_index`,
  and emits the matched owner tag. Missing or unmatched owners now throw a
  fail-closed lowering error rather than dereferencing stale metadata or
  falling back to text/default data.
- The object-helper `Box__Box::this` parameter now lowers through its valid
  module owner without the former crash. Nearby HIR-to-LIR coverage confirms
  missing tag, foreign tag, and incoherent namespace owner metadata reject
  safely; no truthiness, 830, or broad type-system work was changed.

## Suggested Next

- Step 3: retain the canonical before/after focused HIR proof, record the
  accepted aggregate-owner repair result, and return to 831 solely to
  activate the separately owned 833 truthiness route. Do not claim
  full-baseline clearance.

## Watchouts

- Do not add a null/default owner fallback, suppress the crash, weaken tests,
  or claim baseline clearance. 833 remains parked until this route returns
  accepted focused proof to 831.
- `typespec_aggregate_owner_key` is shared by layout and non-parameter paths;
  the repair remains at owned function type construction and does not alter
  that shared helper's behavior.

## Proof

- Evidence predecessor: `ba7958ee4` records the fresh exact 14/14 subset
  failure and the clean backend-enabled `f0fc85e4f^` HIR crash provenance.
  Step 1 must retain that provenance while narrowing the HIR owner seam.
- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_hir_tests$' > test_after.log`. The focused
  HIR subset passed 1/1; matching pre-repair failure is preserved in
  `test_before.log` and accepted proof in `test_after.log`.
