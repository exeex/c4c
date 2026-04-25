Status: Active
Source Idea Path: ideas/open/102_hir_struct_method_member_identity_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Structured Mirrors for Struct Definitions

# Current Packet

## Just Finished

Completed the Step 3 dual-read/parity probe slice for template struct primary and specialization lookup without changing existing lookup authority.

Added lowerer-local parity probes in `find_template_struct_primary` and `find_template_struct_specializations` that compare the rendered-name / `ct_state_` result with the structured owner-key mirrors and record check/mismatch counters. Existing consumers still receive the rendered or `ct_state_` answer, and no HIR dump, codegen spelling, test, or expectation behavior was redirected.

## Suggested Next

Step 3 is ready to move to Step 4 unless the supervisor wants these private parity counters surfaced through a formal HIR inspection artifact first.

## Watchouts

The probes intentionally skip primary lookups with no rendered result because there is no owner-key metadata to compare from a bare name miss. Specialization parity treats absent/empty rendered and structured vectors as matching, and otherwise compares vector size/order/pointer identity. Template-struct specialization mirrors remain keyed by primary declaration owner identity, not by a full specialization identity.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L hir) > test_after.log 2>&1`.

Result: passed. `test_after.log` reports 73/73 HIR-labeled tests passing.
