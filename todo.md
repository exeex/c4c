Status: Active
Source Idea Path: ideas/open/633_aggregate_stack_home_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Or Verify Prepared Stack-Home Facts

# Current Packet

## Just Finished

Step 3 added explicit prepared-layer stack-home local-memory authority coverage.
`prepared_stack_home_local_memory_has_authority(...)` now verifies a
context-aware byval/sret pointer-value access against its prepared value home,
frame slot, stack object, ABI role, load/store direction, selected offset,
size, alignment facts, complete object extent, and proven in-bounds requested
range.

Focused contract coverage in `backend_prepare_stack_layout_test` proves:
- byval-param and sret-param stack-home local-memory lanes are accepted when the
  prepared carrier facts agree.
- role mismatches, missing load/store values, missing value homes, missing
  frame slots, mismatched stack object source kinds, incomplete extents,
  rejected ranges, non-default address spaces, volatile accesses, non-pointer
  bases, and 16-byte/F128-width lanes fail closed.
- The helper is semantic and context-driven; it does not match source filenames,
  copy-name spelling, or final assembly.

## Suggested Next

Step 4 code packet: wire RV64 object emission to consume only
`prepared_stack_home_local_memory_has_authority(...)` for the selected byval and
sret stack-home local-memory lanes. Keep admission narrow and add positive plus
fail-closed object-emission coverage for the same carrier facts.

## Watchouts

Step 4 should treat the new helper as necessary but not sufficient for RV64
emission: keep target-specific width, base materialization, stack-frame offset,
and immediate-encoding checks in the RV64 layer.

Same-block copied-value freshness was not folded into the authority helper
because it is a producer/source-use fact rather than an address/home fact. If
Step 4 depends on store-source freshness for sret stores, validate that through
the existing prepared source-producer lookup path instead of weakening the
stack-home address predicate.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains 1/1 passing
`backend_prepare_stack_layout`. `git diff --check` also passed.
