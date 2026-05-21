Status: Active
Source Idea Path: ideas/open/379_aarch64_local_aggregate_copy_load_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Advancement And Reclassify

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

Delegate a follow-up Step 4/repair packet for the new first bad fact:
localize why `struct S ls21 = *pls` copies corrupt bytes after `pls = &ls`,
then repair the remaining local aggregate copy/load publication path without
special-casing `00216`, `foo`, `ls21`, stack offsets, registers, or output text.

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
