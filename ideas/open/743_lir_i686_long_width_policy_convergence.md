# LIR I686 Long-Width Policy Convergence

Status: Open (inactive)
Type: target-width semantic policy convergence
Blocked Consumer: ideas/open/734_lir_to_new_bir_container_completeness.md

## Goal

Make `long` and `unsigned long` width one structured target-profile policy
across LIR signature production and verification plus new-BIR signature,
return, and global receipt, with I686 preserving 32-bit C long and LP64 targets
preserving 64-bit C long.

## Why This Exists

The current target-independent receiver work exposed a contradiction rather
than a missing container. `src/codegen/shared/llvm_helpers.hpp::llvm_ty`
publishes `TB_LONG` and `TB_ULONG` as `i64` uniformly, and the committed plain
parameter relationship verifier therefore requires 64-bit mirrors. Existing
new-BIR `lower_constant_type` instead maps `long` and `unsigned long` to 32 bits
for the I686 target profile, consistent with existing return-signature
coverage.

Idea 734 cannot truthfully receive production I686 long parameters while those
authorities disagree. It must keep `long` and `unsigned long` fail-closed until
this separate inactive initiative converges producer, verifier, and receiver
policy across all affected semantic surfaces.

## In Scope

- inventory the effective `long`/`unsigned long` width decision for each
  supported target profile, including I686 and the supported LP64 profiles
- identify every structured owner and consumer of that decision in shared LLVM
  type production, LIR function signature mirrors, LIR verification, new-BIR
  type lowering, return signatures, function parameters, globals, and their
  focused tests
- establish one target-profile-aware structured width rule used consistently
  by producer, verifier, and receiver paths
- make I686 `long`/`unsigned long` signatures, returns, and globals consistently
  32-bit without weakening LP64 64-bit behavior
- make declaration and definition signatures preserve the same target-correct
  mirrors and logical types
- update reachable malformed-mirror verification to compare against the target
  policy rather than an unconditional width
- add focused cross-target positive and negative proof for signatures, returns,
  and globals, including transactional new-BIR rejection on conflicts
- record the exact idea-734 receiver handoff after the policy is proven

## Out Of Scope

- activating or implementing this idea as part of the current idea-734 packet
- changing I686 C `long` to 64 bits merely to match current rendered LIR output
- receiving idea-734 `long`/`unsigned long` parameter rows before this policy is
  accepted
- parsing `i32`/`i64` strings, target-triple text, rendered LLVM, parameter
  names, or testcase names as semantic authority
- redesigning the whole target-profile or ABI subsystem
- pointer width, `long double`, narrow-integer extension, aggregate ABI,
  variadics, calling-convention placement, canonical BIR, MIR, allocation, or
  emission work
- standalone constant-family expansion beyond the shared type-lowering seam
  required by signatures, returns, and globals
- changing `int`, `long long`, floating, pointer, or aggregate semantics except
  for neighboring proof that they remain unchanged
- expectation downgrades, supported-to-unsupported changes, allowlists, or
  target-specific testcase dispatch

## Acceptance Criteria

- One checked matrix names the authoritative `long`/`unsigned long` width for
  every supported target profile and every affected signature, return, and
  global producer, verifier, and new-BIR consumer.
- I686 publishes and verifies 32-bit `long`/`unsigned long` mirrors and receives
  matching typed BIR signatures, returns, and globals.
- Supported LP64 targets publish, verify, and receive 64-bit
  `long`/`unsigned long` across the same surfaces.
- Declarations and definitions agree exactly; malformed 32/64-bit mirror or
  target-policy conflicts reject through reachable LIR and BIR verification
  without partial publication.
- Existing I686 return-signature coverage remains at least as strong, and
  neighboring `int`, `long long`, floating, and pointer width behavior does not
  regress.
- Idea 734 receives a checked handoff naming when its default-shape
  `long`/`unsigned long` parameter rows may be unblocked.
- Fresh build, focused cross-target proof, relevant backend proof, and the
  supervisor-selected full regression checkpoint pass.

## Reviewer Reject Signals

- Reject making I686 `long` or `unsigned long` 64-bit to preserve the current
  unconditional LIR mirror; that changes target semantics rather than
  converging policy.
- Reject changing only the new-BIR importer, only the LIR verifier, or only the
  renderer while producer, verifier, return, global, and receiver surfaces
  remain contradictory.
- Reject any unconditional long-width helper claimed as convergence when I686
  and LP64 require different results.
- Reject parsing rendered `i32`/`i64`, target-triple spelling, names, printer
  output, or testcase identity to choose width.
- Reject target-name branches or named-case allowlists embedded in signature,
  return, global, or constant receipt instead of using structured target
  profile authority.
- Reject expectation weakening, supported-to-unsupported changes, removal of
  existing I686 return coverage, or malformed-input tests rewritten to accept
  the old conflict.
- Reject helper renames, classification-only documents, or test-only mirror
  rewrites claimed as capability while production and BIR still disagree.
- Reject broad ABI/target refactors, pointer or long-double policy changes,
  aggregate lowering, canonicalization, allocation, MIR, or emission under
  this bounded initiative.
- Reject idea-734 `long`/`unsigned long` receipt or closure evidence based only
  on LP64 tests; I686 signature, return, and global proof is mandatory.
