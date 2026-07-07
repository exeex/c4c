Status: Active
Source Idea Path: ideas/open/585_target_abi_contract_and_value_consumption_research.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace Target Information Entry

# Current Packet

## Just Finished

Step 1 from `plan.md` is complete. Created
`docs/target_abi_contract_research/01_how_target_information_enters_the_pipeline.md`
with the required trace from `TargetProfile` creation through BIR call ABI
facts, prepared/prealloc register placement, and RV64/AArch64 prepared-surface
consumption.

## Suggested Next

Execute Step 2 from `plan.md`: produce
`docs/target_abi_contract_research/02_are_current_contract_fields_sufficient.md`.
Table the named contract surfaces and classify whether the current fields are
sufficient for the shared AArch64/RV64 contract.

## Watchouts

- Do not change implementation files, test expectations, unsupported markers,
  runtime behavior, source ideas, or closed idea files.
- Each numbered answer file must answer only its assigned question and follow
  the source idea's required answer shape.
- The final delivery must contain exactly one `index.md` plus exactly six
  numbered answer files under `docs/target_abi_contract_research/`.
- Step 1 found directionally healthy target flow, but also noted a concrete
  contract asymmetry: RV64 currently publishes explicit ABI
  `PreparedTargetRegisterIdentity` for call placements, while AArch64 consumes
  prepared call plans/register names after the public handoff clears regalloc
  functions.

## Proof

Documentation-only proof. Verified the Step 1 file exists, answers only the
first research question, follows the required answer shape from the source
idea, and cites concrete code paths for `TargetProfile` creation, BIR call ABI
facts, prepared/prealloc target register placement, and RV64/AArch64
prepared-surface consumption. No build required because no implementation files
changed. Proof log: `test_after.log`.
