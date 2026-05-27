# Current Packet

Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 7
Current Step Title: Classify remaining stale-home ownership

## Just Finished

Step 7 classified the remaining `00164` stale-home failure without
implementation changes.

Generated prepared-BIR for `tests/c/external/c-testsuite/src/00164.c` shows
the target chain in `logic.end.100`: `%t107 = bir.call ... printf`, then
`%t109 = bir.load_local %lv.a`, `%t110 = bir.load_local %lv.b`,
`%t111 = bir.load_local %lv.c`, `%t112 = bir.load_local %lv.d`,
`%t113 = bir.and %t111, %t112`, `%t114 = bir.xor %t110, %t113`, and
`%t115 = bir.or %t109, %t114` (`/tmp/00164.prepared-bir.txt:267`).

The prepared homes and addressing facts are internally consistent but unsafe
to read as post-call result homes for this chain: `%t109` is homed in caller
saved `x13`, `%t110`/`%t111`/`%t112` are homed in stack slots
`stack+148`/`stack+152`/`stack+156`, while prepared addressing records their
real load-local sources as frame slots `#0`/`#1`/`#2`/`#3` at
`logic.end.100` instruction indexes 2-5 (`/tmp/00164.prepared-bir.txt:735`
and `/tmp/00164.prepared-bir.txt:1739`).

The emitted AArch64 confirms the stale-home read: immediately after the
`printf` for `%t106`, the code stores the call result to `stack+144`, then
loads `%t111`/`%t112` from `stack+152`/`stack+156`, loads `%t110` from
`stack+148`, and ORs with `w13` for `%t109`; it does not reload locals
`a`/`b`/`c`/`d` from `sp+0`/`sp+4`/`sp+8`/`sp+12`
(`build/c_testsuite_aarch64_backend/src/00164.c.s:260`).

Call ownership is not the best classification for this chain. The prepared
call plan already marks the preceding `%t107` call as clobbering `x13`, and
the stale `%t109`-`%t112` values are produced by `load_local` instructions
after that call, not by call arguments or call-boundary republishing
(`/tmp/00164.prepared-bir.txt:1384`). The owner is ALU operand/source
materialization still falling back to prepared result homes for ordinary
post-call `load_local` operands instead of consistently consuming the shared
same-block load-local source-producer/addressing authority.

## Suggested Next

Stay in the ALU route for one bounded implementation packet.

Smallest next packet: repair ordinary scalar ALU operand selection so a named
`load_local` operand in the same block before the consumer must prefer
`find_prepared_same_block_load_local_source_producer` plus prepared addressing
over the load result's assigned home, including register homes clobbered by a
prior call and stack homes that have not been materialized. Keep the existing
shared query in `publication_plans.*` unless the implementation proves it
cannot answer the ALU record path; in that case, add the narrow shared query
rather than an ALU-local scan.

## Watchouts

The current proof remains the delegated 4/6 split, not fully green:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c` still fail, while the two focused
probes plus `00176`/`00181` pass.

The classification is evidence for ALU source selection, not for switching to
`ideas/open/52_aarch64_calls_prepared_authority_repair.md`: `%t109` through
`%t112` are post-call load-local producers. Calls provide the clobber evidence
for `x13`, but do not own republishing those newly loaded locals.

The `00204` failure still starts in the stdarg output after otherwise-correct
argument and return-value sections; keep it as a secondary signal until the
smaller `00164` ALU chain is reduced.

Do not add testcase-shaped fixes for `00164`; prove the same-feature focused
ALU probes with the c-testsuite subset.

Do not resume `dispatch_value_materialization.cpp` close-readiness work until
the ALU-owned stale-home baseline is reduced or reclassified.

## Proof

No implementation proof was rerun for this classification-only Step 7 packet.
Existing canonical proof log remains `test_after.log`.

Last canonical proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest returned the delegated baseline split, 4/6
passed. Passing tests:
`backend_codegen_route_aarch64_alu_unpublished_load_local_after_call`,
`backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary`,
`c_testsuite_aarch64_backend_src_00176_c`, and
`c_testsuite_aarch64_backend_src_00181_c`. Failing tests:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c`.

Generated inspection commands used for classification:

`./build/c4cll --dump-prepared-bir --target aarch64-unknown-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00164.c > /tmp/00164.prepared-bir.txt`

`nl -ba build/c_testsuite_aarch64_backend/src/00164.c.s | sed -n '248,286p'`
