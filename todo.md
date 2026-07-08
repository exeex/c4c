Status: Active
Source Idea Path: ideas/open/610_rv64_move_bundle_target_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Move-Bundle Residual Diagnostics

# Current Packet

## Just Finished

Activated `ideas/open/610_rv64_move_bundle_target_materialization.md` into
`plan.md` and initialized this executor-compatible scratchpad.

## Suggested Next

Execute Step 1 from `plan.md`: refresh focused diagnostics for the
`unsupported_move_bundle_target_shape` family, starting with the representative
`src/20020206-2.c` when the current harness exposes it, and classify which rows
are true RV64 consumer gaps with complete prepared authority.

## Watchouts

- Do not infer missing prepared authority from an encodable RV64 move.
- Keep destination fan-in policy out of this plan.
- Do not touch expectations, unsupported markers, allowlists, timeout files,
  runtime accounting, or unrelated implementation surfaces.
- Reject named-case-only fixes for `src/20020206-2.c` or similar cases.

## Proof

Suggested baseline/proof command for code-changing executor packets:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```
