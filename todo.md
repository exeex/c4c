# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the exhaustive coverage ledger and schema/import boundary

## Just Finished

- Plan Step 1 completed: reconciled the checked implementation ledger against
  the exact 38 `LirInst` alternatives, six `LirTerminator` alternatives, 18
  metadata families, current Raw storage/builders/views/verifier, the sole
  build-included top-level importer, build wiring, and legacy evidence.
- Corrected stale lifecycle/source-gap/implementation claims and recorded the
  dependency order with one unambiguous first C++ packet.

## Suggested Next

- Execute Step 2A only: add closed typed Raw receipt for current structured
  `TypeSpec` and all `LirTypeKind` alternatives, one shared importer conversion
  for signatures and inline-asm bindings, matching builder/view/verifier rules,
  and neighboring positive/negative tests. Reject semantic `RawText`, missing
  widths, invalid struct identities, and conflicting named definitions; do not
  add globals, objects, constants, new opcodes, or new terminators.

## Watchouts

- `src/backend/bir/lir_to_bir.cpp` is the only production importer;
  `src/backend/bir/lir_to_bir/*.cpp` remains build-excluded legacy evidence and
  cannot be used to claim or implement coverage implicitly.
- Checked-in `CondJumpTerm` schema/view/verifier support is not importer
  coverage; conditional LIR branches still fail in the production importer.
- Step 2A must preserve module-transactional publication and must not widen
  into module name tables (Step 2B) or foundational constants/forward-use
  reservation (Step 2C).
- Do not resume current Child C after 734. Once 734 closes, reactivate 732 and
  rerun documentation convergence from phase A, then B, then C and onward.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- All four matching backend tests passed; the proof was sufficient for this
  documentation/ledger packet. Canonical log: `test_after.log`.
