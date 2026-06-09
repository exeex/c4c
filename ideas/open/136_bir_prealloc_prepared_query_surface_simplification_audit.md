# 136 BIR/prealloc prepared query surface simplification audit

## Goal

Audit the BIR/prealloc prepared-query and contract surfaces after the AArch64
codegen cleanup series, especially ideas 130-135, and produce focused follow-up
ideas for simplification.

This idea is analysis-only. It should not directly rewrite prealloc, BIR, or
AArch64 codegen. Its job is to decide whether the shared layer has become a
large facade for AArch64-specific cleanup work, and where the next small
simplification slices should be.

## Why This Exists

Recent AArch64 cleanup moved several target-neutral relationships forward into
shared prepared/prealloc query surfaces:

- prepared value-home lookup
- same-block materialization relationships
- select-chain dependency relationships
- current-block entry publication lookup
- call-boundary and publication authority facts from earlier ideas

That was the right direction for avoiding duplicated AArch64 rediscovery, but
it may have shifted complexity into shared code. The current line-count shape
is now a warning sign:

- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/out_of_ssa.cpp`

The reference ARM backend is much simpler at the backend/codegen boundary. We
should not copy its layout mechanically, but we should check whether our shared
query layer is expressing semantic prepared facts or merely preserving old
AArch64 helper shapes under new names.

## Required Analysis

Classify the current BIR/prealloc prepared-query surface by domain:

- value-home and prepared storage lookup
- current-block entry publication
- edge-publication and move-bundle facts
- same-block producer and materialization facts
- select-chain dependency facts
- call-plan argument/result/outgoing-stack facts
- memory-access and pointer/value-home facts
- preservation and runtime-helper facts

For each domain, answer:

1. What files currently own the data and the public query API?
2. Which AArch64 consumers call the query?
3. Is the query target-neutral by semantics, or does it mirror an AArch64
   helper shape?
4. Is the query duplicated under nearby names or split too narrowly?
5. Does the API belong in `prepared_lookups.*`, or should it move to a more
   natural domain owner such as call plans, publication plans, memory/access
   plans, control-flow facts, or a smaller prepared-query module?
6. Would simplifying it help future x86/riscv work, or would it only reshuffle
   AArch64 code?

The audit must explicitly compare against the closures of ideas 130-135 and
record which new shared queries are:

- good shared semantic facts;
- good facts but in the wrong owner file;
- too narrow or duplicated;
- suspicious AArch64-shaped facade;
- no longer needed after later cleanup.

## Expected Output

The closure note must contain:

- a table of the major prepared-query functions or groups inspected;
- a domain ownership map for `prepared_lookups.*`, `call_plans.cpp`,
  `publication_plans.cpp`, `control_flow.hpp`, and adjacent prealloc files;
- a consumer map showing which AArch64 files still depend on these queries;
- explicit no-new-idea notes for query groups that are already clean;
- follow-up ideas for concrete simplification slices, each with bounded files,
  proof route, and reject signals;
- a recommendation on whether `prepared_lookups.*` should stay as one large
  facade, split by domain, or shrink by moving queries to existing owners.

If the analysis finds that the shared layer is already coherent, close with no
follow-up ideas and explain why the extra size is justified by cross-arch
contracts.

## Out Of Scope

- Direct implementation changes in this analysis idea.
- Mechanical line-count reduction without an ownership argument.
- Moving AArch64 register spelling, hazard policy, scratch choices, or final
  instruction emission into shared code.
- Reopening closed AArch64 dispatch/calls/memory/wide-value fixes unless a
  current shared-query smell proves they left duplicated policy behind.
- Redesigning x86 or riscv backends before the shared contract shape is clear.

## Reviewer Reject Signals

- The route treats every large prealloc file as bad without classifying owner
  semantics.
- It creates vague ideas such as "simplify prealloc" or "clean prepared
  lookups" without naming concrete files and query groups.
- It proposes deleting shared queries while AArch64 consumers would return to
  local BIR scans, name matching, predecessor rescans, or testcase-shaped
  logic.
- It moves target-local AArch64 emission details into BIR/prealloc.
- It ignores the closures of ideas 130-135 and duplicates already-settled work.
- It produces follow-up ideas that would block future x86/riscv reuse instead
  of clarifying it.

## Suggested Proof Route For Follow-Ups

This idea itself is analysis-only. Follow-up implementation ideas should choose
their proof by touched owner:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`
- a broader suite only when shared semantics change enough to affect frontend
  lowering or full backend behavior
