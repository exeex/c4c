Status: Active
Source Idea Path: ideas/open/379_aarch64_local_aggregate_copy_load_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize First Bad Fact

# Current Packet

## Just Finished

Step 1 localized the first bad fact for idea 379 without implementation or
test edits. In `tests/c/external/c-testsuite/src/00216.c` `foo`, the crashing
generated sequence in `build/c_testsuite_aarch64_backend/src/00216.c.s` lines
435-441 maps to `const struct S *pls = &ls;` followed by
`struct S ls21 = *pls;`.

Semantic and prepared BIR contain the correct source operation:
`%lv.ls = bir.add ptr %lv.ls.0, 0`, `bir.store_local %lv.pls, ptr %lv.ls`,
then `%t167.pointer.aggregate.load.*` for the `ls21 = *pls` aggregate load.
Prepared stack metadata records address materialization for `%lv.ls.0` at
offset 0, but regalloc also assigns `%lv.ls.0` to spill slot `slot#427` at
`stack+496`. The emitted AArch64 uses that spill slot as the pointer source:
`ldr x13, [sp, #496]`, `str x13, [sp, #256]`, `ldr x10, [sp, #992]`,
`ldrb w9, [x10]`. There is no prior store publishing `sp + 0` into
`[sp, #496]`; the actual aggregate bytes for `ls` are written at
`[sp]..[sp,#3]`.

Classification: the missing publication is the aggregate address for `&ls`,
not the copied aggregate value, not the pointer-derived subobject load itself,
and not a wrapper-subobject load. The pointer-derived `*pls` aggregate load is
only the first consumer of the bad address.

## Suggested Next

Delegate Step 2 to add the smallest focused AArch64 backend coverage shape:
a local byte-sized struct aggregate initialized in frame memory, a local
pointer initialized with `&local_aggregate`, and a copied aggregate or scalar
check loaded through `*pointer`. The test should fail when `&local_aggregate`
is lowered by loading the aggregate element's spill slot instead of
materializing the frame address.

## Watchouts

Do not implement under umbrella idea 295. Keep `00200` and `00207` parked as
timeout-only buckets. Keep later `00216` relocation/function-pointer-table
audit parked until the current `foo` crash advances and fresh first-bad-fact
evidence justifies a handoff. Do not special-case `00216`, `foo`, one local
variable, one frame offset, one register, or the exact emitted `ldrb`
neighborhood. The focused fix should target local aggregate address
materialization/publication for address-taken aggregate storage; nearby direct
aggregate copies from `ls` into `lu1`/`lu2` already lower as byte
`bir.load_local`/`bir.store_local` copies and are not the first failing
boundary.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00216_c$'; } > test_after.log 2>&1`

Result: build was up to date; focused CTest still fails with
`[RUNTIME_NONZERO] .../00216.c exit=Segmentation fault`, matching the localized
first bad fact. Proof log: `test_after.log`.
