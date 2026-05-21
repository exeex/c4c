Status: Active
Source Idea Path: ideas/open/375_aarch64_local_aggregate_bitfield_layout_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative And Classify Residuals

# Current Packet

## Just Finished

Step 4 proved the representative `00218` case no longer fails from the local
aggregate bit-field layout/store mismatch. Generated semantic BIR stores and
loads the bit-field storage unit through layout offset `16` (`%lv.node.16` /
`addr %p.node+16`), and generated
`build/c_testsuite_aarch64_backend/src/00218.c.s` stores the published unit at
`[sp, #16]` before the callee loads from `[x0, #16]`; the old mismatched local
store at `sp+2` is not present. `c_testsuite_aarch64_backend_src_00218_c`
passes, so there is no new first bad fact for this representative.

## Suggested Next

Supervisor should decide whether Step 4 completion is enough for lifecycle
closure or whether a plan-owner review/close packet is needed.

## Watchouts

Supervisor-side broader validation already passed `^backend_` after the Step 3
commit. This packet did not sample additional residuals because the delegated
representative passes and did not expose a new shared aggregate layout boundary
failure.

## Proof

Ran delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_x86_64_local_aggregate_bitfield_layout_observe_semantic_bir|c_testsuite_aarch64_backend_src_00218_c)$'; } > test_after.log 2>&1`.
Result: build succeeded; both tests passed:
`backend_codegen_route_x86_64_local_aggregate_bitfield_layout_observe_semantic_bir`
and `c_testsuite_aarch64_backend_src_00218_c`.
Proof log: `test_after.log`.
