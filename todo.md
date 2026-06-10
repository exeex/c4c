Status: Active
Source Idea Path: ideas/open/151_phase_a_bir_normalization_candidate_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Classify Rejected Prepared Facts

# Current Packet

## Just Finished

Step 3 of `plan.md` classified the facts rejected from BIR normalization in
`docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`.

Completed reject groups:

- ABI register/stack placement
- stack offsets, frame slots, and saved-register placement
- register spelling, banks, views, occupied registers, and operand conversion
- scratch policy and aggregate transport lanes
- target addressing modes, TLS relocations, and base-plus-offset legality
- parallel-copy execution and publication routing policy
- publication hook, storage encoding, and typed stack-source emission payloads
- final instruction routing, ordering, hazards, and record/error payloads
- runtime helper, special-carrier, variadic, and dynamic-stack resource policy
- mixed publication/call/memory payloads from Step 2 candidate rows

Each reject row now names current owners and examples, the concrete rejection
reason, the required non-BIR owner, and the cross-check that the Step 2
candidate rows do not rely on smuggling target-local payloads into BIR.

## Suggested Next

Execute Step 4: order safe normalization routes in the artifact. Group the
accepted Step 2 candidate rows into route-sized implementation sequences, then
order them so producer/source identity precedes publication/edge, call, memory,
and comparison consumers.

## Watchouts

- This plan is analysis-only; do not edit implementation files or test
  expectations.
- Step 3 rejects are not dead code; they are required prealloc, ABI, stack,
  regalloc, MIR, or AArch64 ownership boundaries.
- Step 4 should preserve the split: BIR routes may depend on target-neutral
  producer/source/access identity, but not on homes, stack offsets, register
  spelling, ABI placement, TLS relocations, parallel-copy schedule, scratch
  policy, or final instruction records.
- Any route that imports a whole mixed `Prepared*Plan`, `Prepared*Publication`,
  `PreparedAddress`, or `PreparedValueHome` shape into BIR should be treated as
  route drift.

## Proof

Analysis-only/docs packet; no build required. Verification for this packet:
used `c4c-clang-tools` symbol inventories for the call, addressing,
publication, and value-location headers; inspected targeted owner snippets for
call plans, frame/register placement, address materialization, publication
plans, control-flow copy routing, and AArch64 instruction records; artifact
reject table has concrete owner/reason/cross-check entries; `todo.md` records
Step 3 completion. No `test_after.log` was created because the delegated proof
explicitly required no build.
