Status: Active
Source Idea Path: ideas/open/585_target_abi_contract_and_value_consumption_research.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Document Prepared Value Consumption

# Current Packet

## Just Finished

Step 4 from `plan.md` is complete. Created
`docs/target_abi_contract_research/04_current_prepared_value_consumption_model.md`
with the required prepared value consumption model. The answer describes the
current reuse/rematerialize/copy/fail order, cites concrete code paths for
value-home publication, call-argument source selection, prior preservation,
call-boundary effects, AArch64/RV64 consumers, and fail-closed diagnostics,
identifies explicit rematerialized producer kinds, and states that the model is
distributed rather than centralized.

## Suggested Next

Execute Step 5 from `plan.md`: produce
`docs/target_abi_contract_research/05_prior_preservation_freshness_and_stale_home_risk.md`.
Define freshness in the current prepared/prealloc contract, explain when prior
preservation is valid today, explain when producer rematerialization should
outrank prior preservation, cite concrete stale-home or missing-producer risk
evidence, and list the minimum facts needed to make preservation authority
explicit.

## Watchouts

- Do not change implementation files, test expectations, unsupported markers,
  runtime behavior, source ideas, or closed idea files.
- Each numbered answer file must answer only its assigned question and follow
  the source idea's required answer shape.
- The final delivery must contain exactly one `index.md` plus exactly six
  numbered answer files under `docs/target_abi_contract_research/`.
- Step 4 found the prepared value consumption model is distributed across
  value-home publication, call-plan source selection, preservation lookup,
  move-bundle/call-boundary effects, target backend emission, and diagnostic
  classifiers.
- Step 5 should focus on freshness and stale-home risk. It can build on Step 4's
  finding that prior preservation is selected only after direct source routes,
  but should not turn that into implementation changes.

## Proof

Documentation-only proof. Verified the Step 4 file exists, answers only the
fourth research question, describes the current reuse/rematerialize/copy/fail
decision order with concrete code paths, identifies explicit rematerialized
producer kinds, identifies consumer contexts that choose reuse or preservation
fallback, identifies fail-closed diagnostics that protect unknown authority,
and states that the decision model is distributed. No build required because no
implementation files changed. Proof log: `test_after.log`.
