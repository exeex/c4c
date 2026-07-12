# Current Packet

Status: Active
Source Idea Path: ideas/open/707_prepared_stack_destination_authority_positive_gate.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Establish one real stack-backed publication/move producer

## Just Finished

- Completed repaired Plan Step 2.1 without defining or composing a destination
  authority row. The real out-of-SSA/select producer now emits a unique
  `PreparedEdgePublication` for `%cmp` -> `%selected` on exact `pred` -> `join`
  identity and binds its actual `PreparedMoveResolution`.
- Added a prepared relationship validator that retains distinct owner-provided
  source/destination identities and homes, exact edge, publication pointer, and
  move pointer, then independently matches the stack destination home against its
  `PreparedFrameSlot` and `PreparedStackObject` IDs and geometry.
- The real-production fixture keeps `%cmp` in its producer-owned register home
  and gives distinct `%selected` a genuine I32 stack destination/frame object;
  its bound move uses `StackSlot` storage and the same destination offset. Missing
  publication, missing move, and mismatched stack-object evidence reject.
- Producer-side applicability now records branch-stack-load evidence as
  `NotApplicable` because this is an edge-bound out-of-SSA copy rather than a
  terminator operand load, and aggregate-source evidence as `NotApplicable`
  because the producer-owned source home is a register rather than a stack
  source. Stack sources use the same source-home width semantics as
  `PreparedAggregateStackSourceAuthority`; incomplete source layout becomes
  `Unknown`. The focused contract asserts the I32 case, a real-lookup F64
  sibling, and a padded I32 destination all remain `NotApplicable`, proving
  destination geometry cannot manufacture aggregate-source applicability.
- A cloned sibling is passed through the same real edge-publication lookup with
  its original register destination home/move. The relationship validator
  rejects it specifically as `MissingDestinationHome`, proving the positive is
  selected by stack storage semantics rather than fixture name.
  Branch/aggregate evidence is inapplicable to this scalar edge-copy producer.

## Suggested Next

- Execute Plan Step 2.2 against this real relationship. Do not revive the
  rejected branch-load-policy substitution or advance into row composition.

## Watchouts

- This packet establishes only the producer relationship; it intentionally
  does not define `PreparedStackDestinationAuthorityView` or touch MIR.
- The monolithic test retains the pre-existing global-publication failure. Use
  the isolated registered Step 2.1 contract for green acceptance.

## Proof

- Green focused acceptance: `(cmake --build build --target
  backend_prepare_stack_publication_contract_test -j && ctest --test-dir build
  -R '^backend_prepare_stack_publication_contract$' --output-on-failure) 2>&1 |
  tee test_after.log` passed 1/1.
- Matching comparison: `(cmake --build build --target
  backend_prepare_stack_layout_test -j && ctest --test-dir build -R
  '^backend_prepare_stack_layout$' --output-on-failure) 2>&1 | tee -a
  test_after.log` compiled and reproduced the unchanged `test_before.log`
  failure `expected coherent global store source publication authority`.
- Combined proof log: `test_after.log`.
