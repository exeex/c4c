# MIR boundary

Status: scaffold; the allocated-BIR consumer described here is not implemented.

## Owns

- consuming verified, MIR-ready BIR whose body contains only admitted pseudo
  instruction nodes and `InlineAsm` nodes
- lowering pseudo instructions to machine instructions, either one-to-one or
  through explicitly bounded target expansions
- mapping already-assigned pseudo register categories, classes, groups, and
  stack-slot homes to concrete target registers and locations under the
  `TargetProfile` layout and calling-convention mapping derived by BIR

## Does not own

- normal liveness analysis, register allocation, or out-of-SSA conversion
- repairing register pressure or inserting routine spill/reload operations
- changing an allocation because target mapping or lowering failed
- parsing inline-assembly instruction text

## Input

The input is verified allocated/MIR-ready BIR. Every value requiring a
register has a complete pseudo-physical assignment, or its movement is made
explicit by admitted spill/reload nodes. The instruction body contains no
unadmitted semantic or target-machine nodes.

`InlineAsm` uses the same allocated generic operands and results as other BIR
instructions. Its instruction text stays opaque through MIR. Explicit
constraints and clobbers have already been enforced. A register name written
directly into the text is the user's responsibility.

## Output

The output is a target machine-instruction graph with concrete register and
location mappings, ready for assembler substitution, instruction-text parsing,
encoding, and emission.

## Verification gate

MIR publication succeeds only when every admitted pseudo node has a valid
lowering, every pseudo home maps under the already-derived target layout, and
all bounded expansions preserve the verified BIR control-flow and operand
contract. Mapping or lowering failure is a structured boundary failure; it is
not permission to reallocate or silently insert pressure spill/reload work.

The existing target trees are current or legacy implementation evidence. Their
presence does not mean that they satisfy this allocated-BIR boundary, and this
scaffold does not assign a new implementation owner beyond the top-level MIR
boundary.
