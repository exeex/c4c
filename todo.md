# Current Packet

Status: Complete
Source Idea Path: ideas/open/729_common_current_block_join_query_exposure.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the boundary and hand back idea 709

## Just Finished

- Plan Step 3 audited the common/AArch64 boundary and proved that the two
  AArch64 role wrappers extract only stable owner/block/value identity and call
  the direct attached-owner common queries. No renamed routing cache, copied
  prepared-fact lookup loop, Route 5 authority, or expectation change remains.

## Suggested Next

- Hand lifecycle control back to idea 709 at its remaining named-handoff
  materializer retirement step; keep its address-materialization cleanup out of
  idea 729.

## Watchouts

- The broader backend suite retains 56 known baseline failures. The failure set
  and pass count exactly match `test_before.log`; these remain baseline debt and
  are not regressions from idea 729.

## Proof

- Monotonic against baseline: `cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R '^backend_'` ran 400 tests with
  344 passes and the same 56 known failures as `test_before.log` (no new
  failures and no reduced pass count). Combined exact output is preserved in
  `test_after.log`.
