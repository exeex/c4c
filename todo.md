Status: Active
Source Idea Path: ideas/open/652_prepared_incoming_stack_formal_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Producer Authority Boundary

# Current Packet

## Just Finished

Lifecycle transition closed idea 644 after `cf784a80c` precisely classified
the RV64 object-route residual as a missing producer/prealloc publication
authority. The active plan now targets that producer/prealloc authority gap.

## Suggested Next

Execute Step 1: inspect the prepared producer/prealloc representation for the
stack-passed scalar formal family exposed by `src/20001017-1.c`, and determine
whether an explicit incoming caller-stack offset/address exists but is not
published, or whether the computation itself is missing upstream.

## Watchouts

- Do not reintroduce RV64 helpers that compute incoming offsets by walking
  `function.params`, applying ABI size/alignment, or adding
  `stack_frame_bytes`.
- Keep `IncomingStackToHome` plus callee local home distinct from explicit
  caller-stack incoming authority.
- Positive RV64 coverage should wait until the producer/prealloc fact exists;
  producer/prealloc coverage should prove the authority first.

## Proof

Close gate used the rolled-forward focused proof log:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed`

Result: PASS, with passed=6 failed=0 total=6 on both sides.
