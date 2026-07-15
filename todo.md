# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.40
Current Step Title: Receive the 827-authorized fixed-direct-call argument-0 DirectScalar body-parameter authority row

## Just Finished

- Step 7.40 complete: Raw BIR now receives only the closed-827
  `structured_args[0]` DirectScalar fixed-direct-call argument-0 authority,
  preserving the typed parameter/owner/index relation through the call node.
  The receiver rejects missing, invalid, duplicate, foreign, owner/index/type,
  ABI/role, and consumer-incoherent rows transactionally.

## Suggested Next

- Return to the source completion gate; do not infer source-idea completion
  from this packet alone.

## Watchouts

- Receipt remains bounded to one structured argument-0 row for a direct,
  non-variadic fixed call; do not admit another parameter row or recover
  authority from presentation fields.
- Preserve the unrelated 821/822 worktree material and do not restore,
  accept, overwrite, or co-commit it on this route.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log` passed (6/6); output is in `test_after.log`.
