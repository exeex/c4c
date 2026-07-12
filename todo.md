# Current Packet

Status: Active
Source Idea Path: ideas/open/717_prepared_mir_join_source_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Preserve exact authority through the BIR semantic adapter

## Just Finished

- Plan Step 2.1 implemented exact typed destination/source, bundle, move,
  publication, producer, and selected-freshness authority in the prepared-MIR
  direct-edge view, with incomplete or inconsistent authority failing closed.
- Focused named, immediate, and stack boundary assertions now compare the
  public view to its originating publication, move, producer, and freshness
  references.
- The first failing predicate was the named destination-value comparison: the
  test dereferenced a publication pointer after its lambda-local prepared-MIR
  core had been destroyed. Comparing against the origin within the core/view
  lifetime corrected the boundary assertion without weakening the contract.

## Suggested Next

- Execute Plan Step 2.2: preserve the exact Step 2.1 authority through the BIR
  semantic adapter and keep incomplete adapted rows non-available.

## Watchouts

- Raw publication/move/producer pointers in the prepared-MIR view remain tied
  to the lifetime of their owning `PreparedMirCoreView`; consumers may retain
  the copied typed identities but must not dereference those pointers after
  the core is destroyed.

## Proof

- Passed `CMAKE_BUILD_PARALLEL_LEVEL=1 cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R
  '^backend_prepared_lookup_helper$'`.
- The supervisor-selected focused proof is sufficient for Step 2.1; complete
  combined output is preserved in `test_after.log`.
