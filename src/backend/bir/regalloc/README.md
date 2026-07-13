# Shared BIR Register Allocation

Status: scaffold (unimplemented).

## Owns

One shared RV64, AArch64, and x86 flow for liveness, interference, allocation,
eviction, spill-slot management, and coordination of explicit spill/reload
insertion. Target differences enter as verified layout and constraint data,
not as separate allocators.

## Does not own

Target-specific allocator implementations, concrete target register names,
machine instruction selection, frame-offset encoding, or parsing inline
assembly instruction text.

## Input

Verified pseudo BIR, a profile-keyed pseudo-register layout, and typed operand,
result, clobber, and grouping constraints.

## Output

Complete pseudo-home assignments plus explicit abstract spill-slot and
`Spill`/`Reload` facts suitable for `AllocatedBir` verification.

## Verification and publication gate

Allocation publishes only when all live ranges obey class, alias, reserved,
ABI, tie, clobber, group-width, alignment, and capacity rules and all evicted
values have consistent explicit spill state.
