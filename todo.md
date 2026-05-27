# Current Packet

Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 7
Current Step Title: Classify remaining stale-home ownership

## Just Finished

Step 7 classified the next exposed `00164` stale-home site after the repaired
post-`%t106` `and/xor/or` chain.

The next mismatch is the first `1916` output line from `a + b * c / f`. In
semantic/prepared BIR this is `logic.end.146`:
`%t156 = load_local %lv.a`, `%t157 = load_local %lv.b`,
`%t158 = load_local %lv.c`, `%t159 = mul %t157, %t158`,
`%t160 = load_local %lv.f`, `%t161 = sdiv %t159, %t160`, and
`%t162 = add %t156, %t161`, followed by `printf(%t162)`.

Prepared facts show `%t156` and `%t157` have register homes
`x13`/`x21`, but the same block also has prepared memory accesses for their
real local sources: `logic.end.146` instruction indexes 2 and 3 load frame
slots `#0` and `#1` (`[sp]` and `[sp, #4]`). The preceding call
`%t154 = printf(%t153)` explicitly clobbers `x13` and returns into `x21`.

Emitted AArch64 in `build/tmp/00164.current.s` lines 342-353 confirms the stale
read: after `bl printf`, it uses `w21` as `%t157` for `mul w9, w21, w10`, then
uses `w13` as `%t156` for `add w21, w13, w21`. It should reload `%lv.b` from
`[sp, #4]` and `%lv.a` from `[sp]` before those ALU consumers, just as the
earlier fixed chain now reloads `%lv.a`-`%lv.d`.

Ownership classification: stay in ALU source/materialization, specifically the
ordinary scalar binary source path when a `load_local` producer's prepared
home is a register invalidated or overwritten across an intervening call. This
does not look calls-owned: the prepared callsite records the `x13` clobber and
`x21` call result correctly. The missing piece is applying the shared
same-block `load_local` source-producer/addressing query to all ordinary scalar
ALU operands, including register-homed `load_local` producers, before falling
back to prepared result homes.

## Suggested Next

Implement the bounded ALU packet for the first `1916` site: in ordinary scalar
binary operand selection, prefer the shared prepared same-block `load_local`
source-producer plus prepared addressing for register-homed same-block
`load_local` operands before their ALU consumer. The focused target is
`%t157` feeding `%t159 = mul` and `%t156` feeding `%t162 = add` in
`logic.end.146`; the expected emitted shape is reloads from `[sp, #4]` and
`[sp]` before using those operands.

## Watchouts

The current proof remains a 4/6 split, not fully green:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c` still fail, while the two focused
probes plus `00176`/`00181` pass. The refreshed `00164` actual values for the
two `1916` lines are unstable large negatives, consistent with post-call
caller-saved/stale-register reads rather than deterministic arithmetic.

The `00204` failure still starts in the stdarg output after otherwise-correct
argument and return-value sections; keep it as a secondary signal until the
smaller remaining `00164` failure is reduced.

The next implementation should avoid treating only the `%t156`/`%t157` names
specially. The same rule should cover same-block ordinary scalar ALU operands
whose BIR producer is `load_local` and whose prepared memory-access fact exists
before the ALU consumer, even when the prepared result home is a register.

Do not add testcase-shaped fixes for `00164`; prove the same-feature focused
ALU probes with the c-testsuite subset.

Do not resume `dispatch_value_materialization.cpp` close-readiness work until
the ALU-owned stale-home baseline is reduced or reclassified.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest returned the expected 4/6 split, 4/6
passed. Passing tests:
`backend_codegen_route_aarch64_alu_unpublished_load_local_after_call`,
`backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary`,
`c_testsuite_aarch64_backend_src_00176_c`, and
`c_testsuite_aarch64_backend_src_00181_c`. Failing tests:
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00204_c`.

Proof log: `test_after.log`.

Classification artifacts generated under `build/tmp/`: `00164.semantic-bir.txt`,
`00164.prepared-bir.txt`, and `00164.current.s`.
