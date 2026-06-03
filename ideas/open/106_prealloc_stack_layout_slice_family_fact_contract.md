# Prealloc Stack Layout Slice Family Fact Contract

## Goal

Replace or explicitly justify stack-layout slot/slice-family reconstruction
from names and BIR instruction scans with a structured placement-analysis
contract.

## Why This Exists

The memory boundary audit found that stack object creation, frame-slot
assignment, storage plans, and decoded homes are intentional prealloc
placement/storage authority. The ambiguity is narrower: stack-layout hints
such as address publication, alloca coalescing, and copy coalescing can infer
slot families, pointer roots, or slice exposure from BIR instruction shape and
slot naming instead of consuming an explicit BIR or prepared fact.

Those scans may be legitimate placement analysis, but the contract should make
clear whether they are only prealloc home-slot/elision hints or whether BIR is
missing a target-neutral slice-family/publication fact.

## In Scope

- Trace stack-layout hint paths in `src/backend/prealloc/stack_layout/*.cpp`
  that infer slot families, aggregate slice exposure, or pointer roots from
  names or instruction scans.
- Decide which facts are pure prealloc placement hints and which need an
  explicit BIR/prepared family or publication carrier.
- Keep frame-slot IDs, offsets, frame size, alignment, and home-slot decisions
  in prealloc.
- Add focused proof for aggregate slice, address-publication, and coalescing
  behavior affected by the old reconstruction path.

## Out Of Scope

- Moving physical stack layout or frame-slot assignment into BIR.
- Rebuilding alloca/copy coalescing broadly.
- Changing unrelated prepared memory access or pointer-carrier contracts.
- Treating every BIR scan in prealloc as duplication without naming the
  reconstructed fact.

## Acceptance Criteria

- Each slot/slice-family reconstruction route is classified as retained
  placement analysis or replaced with a structured BIR/prepared fact.
- Name-based family inference is removed, narrowed to a documented
  compatibility path, or proven to be non-semantic placement-only analysis.
- Proof covers both an address-publication case and a coalescing/slice-family
  case.

## Reviewer Reject Signals

- The patch claims boundary cleanup while only changing helper names or table
  layout.
- It moves frame offsets, stack object ordering, or home-slot placement into
  BIR.
- It adds named-case slot matching or expectation rewrites instead of a
  structured fact or documented compatibility path.
- Proof covers only one aggregate shape while nearby slice/coalescing routes
  remain unexamined.
- Existing unsupported or weaker test contracts are used as evidence of
  progress without explicit approval.
