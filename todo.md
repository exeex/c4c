Status: Active
Source Idea Path: ideas/open/841_lir_compact_scalar_abi_leaf_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire accepted scalar text escape hatches

# Current Packet

## Just Finished

Completed plan.md Step 3 for idea 841 with the bounded selected `LirBinOp`
consumer migration. The binop verifier now uses `compact_scalar_type` as the
semantic scalar source for floating opcode agreement, integer immediate width
checks, and direct-scalar parameter authority matching when the compact carrier
is present. The printer validates compact-scalar/type-text parity before
rendering selected scalar binops and renders from the compact carrier after
parity is proven.

Kept `LirBinOp.type_str` as compatibility/rendering parity text and preserved
the optional-carrier fallback for unmigrated/manual compatibility construction.
Added focused coverage proving valid integer/floating scalar binops still
verify/render and stale `type_str` text cannot override a mismatched compact
scalar carrier.

## Suggested Next

Execute Step 4 with one bounded retirement candidate: remove only the
`LirBinOp` scalar text classification/comparison use that is now covered by
the compact scalar carrier, leaving `type_str` compatibility text in place
until every named consumer of that field has migrated.

## Watchouts

Do not implement 734 Raw-BIR receiver work in this idea. Do not assume opaque is
scalar, do not parse rendered text as scalar authority, and do not reopen
accepted receiver rows such as `LirAbsOp` selected-global/i32.

The compact scalar carrier remains intentionally optional for compatibility and
auto-derived from `LirBinOp.type_str` through aggregate initialization. Do not
delete `LirBinOp.type_str` yet: it is still the compatibility/rendering parity
field and other binop-adjacent paths may still reference it.

## Proof

Step 3 implementation proof passed:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_'; } > test_after.log 2>&1`.

Additional whitespace proof passed: `git diff --check`.

Proof log path: `test_after.log`.
