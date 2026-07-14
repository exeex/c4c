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
