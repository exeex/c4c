Status: Active
Source Idea Path: ideas/open/585_target_abi_contract_and_value_consumption_research.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Assess Contract Field Sufficiency

# Current Packet

## Just Finished

Step 2 from `plan.md` is complete. Created
`docs/target_abi_contract_research/02_are_current_contract_fields_sufficient.md`
with the required sufficiency assessment for `CallArgAbiInfo`,
`CallResultAbiInfo`, `PreparedRegisterPlacement`,
`PreparedTargetRegisterIdentity`, move bundles, call plans, and preservation
plans. The answer classifies facts as semantic ABI, allocation-policy, or
physical target facts, compares AArch64 and RV64 requirements explicitly, and
states `sufficient with named limitations`.

## Suggested Next

Execute Step 3 from `plan.md`: produce
`docs/target_abi_contract_research/03_where_target_facts_are_split.md`.
List the code surfaces that currently own target ABI/register facts, mark each
split as healthy, suspicious, or accidental, and identify any smallest coherent
target ABI policy consolidation candidate without changing implementation.

## Watchouts

- Do not change implementation files, test expectations, unsupported markers,
  runtime behavior, source ideas, or closed idea files.
- Each numbered answer file must answer only its assigned question and follow
  the source idea's required answer shape.
- The final delivery must contain exactly one `index.md` plus exactly six
  numbered answer files under `docs/target_abi_contract_research/`.
- Step 2 classified the contract as `sufficient with named limitations`, not
  simply sufficient. The important limitations are asymmetric physical register
  identity publication, distributed preservation freshness authority, and
  destination-oriented move-bundle authority.
- Step 3 should treat those limitations as ownership-split evidence, not as an
  implementation mandate.

## Proof

Documentation-only proof. Verified the Step 2 file exists, answers only the
second research question, includes the required table for all named contract
surfaces, classifies facts as semantic ABI/allocation-policy/physical target
facts, compares AArch64 and RV64 explicitly, and states `sufficient with named
limitations`. No build required because no implementation files changed. Proof
log: `test_after.log`.
