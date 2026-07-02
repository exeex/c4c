Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Pin Direct Local-Slot Publication And Route Compatibility

# Current Packet

## Just Finished

Lifecycle rewrite after `review/local_memory_step6_route_review.md`.
Step 6 is no longer a producer-only coverage packet. The active runbook now
requires the next executor to pin direct local-slot address publication and the
prepared/codegen route compatibility for those same facts before reintroducing
implementation.

The source idea remains active and unchanged. The reviewer found the committed
local-slot producer work semantic and aligned, but the reverted Step 6 attempt
showed that direct local-slot address facts affect prepared/source identity and
byval aggregate route behavior.

## Suggested Next

Execute Step 6 by adding or extending focused tests that define:

- direct local-slot address publication for same-slot scalar store/load paths;
- prepared/route consumer behavior for those facts;
- byval aggregate and source-identity guardrails exposed by the blocked
  attempt.

Keep `src/20001026-1.c` as the first store-family representative proof seed,
not as the contract shape. After code or test changes, use backend-focused
proof first, then the selected RV64 backend-object representative proof when
delegated by the supervisor.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, and RV64/MIR inference.
Do not infer prepared/route compatibility from the RV64 row alone.

The blocked neutral attempt failed these existing expectations:

- `backend_codegen_route_riscv64_byval_aggregate_fixed_call`: missing snippet
  `lw t3, 32(sp)`.
- `backend_store_source_publication_plan`: expected BIR load-local source
  identity to match prepared oracle.
- `backend_aarch64_prepared_scalar_alu_records`: expected Route 3/prepared
  source mismatch to reject source-home operand.
- `backend_prepared_lookup_helper`: BIR load-local memory identity should match
  prepared semantic fields.

Step 7 should not resume implementation until Step 6 pins how those route
consumers should accept or semantically reject direct local-slot facts.

## Proof

Lifecycle-only rewrite. No code validation was run.
