Status: Active
Source Idea Path: ideas/open/585_target_abi_contract_and_value_consumption_research.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Review Closed Idea Tails

# Current Packet

## Just Finished

Step 6 from `plan.md` is complete. Created
`docs/target_abi_contract_research/06_closed_idea_tails_and_followup_questions.md`
with the required closed-idea tail review. The answer tables the relevant
closed ideas reviewed, summarizes each closure tail used as evidence,
classifies tails across target ABI policy, value freshness, publication,
preservation/rematerialization, move-bundle authority, or unrelated/historical
status, checks current `ideas/open/` coverage, and lists concrete follow-up
questions left after the research.

## Suggested Next

Execute Step 7 from `plan.md`: produce
`docs/target_abi_contract_research/index.md` and verify the required file set.
Link to all six numbered answer files, summarize the overall research result,
include the final recommendation table, and prove the directory contains
exactly `index.md` plus the six required numbered Markdown answer files.

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
- Step 6 found that current `ideas/open/` contains only the active research
  idea, so older closure tails that reference now-closed or absent follow-up
  ideas should be treated as historical evidence unless Step 7 recommends new
  implementation planning.
- Step 6 grouped the remaining questions around freshness authority,
  producer-rematerialization precedence, ABI-bound publication-routing reads,
  stale-row proof, and move-bundle source authority.

## Proof

Documentation-only proof. Verified the Step 6 file exists, answers only the
sixth research question, tables every relevant closed idea reviewed, summarizes
each closure tail used as evidence, classifies each tail as target ABI policy,
value freshness, publication, preservation/rematerialization, move-bundle
authority, or unrelated/historical, identifies current open coverage, and lists
remaining concrete follow-up questions. No build required because no
implementation files changed. Proof log: `test_after.log`.
