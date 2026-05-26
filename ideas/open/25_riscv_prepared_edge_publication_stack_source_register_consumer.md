# RISC-V Prepared Edge Publication Stack Source Register Consumer

## Goal

Add RISC-V prepared edge-publication consumption for `StackSlot -> Register`
register-destination moves through the shared `edge_publications` lookup path.

## Why This Exists

Idea 24 intentionally closed after landing RISC-V register-destination support
for `Register -> Register` and `RematerializableImmediate -> Register`.
`StackSlot -> Register` remained fail-closed because it needs a target-local
RISC-V stack-slot load/address emission policy rather than the simple register
or immediate source rendering used by the first consumer route.

## In Scope

- Define the RISC-V-local stack-slot source rendering needed for prepared
  edge-publication moves into registers.
- Consume shared prepared `edge_publications` as the only semantic authority
  for the edge move.
- Keep register spelling, load instruction choice, scratch policy, and final
  assembly formatting inside the RISC-V backend.
- Add focused positive coverage for `StackSlot -> Register` and negative
  coverage proving missing shared publication authority still fails closed.
- Preserve existing `Register -> Register` and
  `RematerializableImmediate -> Register` behavior.

## Out Of Scope

- Pointer-base source homes.
- Source-to-`StackSlot` destination moves.
- A RISC-V-local predecessor/successor scan or edge-copy fact table.
- Moving RISC-V stack-slot addressing or load policy into shared prepare, BIR,
  or target-neutral helpers.
- Broad RISC-V codegen rewrites unrelated to prepared edge-publication stack
  source loads.

## Acceptance Criteria

- RISC-V consumes shared `edge_publications` for `StackSlot -> Register`
  prepared edge moves.
- Focused tests fail if shared publication facts are absent or ignored.
- Unsupported pointer-base sources and stack-slot destinations remain explicit
  and fail closed.
- Validation covers the focused RISC-V tests, relevant shared prepared lookup
  tests, and an appropriate backend bucket.
- Final handoff states the covered stack-source behavior and any remaining
  RISC-V edge-publication gaps.

## Reviewer Reject Signals

- The patch matches fixture names, edge labels, value ids, or testcase shapes
  instead of implementing a semantic `StackSlot -> Register` rule.
- The patch scans predecessor or successor blocks, creates RISC-V-local
  edge-copy facts, or otherwise rediscover publications instead of reading
  shared `edge_publications`.
- The patch weakens or marks supported register-destination paths unsupported
  to claim progress.
- The patch moves RISC-V stack-slot addressing, load instruction choice, scratch
  policy, or assembly formatting into shared prepare, BIR, or target-neutral
  helpers.
- The patch claims pointer-base source or stack-slot destination support
  without implementing and proving those distinct homes.
- The patch is only a helper rename, expectation rewrite, or classification
  change while leaving `StackSlot -> Register` unsupported.

