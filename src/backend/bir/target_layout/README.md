# Target Layout

Status: scaffold (unimplemented).

## Owns

Derivation of an immutable, profile-keyed BIR pseudo-register layout from
[`c4c::TargetProfile`](../../../target_profile.hpp). The derived facts describe
ordered abstract categories/classes/slots; caller-saved, callee-saved, and
temporary eligibility sets and their derived capacities; aliases and reserved
slots; group width/alignment/contiguity/legal bases; ABI argument/result
eligibility; and a private concrete-mapping domain.

## Does not own

Register allocation, liveness, spill decisions, concrete register identities
inside BIR, or target machine opcodes. `TargetProfile` selects the contract; it
is not treated as a ready-made table of register counts. Temporary eligibility
is a subset of real slots and never additional capacity.

## Input

A validated `c4c::TargetProfile`, including architecture, backend ABI,
relocation model, and applicable floating-point ABI/capability flags.

## Output

A target-context/version-keyed abstract-register-layout fact consumed as data
by preparation, allocation, MIR mapping, and boundary verification. The
initial family covers RV64 LP64 variants, AArch64 AAPCS64, x86-64 SysV, and
i686 SysV; each supported profile must enumerate its exact slots and therefore
exact caller/callee/temp capacities rather than use architectural guesses.

## Verification and publication gate

Publication must reject internally inconsistent pools, overlapping aliases or
reserved/eligible slots, asymmetric alias sets, duplicate concrete mappings,
impossible group rules, unknown ABI/features, empty required classes, and
ABI-ineligible mappings. No layout is authoritative until its complete target
context key, schema version, capacities, and invariants are verified.
