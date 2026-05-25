Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate The Affected Helper Boundary

# Current Packet

## Just Finished

Step 3 verified the affected outgoing stack extent helper/API boundary after
Step 2.

- `outgoing_stack_argument_bytes` is now an emission-only AArch64 helper over
  `prepare::PreparedCallPlan`.
- No surviving overload, declaration, or call site passes `bir::CallInst` to the
  helper.
- No obsolete retained-ABI size reconstruction remains in the helper path; the
  extent comes from prepared `destination_stack_size_bytes`.
- No code changes were needed for this consolidation packet.

## Suggested Next

Have the supervisor review and commit the Step 2/Step 3 outgoing stack extent
authority slice, then select the next retained call-emission authority leak for
the following packet.

## Watchouts

No remaining boundary work was found for `outgoing_stack_argument_bytes`.
Hand-built tests that expect outgoing stack area setup must continue to provide
both `destination_stack_offset_bytes` and `destination_stack_size_bytes`;
AArch64 no longer reconstructs a missing size from retained BIR.

## Proof

No code changes were made, so no new build was required by the packet proof
contract.

Accepted Step 2 broad proof baseline: `test_before.log` reports passing
`162/162` `^backend_` tests.
