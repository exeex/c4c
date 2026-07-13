# MIR boundary

Status: scaffold; the allocated-BIR consumer described here is not implemented.

## Owns

- consuming verified, MIR-ready BIR whose body contains only admitted pseudo
  instruction nodes and `InlineAsm` nodes
- mapping every allocated pseudo instruction one-to-one to its already proved
  machine instruction record
- mapping already-assigned pseudo register categories, classes, groups, and
  stack-slot homes to concrete target registers and locations by applying the
  exact E4 `FrameRealizationPlan` and registered target mapping rule

## Does not own

- normal liveness analysis, register allocation, or out-of-SSA conversion
- repairing register pressure or inserting routine spill/reload operations
- changing an allocation because target mapping or lowering failed
- choosing a frame object, base, offset, displacement, stack adjustment, or
  static/dynamic-region interaction
- creating scratch identities or homes, resolving or scheduling
  `ParallelCopy`, or expanding one BIR node into multiple machine records
- parsing inline-assembly instruction text

## Input

The only input is an exact borrowed `MirReadyBirView` minted by E4. It names
the resolved BIR revision, its `CopyResolutionFingerprint`, and every current
target, layout, constraint, liveness, allocation, spill,
`FrameActionFingerprint`, frame realization, and target-realizability key.
Every value requiring a register has a
complete pseudo-physical
assignment, or its movement is made explicit by admitted spill/reload nodes.
The instruction body contains no unadmitted semantic or target-machine nodes,
no `ParallelCopy`, and no `CopyScratch`; every remaining non-`InlineAsm` node,
including each single-move `EdgeCopy`, is directly realizable as one machine
instruction record.

The immutable private frame plan already fixes each spill/reload object,
outgoing-call object and access, static/dynamic frame region, base, exact
offset/displacement, stack size/alignment and adjustment, and registered
mapping-rule ID. Every required record-producing frame action is an explicit
admitted one-record node; prologue, epilogue, adjustment, save/restore, probe,
and every other record-producing action are never implicit or hidden. F1 does
not choose these facts.

`InlineAsm` uses the same allocated generic operands and results as other BIR
instructions. Its instruction text stays opaque through MIR. Explicit
constraints and clobbers have already been enforced. A register name written
directly into the text is the user's responsibility.

## Output

The output is a target machine-instruction graph with concrete register and
already planned location mappings, ready for assembler substitution,
instruction-text parsing, encoding, and emission.

## Verification gate

MIR publication succeeds only when the `MirReadyBirView` revision and all
fingerprints are exact, every admitted pseudo node has its one-to-one mapping,
the exact frame plan is present, and every pseudo home applies under the
already-derived target layout. F1 may choose concrete register spelling,
opcode, and encoding only within the registered mapping rule. Mapping or
lowering failure is a structured boundary failure; it is not permission to
reallocate, create scratch, resolve copies, expand a node, or silently insert
pressure spill/reload work. Repair requires an upstream D4 schema/legalization
change and a new BIR transaction. Machine construction is apply-only and cannot
return repair to D4 or mutate while consuming the view.

The existing target trees are current or legacy implementation evidence. Their
presence does not mean that they satisfy this allocated-BIR boundary, and this
scaffold does not assign a new implementation owner beyond the top-level MIR
boundary.
