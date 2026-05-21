Status: Active
Source Idea Path: ideas/open/379_aarch64_local_aggregate_copy_load_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair Same-Owner Pointer-Derived Copy Mismatch

# Current Packet

## Just Finished

Step 5 follow-up localized the `flow` aggregate mismatch to subobject
byte-layout publication in AArch64/prealloc stack layout, not to semantic BIR or
call-boundary address publication. Semantic BIR already copied contiguous
`i8` lanes from `phdr->daddr` and `phdr->saddr`, and the AArch64 call lowered
`print(flow)` with the address of `%lv.flow.0`; the bad bytes came from
prepared frame slots assigning the fixed `%lv.flow.N` byte lanes at even
offsets because each lane inherited `align=2`.

The repair keeps fixed slice-family offsets when a dense byte-indexed family
would otherwise be padded by lane alignment. `%lv.flow.0..31` now occupy
contiguous byte offsets, so the pointer-published aggregate prints the expected
`flow: 9 8 7 6 ... 6 5 4 3 ...` sequence. An earlier broad value-home remap
was rejected during localization because it regressed prior aggregate prints;
the accepted change is scoped to stack-layout slice offset preservation.

## Suggested Next

Delegate the next packet for the new first bad fact in `00216`: the
function-pointer-table/relocation observation is now first, with
`test_multi_relocs` printing `two/two/two` instead of `one/two/three`.

## Watchouts

The function-pointer-table/relocation observation was parked while `flow` was
first. It is now the first remaining mismatch in the focused `00216` proof.
Treat it as a separate route decision rather than expanding the aggregate-copy
slice silently.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|c_testsuite_aarch64_backend_src_00216_c$)'; } > test_after.log 2>&1`

Result: build completed and CTest ran 147 tests. All 146 `backend_` tests
passed. `c_testsuite_aarch64_backend_src_00216_c` failed with
`[RUNTIME_MISMATCH]`, but the former first bad fact is fixed: `ls21`, `lu22`,
`lv3`, and `flow` now match expected output. The first remaining mismatch is
the known `two/two/two` relocation/table-dispatch observation. Proof log:
`test_after.log`.
