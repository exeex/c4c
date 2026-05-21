Status: Active
Source Idea Path: ideas/open/379_aarch64_local_aggregate_copy_load_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair Same-Owner Pointer-Derived Copy Mismatch

# Current Packet

## Just Finished

Step 5 localized the `struct S ls21 = *pls` mismatch to AArch64/prealloc
frame-slot publication, not semantic BIR copy width: BIR already copied four
`i8` lanes from the pointer-derived load, but stack layout had let the
`%lv.ls21.N` lane slots become sparse while call lowering published
`%lv.ls21` as the address of the first lane.

The repair now marks aggregate slice families whose root pointer value is
published to a non-LLVM call or pointer store as fixed home-slot families before
frame-slot assignment. This keeps pointer-published aggregate lanes contiguous
without applying the rule to unrelated sliced locals. The focused backend route
snippet for local aggregate address pointer copy passes again, and `00216`
now prints `ls21: 1 2 3 4`.

## Suggested Next

Delegate the next packet for the new first bad fact in `00216`: `flow` now
prints corrupt bytes after the `lt2` line even though `ls21`, `lu22`, and `lv3`
match expected output. Localize whether `flow` is a pointer-derived aggregate
load/store, prepared frame-slot coverage, call-boundary preservation, or
subobject byte-layout publication issue.

## Watchouts

The function-pointer-table/relocation observation remains later in the same
proof: `test_multi_relocs` still prints `two/two/two` instead of
`one/two/three`, but it is no longer the first mismatch. Keep it parked unless
a later proof makes it first. Do not turn the current aggregate-copy route into
a relocation/table-dispatch repair.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|c_testsuite_aarch64_backend_src_00216_c$)'; } > test_after.log 2>&1`

Result: build completed and CTest ran 147 tests. All 146 `backend_` tests
passed. `c_testsuite_aarch64_backend_src_00216_c` failed with
`[RUNTIME_MISMATCH]`, but the former first bad fact is fixed:
`ls21: 1 2 3 4` now matches expected output. The first remaining mismatch is
`flow`, followed later by the known `two/two/two` relocation/table-dispatch
observation. Proof log: `test_after.log`.
