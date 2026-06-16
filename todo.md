Status: Active
Source Idea Path: ideas/open/287_post_286_prepared_boundary_interface_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Classify Each Interface Smell

# Current Packet

## Just Finished

Completed Step 2 classification of the three post-286 prepared-boundary
interface smells. No implementation files were changed.

Classification:

- Call argument rendered-string suffix use:
  - Class: needs structured carrier before broad removal; possible narrow
    cleanup/quarantine after Step 3.
  - Reason: `strip_call_arg_abi_type_suffix` currently removes `alignstack(...)`
    from rendered argument text before scalar and aggregate layout lowering.
    Structured call arguments and `LirTypeRef` already exist, but the AArch64
    variadic HFA carrier array is still emitted as rendered type text with an
    `alignstack(...)` suffix and no structured metadata owner. Deleting the
    strip helper now would remove compatibility before the replacement fact has
    an owner.
  - Candidate narrow cleanup packet: add an explicit structured call-argument
    ABI metadata carrier for stack alignment or carrier layout, populate it for
    the AArch64 variadic HFA carrier-array path, consume it on structured call
    lowering, and leave rendered suffix stripping only as a named legacy
    compatibility path for hand-built/no-ref LIR.
  - Reject signals: the packet only renames `strip_call_arg_abi_type_suffix`;
    the structured path still depends on parsing `alignstack(...)`; legacy text
    fallback is deleted without an equivalent `LirCallArg`/`LirTypeRef`/ABI
    carrier; byval/carrier struct identity remains inferred only from rendered
    text while the patch claims ownership cleanup.

- AArch64 variadic HFA lane expansion ownership:
  - Class: retained target ABI policy with a structured-helper prerequisite;
    likely separate follow-up idea rather than a safe local cleanup.
  - Reason: `append_aarch64_variadic_hfa_carrier_arg_lanes` joins target
    selection, HFA payload classification, aggregate alias lookup, local slot
    leaf ordering, lane BIR values, and `CallArgAbiInfo` lane metadata. That is
    not just a formatting smell. It is valid target policy, but the current
    inline placement couples semantic call lowering to AArch64-specific lane
    assembly.
  - Candidate narrow cleanup packet: none for this step. The plausible route is
    a follow-up helper-boundary packet where a target ABI helper accepts
    structured aggregate layout/slot facts and returns lane values plus ABI
    records, preserving existing fail-closed checks and 286 behavior.
  - Reject signals: generic call lowering grows more AArch64 case logic; the
    helper boundary still reaches through raw local slot maps and rendered text
    instead of structured inputs; lane-count or slot-type mismatch behavior is
    weakened; proof relies only on the 00204 fixture while nearby HFA vararg
    cases are unexamined.

- Opaque pointer byte-offset provenance:
  - Class: separate follow-up idea; retain current prepared/semantic
    fail-closed policy until a structured base-object extent or byte-range
    carrier exists.
  - Reason: `allow_opaque_ptr_base && stored_type == I8` is a coarse
    compatibility admission rule for byte-wise scalar access through opaque
    bases. Prepared `memory_accesses` preserve byte offset, size, alignment,
    and addressing policy, but no prepared or semantic carrier currently
    provides a structured base-object extent/range or `LirTypeRef` provenance
    back to the admission helper. Narrowing this without that carrier risks
    rejecting valid byte access or broadening invalid subobject access.
  - Candidate narrow cleanup packet: none inside the current runbook unless a
    later step first defines the extent/range carrier. The needed work should
    be a follow-up idea covering provenance address state, prepared lookup
    publication, and fail-closed byte-range validation.
  - Reject signals: replacing the opaque rule with another rendered type-text
    check; broadening all `i8` offset accesses without object extent bounds;
    migrating prepared memory-access policy into BIR without a route agreement;
    deleting prepared lookup/publication surfaces because the current 286
    subset happens to pass.

## Suggested Next

Proceed to Step 3. Decide the exact call-argument suffix ownership boundary:
which structured carrier owns `alignstack`, byval/carrier shape, and struct
identity, and whether the candidate narrow cleanup should quarantine
`strip_call_arg_abi_type_suffix` to legacy/no-ref compatibility only.

## Watchouts

No surface is a pure safe local cleanup today. The call-argument path is the
only candidate for a narrow code packet, and even that should be framed as
structured-owner quarantine rather than deletion of rendered compatibility. HFA
lane expansion and opaque pointer byte-offset provenance should remain
follow-up candidates unless later steps define structured helper/carrier
contracts.

## Proof

Not run (classification-only). No `test_after.log` was created because this
packet made only documentation/state updates in `todo.md`.
