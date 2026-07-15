# Typed-LIR Parallel CFG-Edge Verifier Admission

Status: Open
Type: typed-LIR verifier prerequisite for Raw-BIR PHI receiver coverage
Parent Return: `ideas/open/734_lir_to_new_bir_container_completeness.md`, Step 7.25

## Goal

Admit and verify duplicate typed `LirCondBr` successors and duplicate ordered
`LirSwitch.case_successors` as distinct parallel CFG-edge occurrences, then
provide a precise typed-LIR handoff that lets the Raw-BIR receiver preserve
their exact multiplicity and order.

## Why This Exists

The active Raw-BIR PHI receiver requires one incoming per exact predecessor to
PHI-block edge occurrence. Its required parallel-edge inputs cannot reach that
receiver because `src/codegen/lir/verify.cpp` currently rejects duplicate
conditional-branch successors and duplicate switch case successors. This is a
typed-LIR verification/admission boundary outside parent 734's no-LIR-change
scope.

## In Scope

- inspect the typed-LIR successor validation for `LirCondBr` and
  `LirSwitch.case_successors`
- revise only the necessary typed-LIR verification/admission contract so equal
  successor IDs can represent distinct, ordered parallel CFG-edge occurrences
- retain current-function ownership, presence, validity, and structural
  successor checks; distinguish occurrence multiplicity from ambiguous or
  malformed authority
- add focused producer/verifier coverage for duplicate conditional successors
  and duplicate switch case successors, including malformed neighbours
- document an exact handoff to 734: each repeated successor is a separate
  ordered edge occurrence available to the Raw-BIR receiver

## Out Of Scope

- Raw-BIR containers, builders, views, verifier, importer dispatch, receiver
  tests, or accepting the current uncommitted Step 7.25 work
- any broader LIR producer or schema redesign beyond this duplicate-successor
  verifier/admission seam
- target lowering, canonicalization, MIR, emission, or source presentation
  recovery
- deriving CFG identity from labels, text, test names, or rendered output
- changing the parent return step, or receiving PHI nodes in this idea

## Acceptance Criteria

- typed-LIR verification admits duplicate `LirCondBr` successor IDs and
  duplicate entries in ordered `LirSwitch.case_successors` as distinct edge
  occurrences, without collapsing their order or multiplicity
- malformed, missing, invalid, foreign-function, or otherwise incoherent
  successor authority still fails closed before downstream use
- focused nearby tests cover both admitted parallel forms and rejection
  neighbours, with a fresh build and supervisor-selected proof
- the accepted handoff states that 734 may bind PHI incoming entries to the
  exact ordered parallel occurrences; it does not authorize Raw-BIR changes
  inside this blocker
- parent 734 can be reactivated unchanged at Step 7.25 after acceptance

## Reviewer Reject Signals

- Reject any Raw-BIR receiver/container/importer change, parent-WIP acceptance,
  or test expectation downgrade claimed as verifier progress.
- Reject a named-test or rendered-text exception instead of general typed
  successor-occurrence verification for both conditional and switch forms.
- Reject collapsing, sorting, deduplicating, or otherwise losing repeated
  successor occurrence order/multiplicity.
- Reject admitting missing, invalid, foreign-function, or ambiguous successor
  authority merely to make a parallel-edge testcase pass.
- Reject broader producer semantics, target lowering, source recovery, or
  unrelated LIR schema changes.
- Reject a handoff that renames the old duplicate failure without permitting
  distinct typed parallel occurrences to reach downstream consumers.

## Required Return Handoff

Report the accepted verifier contract, affected typed fields, focused proof,
and implementation commit to parent 734. Parent reactivation must resume at
unchanged Step 7.25 for complete PHI receiver coverage, including exact
parallel edge occurrences; it must not treat its existing WIP as accepted.

## Closed Disposition

Capability complete. Commit `a889ce33f` establishes focused positive coverage
that duplicate `LirCondBr` successor IDs and duplicate ordered
`LirSwitch.case_successors` remain distinct ordered typed edge occurrences.
The investigation confirmed `src/codegen/lir/verify.cpp` already validates
those successor occurrences independently, so no verifier implementation
change was needed; existing malformed, missing, invalid, foreign-function,
and incoherent-authority rejection remains in force.

Supervisor acceptance evidence is a fresh `cmake --build --preset default`
followed by `ctest --test-dir build -j --output-on-failure -R
'^frontend_lir_call_type_ref$'`, passing 1/1. The canonical focused
before/after comparison was non-regressive under `--allow-non-decreasing-passed`
(1/1 to 1/1).

Return handoff: reactivate parent
`ideas/open/734_lir_to_new_bir_container_completeness.md` unchanged at Step
7.25, `Receive typed PHI incoming authority`, and reattempt its complete PHI
receiver coverage including exact ordered parallel CFG-edge occurrences. This
blocker accepted no Raw-BIR receiver/container/importer work and does not
accept parent 734's existing Raw-BIR WIP.
