# MIR object boundary

Status: scaffold; the target-neutral post-MIR object boundary described here is
not yet verified as implemented by the existing files in this directory.

## Owns

- the target-neutral object model after machine-instruction verification
- section, symbol, and relocation records
- serialization of that verified object model
- the handoff through which target encoders provide target-specific instruction
  and data encodings

## Does not own

- BIR or MIR liveness, register allocation, spill/reload, or out-of-SSA work
- instruction selection or pseudo-to-machine lowering
- assembler parsing or inline-assembly text interpretation
- linker policy or target linker behavior

## Input

The input is verified machine instructions plus target-produced encodings and
the section, symbol, and relocation facts required to describe the object.

## Output

The output is a serialized target object representation suitable for the
selected downstream linker. Target encoders and linkers remain responsible for
target-specific encodings and linker behavior.

## Verification gate

Serialization may publish only when section ownership, symbol references,
relocation records, offsets, and supplied target encodings are internally
consistent. Failure is reported at this boundary rather than repaired by
changing BIR/MIR allocation or instruction selection.
