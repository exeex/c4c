Status: Active
Source Idea Path: ideas/open/421_rv64_instruction_fragment_bucket_classification.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rank High-Impact Non-F128 Follow-Ups

# Current Packet

## Just Finished

Step 3 ranked high-impact ordinary non-F128 follow-up implementation candidates
from the Step 2 `unsupported_instruction_fragment` bucket evidence. The packet
created `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`, ordered
candidates by bucket count, expected broad impact, owning layer, and
prepared-fact risk, and cross-linked the ranking from
`docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`.

## Suggested Next

Execute Step 4 from `plan.md`: create durable open implementation ideas for the
highest-impact evidence-backed candidates, starting with RV64 scalar
select/join materialization, RV64 call-adjacent scalar and inline-asm
materialization, and RV64 pointer/address materialization with explicit
producer-gap boundaries.

## Watchouts

- Do not implement RV64 lowering while writing Step 4 idea files.
- Keep F128 quarantined and lowest priority unless future evidence proves it
  blocks a broader ordinary non-F128 owner.
- Keep producer-owned work separate: aggregate `sret`/`byval` rows with
  suspicious prepared size/alignment facts need ABI producer review before RV64
  aggregate lowering, and pointer rows with incomplete provenance must not be
  routed through RV64 address guessing.
- Each Step 4 idea should include concrete reviewer reject signals for
  testcase-shaped matching, expectation downgrades, producer-gap bypasses, and
  pass/fail accounting changes.

## Proof

Passed: `git diff --check > test_after.log 2>&1`.

Proof log: `test_after.log`.
