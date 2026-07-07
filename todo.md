Status: Active
Source Idea Path: ideas/open/585_target_abi_contract_and_value_consumption_research.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace Target Information Entry

# Current Packet

## Just Finished

Activation created the active runbook and execution-state skeleton for
`ideas/open/585_target_abi_contract_and_value_consumption_research.md`.

## Suggested Next

Execute Step 1 from `plan.md`: produce
`docs/target_abi_contract_research/01_how_target_information_enters_the_pipeline.md`.

The packet should trace the actual code path from target triple/profile
creation through BIR call ABI facts, prepared/prealloc target register
placement, and RV64/AArch64 backend consumption. Keep the work read-only except
for the required research document and this `todo.md` progress note.

## Watchouts

- Do not change implementation files, test expectations, unsupported markers,
  runtime behavior, source ideas, or closed idea files.
- Each numbered answer file must answer only its assigned question and follow
  the source idea's required answer shape.
- The final delivery must contain exactly one `index.md` plus exactly six
  numbered answer files under `docs/target_abi_contract_research/`.

## Proof

Activation-only lifecycle change. No build or code validation required.
