Status: Active
Source Idea Path: ideas/open/344_rv64_pointer_typed_select_publication_self_move.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Nearby Backend Routes

# Current Packet

## Just Finished

Step 3 proved the focused RV64 prepared edge-publication repair together with
nearby RV64 backend route coverage selected by the supervisor. The subset covers
pointer integer select chains, pointer-typed select publication, nested select
store-source publication, short-circuit select false-lhs publication, compare
result select false-arm publication, byval formal GPR publication, and the
prepared edge-publication unit test.

The focused publication route remains green with the forbidden-snippet contract
intact; no expectation downgrade or baseline-only proof was used.

## Suggested Next

Step 4 handback for broad proof. The supervisor should rerun the full CTest
command that previously exposed the unrelated `mv t0, t0` blocker, then decide
whether to close this backend blocker and return to the parked consteval inline
asm template string close review.

## Watchouts

- The Step 3 proof is still a focused and nearby subset, not full-suite proof.
- The forbidden-snippet expectations remain intact.
- `test_after.log` is the preserved Step 3 proof log.

## Proof

Ran the delegated Step 3 proof exactly:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '(^backend_riscv_prepared_edge_publication$|^backend_(dump|codegen_route)_riscv64_(pointer_integer_select_chain|pointer_typed_select_publication|nested_select_store_source_publication|short_circuit_select_false_lhs|compare_result_select_false_arm|byval_formal_gpr_publication)$)') > test_after.log 2>&1`.

Result: passed, `12/12` selected tests green. `test_after.log` is the preserved
proof log.
