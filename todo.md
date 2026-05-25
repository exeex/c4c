Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm The Call-Argument Lookup Boundary

# Current Packet

## Just Finished

Step 5 closure review rejected source-idea closure. The previous checkpoint's
broader backend proof passed, but `find_prior_preserved_value_for_call_argument`
still selects prior preservation through
`prepare::find_latest_indexed_prior_preserved_value` and falls back to raw
prepared-call-plan discovery and iteration when indexed call-plan lookups are
unavailable.

## Suggested Next

Execute Step 1: confirm whether call-argument prior preservation should use
latest-indexed or dominating prepared lookup semantics, whether all valid
lowering contexts should provide `call_plan_lookups` for this path, decide the
correct prepared owner or missing prepared-fact blocker, and record the focused
proof command here.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not broaden this checkpoint into dispatch cleanup or a whole calls-family
  rewrite.
- Do not claim helper renames, expectation rewrites, or testcase-shaped
  shortcuts as progress.
- Keep AArch64-specific operand construction in AArch64 code; the target of
  this checkpoint is the local call-argument prior-preserved-value selection
  boundary.

## Proof

No new validation was run for this lifecycle-only review. The closure decision
uses the executor-reported Step 4 proof now recorded in the rolled-forward
canonical `test_before.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed with 162/162 backend tests.
