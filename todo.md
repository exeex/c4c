Status: Active
Source Idea Path: ideas/open/577_rv64_20000622_1_runtime_abort_after_call_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair The Underlying RV64 Object-Route Semantics

# Current Packet

## Just Finished

Step 3 repaired the semantic/materialization path for `d = (long)c` in
`src/20000622-1.c`. LIR-to-BIR lowering now emits a real `bir.ptrtoint` for
formal pointer parameters while preserving pointer-address metadata for address
reasoning, so `%t0` is no longer an unproduced scalar value. The RV64 prepared
object cast path now supports pointer-width cast results in stack homes, and
the focused pointer-cast object coverage was updated for that supported shape.

Current artifacts show the original classified `baz` wrong-argument family is
fixed: `baz` materializes from incoming `a2`, preserves the loaded local in
`s2` across `bar`, and passes `s2` as `foo` argument 0.

## Suggested Next

The representative still aborts, but it has advanced to a later family in
`foo`: after `a == 12`, the emitted logical condition treats `b != 0` as the
failure path, so `foo(12, 1, 11)` still reaches `abort`. The next packet should
classify/repair that logical OR/select publication behavior rather than the
now-fixed `baz` pointer-to-integer local materialization.

## Watchouts

- Do not edit expectations, unsupported markers, allowlists, runtime comparison
  behavior, or the gcc_torture runner.
- Do not add filename-specific handling for `src/20000622-1.c`.
- Preserve the same-module call/result behavior repaired under 572.
- The prior classified bad value in `baz` is fixed; do not regress the
  `a2 -> ptrtoint -> local -> s2 -> foo arg0` route.
- Remaining failure is in `foo` logical/select lowering, not `bar(a, 1)` result
  publication and not the original `baz` arg0 source selection.

## Proof

Focused proof log: `test_after.log`.
Combined representative proof log:
`build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/step3-combined-proof.log`.

Delegated combined proof was run exactly and preserved in the artifact log
above. `cmake --build --preset default` passed, and
`ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`
passed. The gcc_torture object case still fails with
`[RV64_BACKEND_RUNTIME_MISMATCH] clang_exit=0 c4c_exit=Subprocess aborted`,
now classified as the later `foo` logical/select condition family.
