# LIR PHI Incoming Value And Predecessor Identity

Status: Open
Type: bounded LIR PHI value/CFG carrier repair
Predecessor: `ideas/open/750_lir_cfg_terminator_block_identity_completion.md`

## Goal

Make `LirPhiOp` incoming entries carry structured value authority and
predecessor block identity instead of raw `(value, label)` strings.

## Why This Exists

`LirPhiOp` is where ordinary value identity and CFG predecessor identity meet.
Today its result has a typed shell, but incoming values and labels are raw
strings. That means even after terminators learn block IDs, PHI receivers can
still silently depend on presentation text.

## In Scope

- Replace or augment PHI incoming entries with `LirOperand` value authority and
  current-function `LirBlockId` predecessor authority.
- Populate the structured carrier for existing ternary, logical, and vaarg PHI
  producers.
- Verify incoming value ownership, predecessor existence, and predecessor/edge
  coherence without parsing labels.
- Retain current rendering compatibility.
- Add focused coverage for ternary, logical short-circuit, and vaarg join
  producers plus malformed predecessor/value cases.

## Out Of Scope

- Terminator block-ID publication; idea 750 owns that prerequisite.
- Generic local/object pointer authority, memory intrinsic authority, Raw-BIR,
  target lowering, MIR, or emission.
- Inferring PHI incoming values or predecessors from `%t` names, label strings,
  printed LLVM, or instruction order.

## Acceptance Criteria

- Active PHI producers publish incoming values and predecessor blocks through
  structured carriers.
- The verifier rejects unknown/cross-function values, missing predecessor
  authority, and predecessor/edge mismatches.
- Misleading display text no longer affects PHI identity.
- Full baseline acceptance requires 100% passing tests. If a baseline run is
  below 100%, reject closure and trace `log/*` by time/commit to identify the
  first bad commit before continuing.

## Resumption Record: vaarg PHI-helper input identity blocker

Last accepted progress: no 751 implementation packet, after-proof, or
implementation commit has been accepted. Completed runbook steps: none. The
interrupted step is Step 1, `Publish and verify typed PHI incoming authority`.

The accepted closed-775 handoff is relevant but non-sufficient context:
`LirVaArgOp.result` identifies the later vaarg operation result, not the
native intermediate values consumed by the existing vaarg PHI constructors.
The first-loss inspection found that
`emit_aarch64_vaarg_gp_src_ptr`, `emit_aarch64_vaarg_fp_src_ptr`, and the
AMD64 vaarg join still feed raw helper values such as `reg_addr`, `stack_ptr`,
`aligned_stack_ptr`, `reg_value`, and `stack_value` into PHI inputs without
native IDs. Inventing those incoming IDs at 751's `LirPhiOp` carrier would
require forbidden recovery from display text or an out-of-scope producer
change.

Classification: `separate-blocker`. Open
`ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md` owns only
native current-function result/value identity for every existing vaarg PHI
helper input required by those three constructors. It must not change
`LirPhiOp`, its verifier, predecessor/edge authority, Raw-BIR/importer,
backend/target lowering/MIR/emission, introduce side tables, or treat
`LirVaArgOp.result` as authority for these helper sources.

Exact return point: after 782 accepts native typed helper-input result fields
for every existing vaarg PHI input, reactivate 751 at Step 1 and publish typed
PHI incoming authority for ternary, logical, and vaarg together as one
carrier/verifier slice. Remaining 751 work is unchanged: retain closed 750's
predecessor-block contract, add the PHI value/predecessor carrier and
fail-closed verification, then prove the three producer families. Accepted
751 proof references: none. Accepted 751 implementation commit references:
none.

## Resumption Update: 782 handoff accepted

Closed 782 is capability-complete and this idea resumes at its preserved Step
1, `Publish and verify typed PHI incoming authority`; no 751 implementation
packet was completed while it was blocked. Its accepted producer-only handoff
is `97d305332`, `4830e94a5`, `11a45f173`, and `774b44b5e`: AArch64 GP selects
the `reg_addr` GEP and `stack_ptr` load, AArch64 FP selects those results plus
the `aligned_stack_ptr` ptrmask call (>8 bytes) or `inttoptr` cast (<=8 bytes),
and AMD64 selects final `reg_value` and `stack_value` loads. The handoff has
focused structural/malformed proof and does not itself establish a PHI carrier,
verification, predecessor/edge, CFG, or downstream consumer fact.
