# LIR PHI Incoming Successor-Occurrence Identity Publication

Status: Closed — capability complete; parent handoff accepted
Type: LIR producer/schema/verifier blocker for Raw-BIR PHI receipt
Blocked Parent: ideas/open/734_lir_to_new_bir_container_completeness.md, Step 7.25

## Goal

Publish a native, typed per-`LirPhiIncoming` CFG successor-occurrence identifier
for the selected PHI producer families, verify it, and provide a precise handoff
that lets the parent bind each incoming to its exact parallel CFG-edge
occurrence without presentation recovery.

## Why This Exists

`LirPhiIncoming` currently carries a typed incoming value and predecessor
`LirBlockId`, but a duplicate conditional or switch successor occurrence from
the same predecessor cannot be selected from those fields alone. Parent 734's
accepted `006d79aaf` receiver deliberately fails closed at this ambiguity;
choosing an occurrence by incoming order, labels, spelling, printer output, or
LLVM text would fabricate semantic authority.

## In Scope

- Define and publish one native per-incoming successor-occurrence identifier in
  `LirPhiIncoming` for the selected ternary, logical, AArch64-vaarg, and
  AMD64-vaarg PHI producer families.
- Populate it at the producer seam from the same typed CFG successor occurrence
  that reaches the PHI destination; preserve duplicate conditional and ordered
  switch occurrences distinctly.
- Extend LIR verification to reject absent, invalid, foreign-function,
  predecessor-mismatched, destination-mismatched, duplicate, or otherwise
  ambiguous occurrence authority before downstream use.
- Add nearby positive and malformed-authority coverage, then document the exact
  typed handoff to 734, including ownership, ordering, multiplicity, and
  fail-closed guarantees.

## Out of Scope

- Raw-BIR code, importer, container, builder/view, verifier, or backend tests.
- Target lowering, MIR/emission, legacy-BIR, textual/LLVM parsing, or
  presentation-based recovery.
- Other LIR instruction/terminator/value families and broad CFG redesign.
- Completing parent Step 7.25; parent 734 alone consumes this handoff.

## Acceptance Criteria

1. Every selected published `LirPhiIncoming` has a native typed identity for
   its exact predecessor-to-destination successor occurrence, including
   duplicate conditional and switch occurrences.
2. The producer and verifier establish current-function ownership, predecessor
   and destination coherence, uniqueness, order/multiplicity preservation, and
   rejection before printing or downstream use.
3. Tests prove the selected ordinary and parallel producer families plus
   malformed/missing/foreign/mismatched/duplicate authority rejection without
   relying on rendered text or test-name matching.
4. The handoff identifies the carrier field/type, producer populations,
   verifier guarantees, selected coverage, and exact parent return point.
5. Supervisor acceptance includes a fresh build, matching `^backend_` 5/5
   before-and-after non-decreasing guard with `--allow-non-decreasing-passed`,
   and fresh full `ctest` 3037/3037 passing; implementation commits are cited
   in the completed handoff record.

## Execution Boundaries

- Treat predecessor block ID and successor occurrence ID as distinct authority;
  a predecessor alone never selects one of multiple outgoing occurrences.
- Preserve accepted `006d79aaf` as parent-side safe partial progress. This
  blocker supplies only missing LIR authority and does not modify it.
- Keep unsupported producer forms fail-closed rather than broadening the
  carrier contract opportunistically.

## Reviewer Reject Signals

- Reject any Raw-BIR, backend importer/container/verifier, lowering, MIR, or
  later-family diff as scope drift.
- Reject assigning occurrence identity from PHI input order, labels, operand
  spelling, printer output, LLVM text, or a named testcase.
- Reject a field that merely renames predecessor identity, fails to distinguish
  duplicate conditional/switch successors, or leaves the old ambiguity behind
  a new abstraction name.
- Reject expectation downgrades, supported-to-unsupported reclassification,
  rendered-text probes, helper-only refactors, or named-case shortcuts claimed
  as semantic capability progress.
- Reject acceptance without malformed-authority rejection, same-feature
  neighboring coverage, the stated build/guard/full proof, and a concrete
  parent handoff.

## Parent Handoff And Return

After accepted publication, reactivate 734 at unchanged Step 7.25. It must
repair and complete only the parallel-edge Raw-BIR PHI receiver portion by
consuming this occurrence ID together with closed 751 value/predecessor and
closed 786 SpecialToken authority. It must preserve `006d79aaf` and must not
repeat Steps 7.20 through 7.24 or the accepted unambiguous/loop-backedge
partial receiver work.

## Completion And Accepted Parent Handoff

Close accepted: this bounded LIR producer/schema/verifier blocker is capability
complete. Commit `f52ced6ae` publishes
`LirSuccessorOccurrenceId` in the native
`LirPhiIncoming.successor_occurrence` carrier for the selected ternary,
logical, AArch64-vaarg, and AMD64-vaarg direct producer populations. Commit
`10d63ff7a` verifies that each published incoming has present, valid,
current-function predecessor authority; that its selected typed terminator
occurrence coheres with both its predecessor and the PHI destination; and that
the selected occurrences provide unique exact destination-edge coverage with
the terminator's native multiplicity and order.

The verifier interprets the ID only against the typed predecessor terminator:
direct-branch, conditional true/false, switch default, and ordered switch-case
occurrences remain distinct even when their destination blocks are equal. It
rejects missing, invalid, foreign, predecessor-mismatched,
destination-mismatched, duplicate, and incomplete coverage. Typed positive
coverage includes conditional and switch parallel occurrences; malformed cases
cover the same rejected authority boundary without rendered-text or testcase
matching.

Supervisor-accepted proof: focused `frontend_lir_call_type_ref` 1/1; matching
`^backend_` before/after guard 5/5 non-decreasing; and fresh full CTest
3037/3037. The accepted implementation commits are `f52ced6ae` and
`10d63ff7a`.

Parent 734 now resumes only Step 7.25's remaining parallel-edge Raw-BIR PHI
receiver portion. It must consume `LirPhiIncoming.successor_occurrence` with
closed 751's value/predecessor authority and closed 786's SpecialToken
authority, preserve accepted `006d79aaf`, and must not repeat Steps 7.20–7.24
or accepted unambiguous/loop-backedge receiver work.
