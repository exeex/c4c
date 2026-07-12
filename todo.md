Status: Active
Source Idea Path: ideas/open/715_pass_ready_bir_schema_and_legacy_quarantine_research.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Produce The Target Schema And API Blueprint

# Current Packet

## Just Finished

- Completed plan.md Step 5: specified the target source tree, stable storage and
  APIs, verification/analysis boundaries, typed raw-to-canonical transition,
  and prepared BIR/MIR ownership for all target policy.

## Suggested Next

- Execute plan.md Step 6 and order reversible migration packets, proof and
  rollback boundaries, compatibility deletion gates, and follow-up proposals.

## Watchouts

- Preserve dependency direction: core cannot include compatibility,
  preparation, MIR, LIR, prealloc, or target headers.
- `CanonicalBir` is a verified move-only state wrapper over core storage, not a
  duplicated module; all target realization belongs to typed later outputs.

## Proof

- Supervisor-selected documentation proof passed:
  `git diff --check && test -f docs/backend/pass_ready_bir/05_target_schema_and_api_blueprint.md && rg -n "lower_lir_to_raw_bir|CanonicalBir|FunctionId|BlockId|InstId|ValueId|verif|analysis|compat|prepar|MIR|ABI|frame|allocation|instruction selection" docs/backend/pass_ready_bir/05_target_schema_and_api_blueprint.md`.
- This documentation-only packet does not produce `test_after.log`; no build or
  test subset was delegated.
