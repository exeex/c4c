Status: Active
Source Idea Path: ideas/open/585_target_abi_contract_and_value_consumption_research.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Evaluate Preservation Freshness Risk

# Current Packet

## Just Finished

Step 5 from `plan.md` is complete. Created
`docs/target_abi_contract_research/05_prior_preservation_freshness_and_stale_home_risk.md`
with the required freshness and stale-home risk evaluation. The answer defines
freshness in the current prepared/prealloc contract, explains the valid
`PriorPreservation` authority path for live-across-call stack slots and
callee-saved registers, explains why verified producer rematerialization should
outrank old preserved homes, cites concrete stale-home and missing-producer
evidence from code and closed ideas, and lists the minimum facts needed to make
preservation authority explicit.

## Suggested Next

Execute Step 6 from `plan.md`: produce
`docs/target_abi_contract_research/06_closed_idea_tails_and_followup_questions.md`.
Review every relevant `ideas/closed/` file for target ABI contract, value home
freshness, publication, preservation, rematerialization, or move-bundle
authority tails; table the reviewed ideas; classify each tail; identify whether
it is already covered by an open idea; and list the remaining concrete
follow-up questions.

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
- Step 5 found that `PriorPreservation` is valid only as a unique, complete,
  dominance-valid preservation of a still-live value; it does not by itself
  prove latest semantic producer freshness.
- Step 6 should use Step 5's closed-idea citations as a starting point but must
  review every relevant closed idea requested by the source idea, not just the
  examples already cited.

## Proof

Documentation-only proof. Verified the Step 5 file exists, answers only the
fifth research question, defines current prepared/prealloc freshness, explains
when prior preservation is valid today, explains when producer
rematerialization should outrank prior preservation, cites stale-home and
missing-producer risk evidence from code and closed ideas, and lists the
minimum facts needed to make preservation authority explicit. No build required
because no implementation files changed. Proof log: `test_after.log`.
