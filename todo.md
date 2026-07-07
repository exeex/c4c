Status: Active
Source Idea Path: ideas/open/585_target_abi_contract_and_value_consumption_research.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Map Target Fact Ownership Splits

# Current Packet

## Just Finished

Step 3 from `plan.md` is complete. Created
`docs/target_abi_contract_research/03_where_target_facts_are_split.md`
with the required target ABI/register ownership map. The answer lists the code
surfaces that own target ABI/register facts today, marks each split as healthy,
suspicious, or accidental, explains the shared x86/AArch64/RV64 regalloc
blocker, and identifies the prepared target register profile as the smallest
coherent consolidation candidate.

## Suggested Next

Execute Step 4 from `plan.md`: produce
`docs/target_abi_contract_research/04_current_prepared_value_consumption_model.md`.
Describe the current prepared value consumption order using concrete code
paths, identify explicitly rematerialized producer kinds, list consumer
contexts that choose reuse or preservation fallback, identify fail-closed
diagnostics, and state whether the model is centralized or distributed.

## Watchouts

- Do not change implementation files, test expectations, unsupported markers,
  runtime behavior, source ideas, or closed idea files.
- Each numbered answer file must answer only its assigned question and follow
  the source idea's required answer shape.
- The final delivery must contain exactly one `index.md` plus exactly six
  numbered answer files under `docs/target_abi_contract_research/`.
- Step 3 found the smallest coherent consolidation candidate to be the
  prepared target register profile layer, not BIR ABI lowering or backend
  emission.
- Step 4 should keep ownership separate from behavior: describe the current
  consume/reuse/rematerialize/copy/fail decision model without proposing
  implementation changes.

## Proof

Documentation-only proof. Verified the Step 3 file exists, answers only the
third research question, lists each code surface owning part of the target
ABI/register contract, states what each owns today, marks each split healthy,
suspicious, or accidental, explains the shared x86/AArch64/RV64 regalloc
blocker, and identifies the prepared target register profile as the smallest
coherent target ABI policy consolidation candidate. No build required because
no implementation files changed. Proof log: `test_after.log`.
