# Current Packet

Status: Active
Source Idea Path: ideas/open/717_prepared_mir_join_source_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Prove authority preservation across both adapters

## Just Finished

- Plan Step 2.2 now copies exact typed destination/source values and prepared
  identities, producer kind/index and typed producer pointer, publication/move
  identities, home/storage classification, and selected freshness authority
  into each BIR join-source fact.
- `Available` now requires internally agreeing base, producer, and freshness
  authority; incomplete prepared rows remain non-available without
  dereferencing owner-bound evidence after the prepared core is destroyed.

## Suggested Next

- Execute Plan Step 2.3: expand public-boundary negative and aggregate checks
  across both the prepared-MIR view and BIR adapter.

## Watchouts

- Publication, move, bundle, and producer pointers in BIR facts are opaque
  identity tokens after the prepared core lifetime; compare them only and do
  not dereference them.

## Proof

- Passed `CMAKE_BUILD_PARALLEL_LEVEL=1 cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R
  '^backend_prepared_lookup_helper$'`.
- The supervisor-selected focused proof is sufficient for Step 2.2; complete
  combined output is preserved in `test_after.log`.
