# 244 Phase F1 x86 Route 6 call-wrapper diagnostic/oracle replacement

## Goal

Replace x86 direct-call wrapper semantic source authority for the selected
scalar `i32` call argument path with Route 6 facts, while preserving prepared
`ConsumedPlans`, route-debug, helper-oracle, fallback, wrapper output, expected
strings, and baseline behavior.

## Why This Exists

Phase F0 found that x86 has narrow Route 6 scalar `i32` evidence, but the
call-wrapper path still depends on prepared-shaped diagnostic and oracle
authority. The next safe packet is not broad call-wrapper migration; it is a
route-native diagnostic/oracle replacement for the selected scalar call
argument source identity, behind the current agreement and fallback matrix.

## In Scope

- The x86 scalar `i32` direct-call argument source identity path currently
  covered by Route 6 route-debug and `ConsumedPlans` compatibility evidence.
- Route-native diagnostic/helper-oracle rows for positive, absent, invalid,
  duplicate, mismatch, wrong-call, wrong-argument, missing-source, and prepared
  fallback cases.
- Agreement-gated use of Route 6 source identity before any wrapper behavior
  changes.
- Preservation of x86 route-debug headers, focus rows, dump-equivalence rows,
  helper-oracle statuses, fallback behavior, direct-call wrapper assembly, and
  supported-path contracts.

## Out Of Scope

- Broad x86 call-wrapper migration or route-wide x86 Route 6 readiness.
- riscv call-wrapper readiness or cross-target wrapper convergence.
- Deletion, hiding, privatization, or aggregate replacement of `call_plans`,
  `PreparedFunctionLookups`, `PreparedBirModule`, or draft 155.
- Moving ABI placement, frame layout, register allocation, call-wrapper
  policy, helper/carrier protocol, outgoing stack layout, formatting,
  instruction selection, emission policy, or wrapper output into BIR ownership.
- Expected-string rewrites, helper renames, supported-path downgrades,
  unsupported relabeling, timeout masking, or baseline refreshes as proof.

## Acceptance Criteria

- Route 6 is the named semantic source authority for the selected scalar `i32`
  call argument only after agreement with the existing prepared call argument
  source selection.
- The current fail-closed matrix remains observable: absent Route 6, invalid
  source id, duplicate Route 6, Route 6/prepared id mismatch, and source-name
  mismatch do not produce a false positive Route 6 wrapper source.
- Prepared fallback continues to produce the same wrapper assembly where it is
  currently accepted.
- x86 route-debug and helper-oracle output remain byte-stable unless this idea
  explicitly adds route-native rows beside the compatibility rows.
- No public prepared aggregate, lookup group, or draft retirement claim is made.

## Proof Requirements

- Targeted x86 Route 6 route-debug coverage for positive, missing, invalid,
  duplicate, mismatch, source-name mismatch, and absent-row behavior.
- Helper-oracle coverage for `available`, `missing_call`, `wrong_call`,
  `missing_argument`, `missing_source_relationship`,
  `missing_source_value`, `abi_bound_excluded`,
  `duplicate_relationship`, and `no_match` where applicable.
- Wrapper output proof that `expected_minimal_direct_extern_call_lane_asm()`
  and prepared fallback assembly remain unchanged.
- Prepared lookup helper proof that `ConsumedPlans` compatibility remains
  public and stable.
- A matching before/after regression guard over the x86 route-debug,
  direct-call wrapper, and prepared lookup helper tests.

## Reviewer Reject Signals

- The change claims broad x86 call-wrapper migration, riscv readiness,
  cross-target wrapper convergence, call-plan group deletion, or aggregate
  prepared retirement.
- The implementation uses named fixture matching, string shortcuts, or a
  one-test special case instead of consuming a Route 6 semantic source fact.
- Existing route-debug rows, helper-oracle statuses, wrapper assembly,
  fallback behavior, supported-path status, or baselines are weakened or
  rewritten as the main evidence.
- ABI, frame, register, stack, helper/carrier, formatting, instruction
  selection, emission, or wrapper policy is moved into BIR.
- Prepared facts are silently read as route-native truth without an explicit
  agreement or compatibility-fallback label.
