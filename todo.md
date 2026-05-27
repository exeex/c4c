# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 8
Current Step Title: Consolidate proof and close readiness regression fix for remaining post-select value publication stale-home reads

## Just Finished

Partially completed Step 8 regression repair for remaining post-select value
publication stale-home reads.

The non-edge select-chain materialization call now keeps prepared producer
lookup authority but uses the consumer materialization instruction index as the
label seed. This removes duplicate `.Lselect_mat...` labels when the same
prepared select producer is materialized more than once.

The remaining stale-home runtime failures were not repaired in this packet.

## Suggested Next

Delegate the next packet to the ordinary AArch64 scalar ALU and call argument
publication route that decides whether unpublished same-block `load_local`
producers are read from their prepared memory access or from their assigned
result homes.

## Watchouts

The delegated four-test proof now passes the duplicate-label cases
`c_testsuite_aarch64_backend_src_00176_c` and
`c_testsuite_aarch64_backend_src_00181_c`, but still fails runtime checks in
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c`.

In `00164.c`, generated assembly after the select-materialized `%t106` call
continues to read later ordinary ALU operands from stale/uninitialized value
homes such as stack slots for `%t110`/`%t111`/`%t112` and register `x13` for
`%t109`. The concrete failing sequence is `%t113 = %t111 & %t112`,
`%t114 = %t110 ^ %t113`, and `%t115 = %t109 | %t114` after the `%t106`
`printf` call.

Speculative current-block `load_local` reload changes in
`dispatch_value_materialization.cpp` and direct operand-order changes in
`alu.cpp` were tried and backed out. The stale reads appear to be selected by
the ordinary scalar ALU/call-publication route before those unpublished load
results are materialized to their storage homes.

Do not implement `globals.cpp` or `fp_value_materialization.cpp` global-load
consumer rewrites under this active plan; those remain follow-up work under the
separate global consumer idea unless lifecycle state changes.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`.

Build succeeded; CTest passed 2/4 selected tests and failed 2/4 selected tests.
Proof log: `test_after.log`.
