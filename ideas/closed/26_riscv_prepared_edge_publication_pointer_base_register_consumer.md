# RISC-V Prepared Edge Publication Pointer Base Register Consumer

## Goal

Add RISC-V prepared edge-publication consumption for
`PointerBasePlusOffset -> Register` register-destination moves once the needed
RISC-V address materialization policy is explicit.

## Why This Exists

Idea 24 kept pointer-base source homes fail-closed. Pointer-base support needs
a real semantic lowering rule for RISC-V address materialization; it should not
be folded into the simpler register/immediate consumer path or implemented as a
fixture-shaped shortcut.

## In Scope

- Define how RISC-V materializes `PointerBasePlusOffset` source homes for
  prepared edge-publication moves into registers.
- Consume shared prepared `edge_publications` as the only semantic authority
  for the edge move.
- Keep RISC-V address materialization, scratch policy, instruction selection,
  and assembly formatting target-local.
- Add focused tests that prove shared publication lookup drives the pointer
  source move and that missing authority fails closed.
- Preserve existing register-source, immediate-source, and stack-source route
  boundaries.

## Out Of Scope

- Stack-slot destination moves.
- Local rediscovery of edge-copy facts from RISC-V block shape.
- Moving pointer-base address materialization into shared prepare, BIR, or
  target-neutral helpers.
- Broad pointer-analysis, memory-layout, or full RISC-V codegen rewrites.

## Acceptance Criteria

- RISC-V consumes shared `edge_publications` for
  `PointerBasePlusOffset -> Register` prepared edge moves.
- Tests fail if shared publication facts are missing or ignored.
- Unsupported destination homes remain explicit and fail closed.
- The implementation uses a semantic target-local address materialization rule,
  not testcase-shaped matching.
- Validation covers focused RISC-V pointer-base publication tests, relevant
  shared prepared lookup tests, and an appropriate backend bucket.

## Reviewer Reject Signals

- The patch matches named tests, edge labels, fixture offsets, or value ids
  instead of implementing a general pointer-base source rule.
- The patch scans predecessor or successor blocks or builds a RISC-V-local
  publication table instead of consuming shared `edge_publications`.
- The patch moves RISC-V address materialization or scratch policy into shared
  prepare, BIR, or target-neutral helpers.
- The patch claims stack-slot destination support or broad RISC-V publication
  support without proving those separate surfaces.
- The patch downgrades expectations or marks a targeted pointer-base path
  unsupported to claim progress.
- The patch is only a helper rename, expectation rewrite, or classification
  change while retaining the same pointer-base fail-closed behavior.

## Closure Note

Closed with RISC-V support for `PointerBasePlusOffset -> Register`
register-destination prepared edge-publication moves when the pointer base
resolves through shared prepared value-home lookup authority to a register home
and the byte delta is present and fits a signed 12-bit RISC-V `addi`
immediate. The target-local consumer emits `addi <dst>, <base>, <delta>` for
non-zero deltas and zero-delta `mv <dst>, <base>` through shared
`edge_publications` authority.

Validation accepted at closure:

- focused RISC-V prepared edge-publication subset passed 5/5 with matching
  regression guard
- backend bucket evidence in `test_before.log` passed 163/163
- full-suite baseline evidence in `test_baseline.log` passed 3411/3411
- `review/idea26_riscv_pointer_base_edge_publication_review.md` reported no
  blocking findings

Remaining work is intentionally outside this idea. Existing open ideas cover
source-to-`StackSlot` destinations and stack-source policy broadening. A new
pointer-base broadening idea records non-register bases, wider materialization,
and large-delta policy. Missing or unresolved base names and absent deltas
remain fail-closed malformed/insufficient-authority cases unless a later source
model explicitly changes that contract.
