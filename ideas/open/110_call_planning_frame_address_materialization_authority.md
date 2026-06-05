# Call Planning Frame Address Materialization Authority

## Goal

Replace the remaining call-planning local frame-address name fallback with
explicit prepared frame-slot address materialization authority for ordinary
supported local aggregate address call arguments.

## Why This Exists

The residue audit found that `src/backend/prealloc/call_plans.cpp` still uses
`find_local_frame_address_source()` when
`find_latest_frame_slot_materialization()` does not return an explicit
`PreparedAddressMaterializationKind::FrameSlot` fact. The fallback scans local
stack objects by value name and legacy `.0` lane spelling, so call argument
selection can still recover frame-address authority from prepared object names.

Current Step 2 evidence shows ordinary direct, register-source, and immediate
computed-address local aggregate argument routes should already be covered by
prepared frame-slot materialization production. The remaining work is to prove
that coverage, close the bounded computed-pointer caveat, and remove or narrow
the fallback without weakening supported local aggregate lowering.

## In Scope

- Audit the three `select_prepared_call_argument_source()` fallback branches
  that call `find_local_frame_address_source()`.
- Prove that ordinary pointer call arguments, same-block derived local frame
  addresses, and immediate-offset pointer-carrier bases receive explicit
  `PreparedAddressMaterializationKind::FrameSlot` facts before call planning
  consumes them.
- Close the computed-address caveat by checking whether any supported
  non-binary pointer-carrier authority can produce a
  `PointerBasePlusOffset` home without a preceding or same-instruction
  prepared frame-slot materialization for the base.
- Replace fallback selection with explicit prepared materialization lookup, or
  narrow the fallback to a documented unsupported/diagnostic compatibility
  path when explicit authority is absent.
- Add focused contract coverage for the supported call argument shapes and
  fail-closed missing-materialization behavior.

## Out Of Scope

- Removing stack-layout `LegacyFrameAddressNameCompatibility` as a bootstrap
  producer for frame-address publication facts.
- Moving frame placement, frame-slot offsets, or target ABI placement into BIR.
- Reworking unrelated call-plan argument selection, sret handling, or
  publication planning.
- Changing expectations to make currently supported aggregate call paths
  unsupported.
- Creating x86 or RISC-V backend-specific patches before the shared prealloc
  authority path is repaired.

## Acceptance And Completion Criteria

- `find_local_frame_address_source()` is no longer used as ordinary call
  argument authority for the three audited fallback branches, or its remaining
  use is explicitly bounded to a fail-closed compatibility diagnostic path.
- Supported local aggregate frame-address call argument cases select from
  explicit prepared frame-slot materialization facts.
- The computed-address pointer-carrier branch is proven either covered by
  existing materialization production or blocked with a named missing shared
  fact instead of silently falling back to object names.
- Tests or prepared-dump contracts cover direct frame-slot, same-block derived,
  and immediate computed-address routes, plus absent/ambiguous materialization
  fail-closed behavior.
- No test expectation is downgraded to claim progress.

## Ownership And Proof Route

Owner: prealloc call planning owns the consumer cleanup in
`src/backend/prealloc/call_plans.cpp`; BIR/prepared materialization producers
own any missing explicit `PreparedAddressMaterializationKind::FrameSlot` fact
discovered during the proof.

Proof route:

- Use AST-backed caller/callee checks for `find_local_frame_address_source()`,
  `select_prepared_call_argument_source()`, and
  `find_latest_frame_slot_materialization()`.
- Inspect producer coverage in stack-layout/address-materialization planning
  before editing consumers.
- Run focused backend/prealloc tests covering local aggregate call argument
  planning and prepared dump visibility for address materializations.
- Run `git diff --check` and the supervisor-selected regression subset before
  accepting the cleanup.

## Reviewer Reject Signals

- The change removes the fallback by downgrading supported aggregate call tests
  to unsupported or weaker contracts.
- The implementation adds named-case matching for one testcase instead of
  selecting from explicit prepared materialization facts.
- The computed-address branch is declared safe without proving the
  non-binary pointer-carrier caveat.
- The cleanup treats stack-layout legacy frame-address seeding as stale even
  though current prepared materialization production still depends on it.
- Tests prove only one narrow call shape while nearby direct, derived, and
  computed local aggregate address routes remain unexamined.
