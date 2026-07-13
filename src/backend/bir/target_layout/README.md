# Target Layout

Status: scaffold (unimplemented).

## Owns

Derivation of an immutable, profile-keyed BIR pseudo-register layout from
[`c4c::TargetProfile`](../../../target_profile.hpp). The derived facts describe
caller-saved, callee-saved, and temporary pools; register classes; aliases and
reserved units; group width/alignment/contiguity rules; and ABI eligibility.

## Does not own

Register allocation, liveness, spill decisions, concrete register spellings,
or target machine opcodes. `TargetProfile` selects the contract; it is not
treated as a ready-made table of register counts.

## Input

A validated `c4c::TargetProfile`, including architecture, backend ABI,
relocation model, and applicable floating-point ABI/capability flags.

## Output

A revision- and profile-keyed pseudo-register-layout fact consumed as data by
legalization, constraint handling, allocation, and boundary verification.

## Verification and publication gate

Publication must reject internally inconsistent pools, overlapping aliases or
reserved units, impossible group rules, and ABI-ineligible mappings. No layout
is authoritative until its profile key and invariants are verified.
