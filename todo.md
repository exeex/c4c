# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 4.4
Current Step Title: Receive scalar integer value returns

## Just Finished

- Completed Plan Step 4.3 with typed Raw-BIR `GetElementPtrNode` storage:
  exact `GlobalObjectId` base, fully structured global array `Type`, native
  `inbounds`, ordered ordinary index `ValueId` operands, and one source-backed
  pointer result.
- Added atomic builder insertion/rollback and independent verifier checks for
  global/type/result ownership, nonempty integer indices, `InstResultDef`,
  source index, and instruction/value order. Immediate indices reuse typed
  constants; SSA indices reuse prior current-function source values.
- Admitted only complete authoritative GEPs, with misleading displays, two
  neighboring immediate shapes, a Load-backed SSA index, malformed/raw/mixed
  rejection, late rollback, and public-builder unresolved-index verification.

## Suggested Next

- Implement only Step 4.4's authoritative scalar integer value return row,
  reusing the existing typed function signature and current-function value
  registry while retaining void return receipt.

## Watchouts

- The GEP `LirTypeRef` contributes only native kind plus exact spelling parity.
  The resolved selected global's already-structured object `Type` is the sole
  BIR element/object authority; never reconstruct array facts from spelling.
- Reuse current-function result/value identity for scalar returns; do not add a
  display/name side table or expand into comparison/control receipt.
- Keep noninteger/raw returns, remaining function/CFG/local-object work, and
  other instruction families fail-closed.

## Proof

- `cmake --build --preset default` completed successfully.
- `ctest --test-dir build -R '^backend_lir_to_bir_interface$'
  --output-on-failure` passed 1/1.
- Exact `--dump-bir` boundary observations preserved the intended frontier:
  global Load, global Store, and scalar-return smoke inputs now stop at
  `InvalidVoidReturn`; the array-address input admits its GEP and stops at the
  later `UnsupportedOrdinaryInstruction` comparison/control row.
- `ctest --test-dir build -j --output-on-failure > test_after.log` passed
  3033/3033. The monotonic regression guard against `test_before.log` passed
  with delta `passed=0 failed=0` and no new over-30-second tests.
