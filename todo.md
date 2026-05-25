Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove Or Narrow The Lookup Boundary

# Current Packet

## Just Finished

Step 2 removed the local raw prepared-call-plan fallback from
`find_prior_preserved_value_for_call_argument`.

The call-argument prior-preservation lookup now fails closed when
`context.function.call_plan_lookups` is unavailable and otherwise consumes
`prepare::find_latest_indexed_prior_preserved_value`, matching the prepared
lookup contract needed by later call arguments that reuse the most recent
preservation home. The general value lookup remains dominance-based. AArch64
operand construction remains local in `make_prior_preserved_call_argument_source`.

The broader dispatch regression was caused by instruction-dispatch preservation
fixtures that manually built `FunctionLoweringContext` without attaching
`PreparedFunctionLookups`; those fixtures now attach the shared lookup facts
instead of relying on raw call-plan rediscovery.

## Suggested Next

Supervisor should review and commit this Step 2 slice, then choose the next
coherent packet from the active runbook.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not broaden this checkpoint into dispatch cleanup or a whole calls-family
  rewrite.
- Do not claim helper renames, expectation rewrites, or testcase-shaped
  shortcuts as progress.
- Keep AArch64-specific operand construction in AArch64 code; the target of
  this checkpoint is the local call-argument prior-preserved-value selection
  boundary.
- Tests that manually build AArch64 lowering contexts must attach
  `PreparedFunctionLookups` when they expect prepared lookup authority; the
  production lowering path already does this.

## Proof

Ran the delegated Step 2 proof successfully:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(aarch64_call_boundary_owner|call_boundary_effect_plan|prepared_lookup_helper|aarch64_instruction_dispatch)$'`

Result: passed, with combined build and CTest output captured in
`test_after.log`.
