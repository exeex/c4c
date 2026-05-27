# Current Packet

Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 7
Current Step Title: Classify remaining stale-home ownership

## Just Finished

Step 7 repaired ordinary scalar binary operand selection for register-homed
same-block `load_local` operands.

`ScalarFallbackOperandSelector` and the prepared scalar binary record override
now prefer `make_unpublished_load_local_source_operand` before accepting an
emitted/register home. That routes ordinary ALU operands through the shared
same-block `load_local` source-producer plus prepared addressing authority when
the source is available.

The shared same-block load-local query also now falls back by result value when
the producer instruction index finds a prepared memory access for a different
load result. It only replaces the producer `load_local` with the indexed block
instruction when that indexed instruction matches the recovered prepared access.
This covers the prepared-addressing/source-producer index divergence exposed by
`00164` `logic.end.146`.

The targeted `00164` `logic.end.146` sequence now reloads `%lv.b` from
`[sp, #4]`, `%lv.c` from `[sp, #8]`, and `%lv.a` from `[sp]` before the
`mul`/`add` consumers. The `00164` c-testsuite case now passes in the delegated
proof subset.

## Suggested Next

Classify the remaining `00204` segmentation fault in the same 6-test subset.
Start by confirming whether the failing stdarg/runtime section is still an ALU
source-materialization problem or should route to the calls prepared-authority
idea.

## Watchouts

The proof improved from the 4/6 baseline to 5/6. The remaining failure is
`c_testsuite_aarch64_backend_src_00204_c`, which exits with a segmentation
fault and no stdout/stderr in the delegated run.

The shared query change is intentionally semantic, not `%t156`/`%t157`-shaped:
it recovers by prepared result value only after an indexed producer/access
mismatch, then still requires the load-local result and prepared frame slot to
match.

Do not resume `dispatch_value_materialization.cpp` close-readiness work until
the ALU-owned stale-home baseline is reduced or reclassified.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest improved from the 4/6 baseline to 5/6. Passing
tests:
`backend_codegen_route_aarch64_alu_unpublished_load_local_after_call`,
`backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary`,
`c_testsuite_aarch64_backend_src_00164_c`,
`c_testsuite_aarch64_backend_src_00176_c`, and
`c_testsuite_aarch64_backend_src_00181_c`. Failing test:
`c_testsuite_aarch64_backend_src_00204_c`, with `[RUNTIME_NONZERO]` /
segmentation fault.

Proof log: `test_after.log`.

Classification artifacts generated under `build/tmp/`: `00164.semantic-bir.txt`,
`00164.prepared-bir.txt`, and `00164.current.s`.
