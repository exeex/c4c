Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Prove Store-Local Selected Publication Ownership

# Current Packet

## Just Finished

Completed the lifecycle rewrite after Step 5.

Step 5, GOT-required prepared `LoadGlobal` materialization, is already
committed. The supervisor-reported focused four-test shape is now `3/4`:
`backend_aarch64_instruction_dispatch`,
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`, and
`c_testsuite_aarch64_backend_src_00207_c` pass; only
`c_testsuite_aarch64_backend_src_00196_c` still fails.

`review/remaining_dirty_stack_acceptance_review.md` remains the active warning:
the remaining dirty implementation stack is useful context, but it is still a
multi-seam bundle and must not be accepted as one commit.

## Suggested Next

Next executor packet: Step 6 - Prove Store-Local Selected Publication
Ownership.

Packet boundary:

- Select or extract one focused store-local selected publication probe, using
  `tests/backend/case/aarch64_store_local_selected_publication.c` as the
  default candidate if the backend case harness can express the contract.
- Own only the store-local selected publication seam described in the reviewer
  report: the future-consumer suppression path around `memory.cpp` /
  `dispatch.cpp`.
- If code edits are delegated, do not touch call/outgoing stack argument,
  store-global, fused-compare, direct edge `LoadLocal`, or GOT `LoadGlobal`
  surfaces in this packet.
- If the seam cannot be proven without the broad dispatch integration test as
  the only evidence, stop with the dirty store-local code explicitly
  unaccepted and record the harness gap here.

## Watchouts

- The remaining dirty stack is not testcase-overfit by itself, but accepting it
  as one bundle would violate idea 60's decomposition route.
- Store-local future-consumer suppression is the highest drift-risk seam from
  `review/remaining_dirty_stack_acceptance_review.md`; prove that seam first
  or leave it unaccepted.
- `00196` remains an idea 58 runtime mismatch unless the focused store-local
  probe proves shared ownership.
- Do not rely on the `3/4` four-test result as the sole proof for any remaining
  dirty seam.

## Proof

Plan-owner lifecycle rewrite only. No build or ctest proof was run for this
rewrite.

Expected next executor proof shape:

```sh
cmake --build build -j
ctest --test-dir build -j --output-on-failure -R '<store-local-focused-probe>'
ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor|c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00207_c)$'
```
