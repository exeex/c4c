# Current Packet

Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 7
Current Step Title: Classify remaining stale-home ownership

## Just Finished

Step 7 implemented the ALU source-selection repair for same-block
`load_local` operands in ordinary scalar ALU records.

The shared `find_prepared_same_block_load_local_source_producer` path can now
recover the prepared memory access by result value when the source-producer
instruction index is in the flattened producer index space rather than the
prepared-addressing block-local index space. The ALU caller now keeps the
producer identity so it uses the real `LoadLocalInst` while materializing the
operand from the prepared addressing source.

For the classified `00164` chain after `%t106`, emitted AArch64 now reloads
the real locals from `[sp]`, `[sp, #4]`, `[sp, #8]`, and `[sp, #12]` before
`%t113`/`%t114`/`%t115` instead of reading `%t109`-`%t112` stale homes from
`x13` and `stack+148`/`stack+152`/`stack+156`. This fixes the first bad
`00164` output line (`46`) and preserves the focused probe tests.

## Suggested Next

Stay in Step 7 and classify the next `00164` stale-home site now exposed after
the fixed `and/xor/or` chain. The remaining bad `00164` lines are later
runtime values (`1916`/`1916` expected, large unstable values actual), while
`00204` remains the secondary stdarg signal.

## Watchouts

The current proof remains a 4/6 split, not fully green:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c` still fail, while the two focused
probes plus `00176`/`00181` pass. This is monotonic against the previous
baseline because the targeted `00164` post-call ALU chain now emits the
prepared local-source loads and the first mismatched `46` line is corrected.

The `00204` failure still starts in the stdarg output after otherwise-correct
argument and return-value sections; keep it as a secondary signal until the
smaller remaining `00164` failure is reduced.

Do not add testcase-shaped fixes for `00164`; prove the same-feature focused
ALU probes with the c-testsuite subset.

Do not resume `dispatch_value_materialization.cpp` close-readiness work until
the ALU-owned stale-home baseline is reduced or reclassified.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest returned a monotonic 4/6 split, 4/6
passed. Passing tests:
`backend_codegen_route_aarch64_alu_unpublished_load_local_after_call`,
`backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary`,
`c_testsuite_aarch64_backend_src_00176_c`, and
`c_testsuite_aarch64_backend_src_00181_c`. Failing tests:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c`.

Proof log: `test_after.log`.
