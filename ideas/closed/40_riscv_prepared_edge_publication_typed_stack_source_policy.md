# RISC-V Prepared Edge Publication Typed Stack Source Policy

## Goal

Define and implement the next typed scalar `StackSlot -> Register` prepared
edge-publication policy for RISC-V, covering sub-word integer, unsigned I32, or
F32 stack-source forms only after the needed type and register-bank authority is
available.

## Why This Exists

Idea 31 closed the concrete large-offset stack-source address policy for 4-byte
and 8-byte integer-like loads, but `PreparedValueHome` still records stack
offset and size without signedness, scalar type, or destination register-bank
authority. Sub-word extension, unsigned 32-bit behavior, and F32 loads need
that authority before RISC-V can select `lb`/`lbu`/`lh`/`lhu`/`lw`/`lwu` or
floating load opcodes safely.

## In Scope

- Audit the prepared publication and value-home data available to RISC-V for
  scalar stack-source loads.
- Choose one typed scalar form to support first: signed sub-word, unsigned
  sub-word, unsigned I32, or F32.
- Add only the target-local RISC-V load, extension, register-bank, and
  fail-closed policy required for the selected form.
- Preserve shared prepared `edge_publications` lookup as the only semantic
  authority for edge moves.
- Add focused positive and negative tests proving the selected form depends on
  prepared publication facts and rejects unsupported neighboring typed forms.

## Out Of Scope

- Dynamic-address stack-source support.
- Aggregate-width stack-source support.
- Source-to-`StackSlot` destination broadening.
- `PointerBasePlusOffset -> Register` broadening.
- Moving RISC-V typed load or extension policy into shared prepare, BIR, or
  target-neutral helpers without a separate source idea for that authority.
- Broad frame-layout, allocator, or memory-layout rewrites.

## Acceptance Criteria

- One typed scalar `StackSlot -> Register` form is implemented through shared
  `edge_publications`, or the route records the concrete authority gap that
  keeps the selected form fail-closed.
- Existing 4-byte `lw`, 8-byte `ld`, and large-offset address-materialized
  stack-source behavior remains supported.
- Unsupported signedness, type, width, and register-bank combinations remain
  explicit and fail closed.
- Tests fail if shared publication facts are missing or ignored.
- Validation covers focused RISC-V prepared edge-publication tests, relevant
  shared prepared lookup tests, and an appropriate backend bucket.

## Reviewer Reject Signals

- The patch infers signedness, floatingness, or register bank from fixture
  names, value ids, stack slot ids, offsets, or test names.
- The patch treats size alone as enough authority for sub-word extension,
  unsigned I32, or F32 behavior.
- The patch scans predecessor or successor blocks or creates a RISC-V-local
  edge-copy fact table instead of consuming shared `edge_publications`.
- The patch weakens existing 4-byte, 8-byte, or large-offset stack-source
  support to claim typed-scalar progress.
- The patch claims typed support through expectation rewrites, helper renames,
  or classification-only changes while retaining the same fail-closed behavior.
- The patch broadens dynamic-address, aggregate, pointer-base, or
  source-to-stack behavior outside this idea.

## Closure Evidence

Closed as a documented fail-closed prepared-authority blocker.

Inventory found that RISC-V prepared edge publication has enough concrete
authority for existing 4-byte `lw`, 8-byte `ld`, and large-offset stack-source
forms, but does not yet have shared prepared signedness/extension or
destination register-bank/view authority for sub-word, unsigned-I32-shaped, or
F32 stack-source typed loads.

The route preserved existing concrete behavior and made the unsupported typed
forms explicit fail-closed cases instead of inferring from size, ids, offsets,
fixture names, or raw register spelling. Focused tests cover sub-word,
unsigned-I32-shaped, and F32-shaped unsupported forms, and the RISC-V emitter
does not add `lb`, `lbu`, `lh`, `lhu`, `lwu`, or `flw` for those forms.

Validation evidence:
- Focused RISC-V prepared edge-publication and shared prepared lookup proof
  passed 2/2 after the fail-closed policy packet.
- Backend bucket proof passed 163/163 during Step 4 validation.
- Regression guard read-only close check passed against the accepted backend
  log with 163 passed, 0 failed, and no new failing tests.
