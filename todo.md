Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm The Call-Argument Lookup Boundary

# Current Packet

## Just Finished

Step 1 confirmed the selected lookup boundary is
`find_prior_preserved_value_for_call_argument` feeding
`make_prior_preserved_call_argument_source`.

The owner should be shared prepared-call lookup authority:
`context.function.call_plan_lookups` is populated by normal AArch64 prepared
lowering through `prepare::make_prepared_function_lookups`, and this path
should fail closed when that indexed lookup state is unavailable instead of
rediscovering `PreparedCallPlansFunction` and iterating raw `call_plans->calls`
inside AArch64 emission.

Call-argument prior-preservation sources should use dominating prior lookup
semantics, matching `find_prior_preserved_value_for_value`, not latest-indexed
lexical semantics. Non-dominating sibling preservation is not a valid prior
source for an argument; when a sibling consumer needs the preservation home, the
current call's prepared boundary effects own the home population before the
argument move.

No missing prepared fact blocks Step 2. Existing prepared facts cover the
selection boundary: indexed prior preserved values plus control-flow dominance
are available in `PreparedCallPlanLookups`/`PreparedControlFlowFunction`.

## Suggested Next

Execute Step 2 by deleting the raw prepared-call-plan fallback from
`find_prior_preserved_value_for_call_argument` and switching the indexed lookup
consumer to `prepare::find_dominating_indexed_prior_preserved_value`, preserving
AArch64-local operand construction in `make_prior_preserved_call_argument_source`.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not broaden this checkpoint into dispatch cleanup or a whole calls-family
  rewrite.
- Do not claim helper renames, expectation rewrites, or testcase-shaped
  shortcuts as progress.
- Keep AArch64-specific operand construction in AArch64 code; the target of
  this checkpoint is the local call-argument prior-preserved-value selection
  boundary.
- Step 2 may need to update focused tests that manually build
  `FunctionLoweringContext` without attached `PreparedFunctionLookups`; valid
  lowering contexts should either attach `call_plan_lookups` or observe the new
  fail-closed behavior.

## Proof

No build or ctest was required for this read-only selection packet, and no
`test_after.log` was produced.

Intended focused Step 2 proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(aarch64_call_boundary_owner|call_boundary_effect_plan|prepared_lookup_helper)$'`
