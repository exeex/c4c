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

## Close Note

Closed: 2026-06-01

The audit route is complete. The classification separated the large AArch64
owner residue into target-local emission that should stay in AArch64 codegen,
existing shared/prepared authority that cleanup routes should consume directly,
one missing shared-authority family for aggregate transport planning, local
helper duplication that can fold back into narrower AArch64 owners, and
evidence probes where the audit found overlap but not enough authority to
start implementation cleanup.

Follow-up ideas created from the classification:

- `ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md`:
  shared-authority migration for call/publication prepared facts.
- `ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md`:
  shared-authority migration for memory address, value-home, storage, and
  stack-source facts.
- `ideas/open/71_aarch64_scalar_control_flow_prepared_authority_cleanup.md`:
  shared-authority migration for scalar/control-flow prepared authority.
- `ideas/open/72_aarch64_special_carrier_prepared_policy_cleanup.md`:
  shared-authority migration for i128/f128 carrier and runtime-helper policy.
- `ideas/open/73_aarch64_variadic_prepared_entry_plan_cleanup.md`:
  shared-authority migration for variadic entry-plan and operand-home facts.
- `ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md`:
  mechanical target cleanup for scalar type, register-view, and compare
  predicate helper duplication.
- `ideas/open/75_shared_aggregate_transport_plan_probe.md`: evidence probe and
  missing shared-authority route for aggregate transport planning.
- `ideas/open/76_aarch64_publication_ordering_evidence_probe.md`: evidence
  probe for publication materialization ordering across dispatch, calls, and
  memory sources.
- `ideas/open/77_aarch64_machine_status_printer_validation_probe.md`: evidence
  probe for machine status derivation and printer validation overlap.

Dispatch-family cleanup is not being reopened as the primary next route. The
audit found adjacent publication-ordering questions, but they are now captured
as evidence probe idea 76 rather than a reason to resume the closed dispatch
contraction route. The next implementation work should choose from the scoped
follow-up ideas above instead of expanding this audit into another broad
dispatch-family pass.

No implementation files changed as part of the audit closure. The audited
AArch64 implementation owners remained untouched, and the active lifecycle
state was closed by moving this source idea to `ideas/closed/` and removing
`plan.md` and `todo.md`.
