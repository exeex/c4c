Status: Active
Source Idea Path: ideas/open/370_aarch64_short_circuit_control_publication_regression.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Short-Circuit Control Publication

你該做baseline sanity check了

# Current Packet

## Just Finished

Steps 2 and 3 completed as one packet.

Focused backend coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` with
`short_circuit_join_consumes_edge_published_result()`. The test builds a
semantic short-circuit join where the skip edge publishes immediate `0` to the
join result while the RHS edge source is a same-block derived add/compare. It
asserts that the join block emits only the branch compare against the
edge-published result register and does not emit RHS-only `add`/`cset`
materialization.

The AArch64 repair is in
`src/backend/mir/aarch64/codegen/dispatch.cpp`. Dispatch now recognizes
expression instructions whose results are either direct incoming sources for
the current block's authoritative out-of-ssa parallel copy, or dependencies of
such incoming sources. Those RHS-edge expressions are skipped in the join
block, leaving the prepared predecessor edge publication as the owner of the
join value. This keeps the branch sense unchanged and avoids recomputing
RHS-only producers from stale registers after a skip edge.

The generated `00196` sequence now preserves the Step 1 branch fact and removes
the stale join recomputation. Around
`build/c_testsuite_aarch64_backend/src/00196.c.s` lines 147-171, the skip edge
publishes `mov x13, #0`, the RHS edge computes `(1 + joe()) != 0` before the
join, and `.LBB91_21` directly passes `x13` to `printf` without the previous
join-local `add ...; cmp ...; cset ...` sequence.

## Suggested Next

Supervisor review and commit of idea 370 Steps 2/3. The delegated proof is
green for all backend tests plus `00196` and the preserved `00112`
representative.

## Watchouts

The repair intentionally treats prepared out-of-ssa edge copies as authoritative
for join incoming expression values. It does not change branch lowering,
string-pointer comparison semantics, HFA/byval handling, or testcase
expectations. The focused test covers the stale RHS-derived add/compare shape;
the full delegated proof covers the idea 366 representative `00112`.

## Proof

Ran the delegated Steps 2/3 proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00112_c)$' > test_after.log 2>&1
```

Result: passed, `100% tests passed, 0 tests failed out of 145`.
`c_testsuite_aarch64_backend_src_00196_c`, `c_testsuite_aarch64_backend_src_00112_c`,
and `backend_aarch64_instruction_dispatch` are green. `test_after.log` is the
preserved proof log.
