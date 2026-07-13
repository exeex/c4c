# LIR-to-BIR Memory Migration

Status: scaffold (unimplemented migration placeholder).

## Owns

The future migration boundary for LIR memory addressing, local slots,
provenance, intrinsics, and value materialization into the active BIR model.
The existing source files in this directory are quarantined reference material
and are excluded from the active build.

## Does not own

Legacy resurrection, implicit inclusion of these sources in CMake, active
memory lowering today, register allocation, or MIR lowering.

## Input

Eventually, structured LIR memory operations and the explicit type,
provenance, and layout facts required by an accepted migration contract.

## Output

Eventually, ordinary Raw BIR nodes, values, and explicit memory semantics using
the active builder and stable-ID model.

## Verification and publication gate

This placeholder publishes no implementation. Each migrated family must first
define its accepted contract, enter the active build deliberately, and pass
Raw BIR verification and focused interface tests; legacy file presence alone
is not evidence of support.
