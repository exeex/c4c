Status: Active
Source Idea Path: ideas/open/371_aarch64_indexed_aggregate_snapshot_writeback_regression.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Snapshot Writeback Boundary

# Current Packet

## Just Finished

Switched lifecycle from umbrella inventory idea 295 to focused follow-up idea
371 for the current dynamic indexed local/global aggregate
snapshot/writeback regression.

Plan-owner selected the leading Step 2 bucket rather than the next-ranked
single-test clean owner because `00157` and `00176` form the strongest current
multi-test semantic family with concrete generated-assembly evidence. The
route overlaps closed idea 348, so idea 371 is a new follow-up owner with an
explicit closed-scope comparison instead of an edit to the closed archive.

## Suggested Next

Execute Step 1: localize where selected aggregate element address authority is
lost for `00157` and `00176`.

Suggested initial proof/localization command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00157_c|c_testsuite_aarch64_backend_src_00176_c)$' > test_after.log 2>&1
```

## Watchouts

Do not reopen or edit closed idea 348. Do not special-case the representative
filenames, array names, stack offsets, registers, or emitted instruction
sequences. Keep scalar FP, conditional/switch, external-call return,
pointer/subobject, metadata, initializer, bit-field, and timeout buckets parked
under idea 295 unless fresh localization proves a handoff.

## Proof

Lifecycle-only switch. No implementation proof was run by plan-owner.
