Status: Active
Source Idea Path: ideas/open/841_lir_compact_scalar_abi_leaf_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire accepted scalar text escape hatches

# Current Packet

## Just Finished

Completed plan.md Step 4 for idea 841 with one bounded retirement. The
ordinary modeled-result path now uses `LirBinOp.compact_scalar_type` as the
selected scalar result authority when the compact carrier is present, retiring
that path's direct `LirBinOp.type_str` result-type source. Kept
`LirBinOp.type_str` as compatibility/rendering parity text and preserved the
fallback for unmigrated/manual compatibility construction.

## Suggested Next

Supervisor review/commit for the Step 4 bounded retirement, then route to
plan-owner for the next lifecycle disposition if Step 4 satisfies the active
runbook.

## Watchouts

Do not implement 734 Raw-BIR receiver work in this idea. Do not assume opaque is
scalar, do not parse rendered text as scalar authority, and do not reopen
accepted receiver rows such as `LirAbsOp` selected-global/i32.

The compact scalar carrier remains intentionally optional for compatibility and
auto-derived from `LirBinOp.type_str` through aggregate initialization. Do not
delete `LirBinOp.type_str` yet: it is still the compatibility/rendering parity
field and other binop-adjacent paths may still reference it. This Step 4 slice
only retired the modeled-result `LirBinOp.type_str` source; it did not migrate
nonselected schemas, backend lowering, Raw-BIR receiver/importer/container
files, or pointer/void ABI-leaf authority.

## Proof

Step 4 implementation proof passed:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_'; } > test_after.log 2>&1`.

Additional whitespace proof passed: `git diff --check`.

Proof log path: `test_after.log`.
