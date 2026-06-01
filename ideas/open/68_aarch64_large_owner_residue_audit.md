# AArch64 Large Owner Residue Audit

## Goal

Audit the largest remaining AArch64 codegen owner files after calls and
dispatch-family cleanup, then create follow-up ideas that separate target-local
emission residue from duplicated BIR/prealloc/shared prepared authority.

## Why This Exists

The AArch64 dispatch-family contraction has closed through idea 67 and the
latest baseline is green. However, `src/backend/mir/aarch64/codegen` is still
much larger than the reference ARM codegen layout. The remaining size is now
concentrated in a few broad owners rather than many small helper files.

Current high-residue files include:

- `calls.cpp`
- `memory.cpp`
- `alu.cpp`
- `machine_printer.cpp`
- `instruction.cpp` / `instruction.hpp`

Some of this code is expected target-local emission: register spelling,
instruction records, machine printing, scratch selection, addressing, and ABI
instruction sequencing. Some may still be duplicated authority that should be
consumed from BIR, prealloc, or shared prepared facts. This idea should classify
that residue before any further implementation cleanup.

## In Scope

- Inventory the largest AArch64 codegen owners by line count and responsibility.
- Compare each owner against the reference ARM codegen layout and shared
  backend/prealloc responsibilities.
- Classify major helper groups as:
  - `target-emission`: keep local as instruction selection, register/scratch,
    addressing, ABI sequencing, or printing.
  - `consume-shared`: existing BIR/prealloc/prepared facts should be used more
    directly.
  - `missing-shared-authority`: a shared fact/query is needed before cleanup.
  - `fold-back-or-split`: code belongs in a more precise AArch64 owner.
  - `needs-more-evidence`: a narrow probe is required before deciding.
- Produce numbered follow-up ideas for coherent cleanup slices.
- Keep follow-up ideas scoped by owner or by one shared-authority family rather
  than one broad "shrink AArch64" task.

## Out Of Scope

- Direct implementation edits in `calls.cpp`, `memory.cpp`, `alu.cpp`,
  `machine_printer.cpp`, or `instruction.*`.
- Reopening dispatch-family contraction unless the audit finds a concrete
  cross-owner dependency.
- Treating line count reduction as success without moving or confirming
  authority ownership.
- Merging large files mechanically without first identifying semantic residue.
- Weakening tests, diagnostics, or supported behavior.

## Acceptance Criteria

- A durable classification table exists for the largest AArch64 codegen owners.
- Each audited owner has a clear next action: keep local, consume existing
  shared authority, add missing shared authority, split/fold target residue, or
  gather more evidence.
- Follow-up `ideas/open/` files are created for the next cleanup slices.
- Follow-up ideas name owned files and explain whether they are mechanical
  target cleanup, shared-authority migration, or evidence probes.
- The final close note explains why dispatch-family cleanup is not being
  reopened as the primary next route.

## Reviewer Reject Signals

- A patch edits implementation while claiming to be audit-only.
- A patch says "move to BIR" without naming the duplicated helper or missing
  shared fact.
- A patch treats machine records or printer spelling as semantic duplication
  without evidence.
- A patch creates vague follow-up ideas that do not name file families.
- A patch proposes one monolithic cleanup that mixes calls, memory, ALU,
  instruction records, and printer changes.
