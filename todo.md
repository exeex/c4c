Status: Active
Source Idea Path: ideas/open/01_shared_prepared_call_argument_source_selection.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Source-Choice Contract Mapping

# Current Packet

## Just Finished

Lifecycle switch completed. The AArch64 calls emission consolidation runbook
was parked because its reactivation prerequisite is still absent, and this
runbook now owns the missing shared prepared call-argument source-selection
fact.

## Suggested Next

Execute Step 1 of `plan.md`: map the exact selected `BeforeCall` argument
source-choice contract, including the required prepared fields and lookup or
attachment point for target emitters.

## Watchouts

- Do not consolidate or delete AArch64 `calls*.cpp` files in this prerequisite
  runbook.
- Do not encode AArch64-specific machine-node emission details in the shared
  prepared fact.
- Do not claim progress through testcase-shaped matching, expectation
  weakening, or wrapper-only renames of the existing local decision tree.

## Proof

Lifecycle-only switch. No implementation files were modified, so no build,
ctest, or `test_after.log` was required.
