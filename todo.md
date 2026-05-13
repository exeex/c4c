Status: Active
Source Idea Path: ideas/open/212_bir_mir_allocation_contract.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Capture Shared Carrier Gaps

# Current Packet

## Just Finished

Step 5 of `plan.md` reviewed the allocation audit, implemented-surface
alignment, and roadmap updates for missing shared BIR/prepared allocation
input or output carriers.

No separate shared-carrier gap blocks this allocation contract. The live
`PreparedBirModule` surface already carries the allocation-sensitive shared
facts this plan needs: value homes, regalloc assignments, storage plans,
spill/reload operations, frame and stack facts, liveness, register-class
overrides, call plans, move bundles, control-flow facts, and memory volatility
plus address-space facts.

The remaining non-present items are target-local AArch64 MIR records or later
target ABI policy: typed AArch64 register references, target MIR memory
operands, branch/call/return/move/spill/reload/data records, and complete
AAPCS64 call/return/variadic lowering policy. Those are not shared
BIR/prepared carrier gaps, so no new `ideas/open/` gap idea was created.

## Suggested Next

Execute Step 6: add focused proof for the allocation contract and
implemented-surface alignment without expanding backend capability scope.

## Watchouts

- Missing target-local AArch64 MIR records remain real follow-up work, but they
  should not be misclassified as shared BIR/prepared carrier gaps.
- AAPCS64 call/return/variadic completeness is an ABI-policy ambiguity for
  later target work, not a reason to patch around prepared facts locally.
- Any future route that needs a fact not carried by `PreparedBirModule` should
  stop and open a separate shared-carrier idea instead of recovering data from
  rendered names, legacy examples, or assembly text.

## Proof

`git diff --check` passed. Proof output is preserved in `test_after.log`; the
log is empty because the command produced no diagnostics.
