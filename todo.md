# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow In X86 packet by teaching
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` to accept the countdown
loop route through prepared `LoopCarry` ownership plus a trivial preheader
carrier block, instead of requiring a four-block `entry -> loop` CFG shape. The
new handoff proof in `tests/backend/backend_x86_handoff_boundary_test.cpp`
builds a loop fixture where entry reaches the loop header through a `preheader`
block, and confirms x86 still emits the canonical countdown asm when the
authoritative prepared predecessor labels and canonical phi-store carrier stay
in charge.

## Suggested Next

Stay in Step 3 and keep reducing countdown-loop CFG recovery by proving one more
prepared-control-flow-owned variant beyond the single-preheader case,
preferably a route where the loop header still uses the same authoritative
`LoopCarry` contract but the emitter must stop depending on one remaining local
block-shape assumption.

## Watchouts

- Keep this route in Step 3 consumer work; do not widen into Step 4 file
  organization, idea 57, idea 59, idea 60, idea 61, or the unrelated
  `^backend_` semantic-lowering failures.
- Countdown-loop consumer packets should keep preferring prepared
  predecessor-labeled `LoopCarry` transfers and prepared branch ownership over
  incidental block counts, direct `entry -> loop` topology, or mutable compare
  carriers in the loop blocks.
- The current x86 countdown specialization now tolerates a trivial preheader
  only when that carrier block is empty or holds the canonical prepared
  phi-store for the loop-carry slot; future Step 3 packets should treat that as
  a handoff contract boundary, not a license to grow emitter-local special
  cases.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3 handoff proof passed with the new countdown trivial-preheader
ownership coverage and preserved `test_after.log` at the repo root.
