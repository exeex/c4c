# RISC-V Prepared Edge Publication Register Destination Consumer

## Goal

Add the first RISC-V prepared edge-publication consumer for the
register-destination subset proven ready by the x86 handoff from idea 23.

## Why This Exists

Ideas 21, 22, and 23 proved that the shared prepared
`edge_publications` lookup can drive target-specific edge-move emission across
multiple x86 source-home families. x86 now consumes shared publication facts
for `StackSlot -> Register`, `Register -> Register`, and
`RematerializableImmediate -> Register` moves. That is enough to start a
separate RISC-V consumer route scoped to register destinations without first
requiring full x86 home parity.

Shared prepare remains the semantic authority for edge-publication facts and
lookup indexing. RISC-V must consume those shared facts and keep RISC-V-specific
register naming, move spelling, scratch policy, branch/control-flow emission,
and assembly formatting inside the RISC-V backend.

## In Scope

- Implement RISC-V prepared edge-publication consumption for
  register-destination edge moves.
- Start with the source-home families already proven through the x86 shared
  handoff where RISC-V can emit them safely: stack source, register source, and
  rematerializable immediate source to register destination.
- Use the shared prepared lookup authority, not target-local rediscovery, as
  the only semantic source of edge-publication facts.
- Add focused RISC-V tests that fail if shared `edge_publications` are ignored.
- Preserve explicit fail-closed behavior for homes outside the scoped
  register-destination surface.
- Preserve existing x86, AArch64, and shared prepared lookup behavior.

## Out Of Scope

- Stack-slot destination edge-publication moves.
- Pointer-base source home support unless a separate plan proves the required
  RISC-V address materialization and move policy.
- Full RISC-V codegen completion or broad control-flow lowering rewrites.
- Moving RISC-V emission policy into shared prepare, BIR, or target-neutral
  helpers.
- Creating a RISC-V-local edge-copy fact table, predecessor/successor scan, or
  fallback semantic authority.
- Weakening supported-path expectations or marking targeted paths unsupported
  as a substitute for capability work.

## Acceptance Criteria

- RISC-V consumes shared `edge_publications` for at least one
  register-destination source-home family covered by the x86 handoff.
- Focused tests prove RISC-V reads shared publication facts and fail if those
  facts are absent or ignored.
- Unsupported homes remain explicit and fail closed, especially stack-slot
  destinations and pointer-base sources unless they are intentionally brought
  into scope by a later lifecycle decision.
- The implementation keeps target-specific emission policy in the RISC-V
  backend and does not alter shared prepared authority boundaries.
- Validation covers the new RISC-V consumer tests, relevant shared prepared
  lookup coverage, and an appropriate backend bucket.
- The final handoff states which RISC-V source/destination homes are covered
  and which remain unsupported.

## Reviewer Reject Signals

- A patch matches only a named testcase, fixture label, edge name, or prepared
  value id instead of implementing a semantic RISC-V register-destination
  edge-publication rule.
- A patch scans predecessor or successor blocks, creates RISC-V-local
  edge-copy facts, or otherwise rediscover edge publications instead of reading
  shared `edge_publications`.
- A patch downgrades supported-path expectations, marks a targeted
  register-destination path unsupported, or weakens tests to claim progress.
- A patch claims support for pointer-base sources or stack-slot destinations
  without implementing and proving the necessary RISC-V emission behavior.
- A patch moves RISC-V register spelling, scratch selection, move instruction
  choice, branch handling, or assembly formatting into shared prepare, BIR, or
  target-neutral helpers.
- A patch is only a helper rename, expectation rewrite, or classification-only
  change while retaining the old RISC-V lack of edge-publication consumption.
- A patch broadens into unrelated RISC-V backend rewrites instead of landing an
  auditable prepared edge-publication consumer slice.
