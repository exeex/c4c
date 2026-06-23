# RV64 Aggregate Local Subobject And Byval Flow

## Goal

Repair RV64 prepared lowering for aggregate-local subobject access, nested
aggregate field stores/loads, and aggregate copy/byval flows that remain after
local frame-slot address materialization was completed.

## Why This Exists

Idea 312 repaired the core local stack/address materialization path and closed
with concrete residuals from the Step 5 sweep. `src/00019.c`, `src/00046.c`,
and `src/00140.c` still emit and link but fail at runtime because aggregate
subobject and aggregate copy behavior is broader than local address
publication.

## In Scope

- Aggregate-local subobject address/value flow for structs and unions.
- Nested aggregate field offset stores and loads on RV64.
- Aggregate copy and byval/local aggregate transfer when it is the first bad
  runtime mechanism.
- Focused backend coverage for semantic aggregate-local behavior rather than
  c-testsuite filenames.

## Out Of Scope

- Generic local frame-slot address publication already covered by idea 312.
- Function pointer storage, return, or indirect-call policy.
- Vararg or floating aggregate ABI repair beyond classification unless it is
  inseparable from a narrow aggregate-local proof.
- Global aggregate storage and global address materialization.

## Candidate Evidence

- `src/00019.c`: aggregate self-pointer chain; stops after publishing `s.p =
  &s` and before chained aggregate pointer loads.
- `src/00046.c`: nested union/struct field offset stores and loads stop on
  nested subobject store flow.
- `src/00140.c`: aggregate byval copy and local aggregate pointer flow mixed
  with vararg/float aggregate ABI surface.

## Acceptance Criteria

- Focused backend tests cover nested aggregate-local subobject stores/loads and
  aggregate copy/byval transfer where supported.
- Representative candidates emit, link, and run under qemu, or residual ABI
  gaps are reclassified with concrete emitted-code evidence.
- Repairs use aggregate layout, local memory, and RV64 prepared emission rules
  rather than filename, offset, or field-name matching.

## Reviewer Reject Signals

- The route special-cases `00019`, `00046`, `00140`, fixed field offsets, or
  struct/union names instead of repairing aggregate-local lowering.
- Progress is claimed through expectation rewrites, unsupported downgrades, or
  skipped qemu checks.
- A patch only republishes frame-slot base addresses while nested aggregate
  subobject loads/stores still use missing or stale values.
- A byval/ABI residual is hidden by a fake copy, fake epilogue, or generic
  fall-through that does not move the aggregate payload correctly.
- Broad ABI rewrites outside the aggregate-local evidence are mixed into this
  route without focused proof.
