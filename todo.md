Status: Active
Source Idea Path: ideas/open/379_aarch64_local_aggregate_copy_load_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair Same-Owner Pointer-Derived Copy Mismatch

# Current Packet

## Just Finished

Step 4 ran the focused `00216` proof after the local aggregate address repair.
The prior `foo` segfault at the stale local-slot-derived pointer load advanced:
the representative now executes through `foo` and fails as
`[RUNTIME_MISMATCH]`, not as a crash.

The first current bad fact is the `foo` local pointer-derived aggregate copy
`struct S ls21 = *pls` where `pls = &ls`. Expected bytes are `ls21: 1 2 3 4`;
the fresh proof produced corrupt trailing bytes (`ls21: 1 30 66 a7` in
`test_after.log`). This remains inside idea 379 because the first mismatch is
still a local aggregate copy/load-from-local-address publication failure, not
relocation or function-pointer-table dispatch.

## Suggested Next

Delegate Step 5 for the new first bad fact: localize why
`struct S ls21 = *pls` copies corrupt bytes after `pls = &ls`, decide whether
the remaining mismatch is a copy-width, byte-layout, load-from-local-address,
prepared-value, MIR handoff, or AArch64 frame-slot publication problem, then
repair the general local aggregate copy/load publication path without
special-casing `00216`, `foo`, `ls21`, `struct S`, stack offsets, registers, or
output text.

## Watchouts

Later observations in the same proof are not the first current failure:
`lv3` shows a byte-layout/subobject aggregate mismatch, `flow` shows a later
aggregate initializer mismatch, and `test_multi_relocs` prints `two/two/two`
instead of `one/two/three`. Keep the function-pointer-table/relocation
observation parked for lifecycle routing unless a later proof makes it the
first bad fact. Do not mix that owner into the current `ls21` packet.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00216_c$'; } > test_after.log 2>&1`

Result: build completed (`ninja: no work to do`), and the focused CTest
executed 1 test. `c_testsuite_aarch64_backend_src_00216_c` failed with
`[RUNTIME_MISMATCH]`, proving advancement past the prior `foo` segfault while
leaving a same-owner local aggregate copy/load mismatch to repair.
Proof log: `test_after.log`.
