Status: Active
Source Idea Path: ideas/open/287_post_286_prepared_boundary_interface_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Decide Opaque Pointer Byte-Offset Provenance Boundary

# Current Packet

## Just Finished

Completed Step 5 opaque pointer byte-offset provenance boundary decision. No
implementation files were changed.

Decision:

- Current owner:
  - `src/backend/bir/lir_to_bir/memory/provenance.cpp` owns the admission
    check in `can_address_scalar_subobject`.
  - Opaque runtime pointer bases reach the helper through `PointerAddress`
    records from pointer parameters, runtime globals, `alloca`/call-derived
    carriers, GEPs, phi merges, and loaded pointer values.
  - Prepared `memory_accesses` publish the lowered result as
    `PreparedAddressBaseKind::PointerValue` plus `byte_offset`, `size_bytes`,
    `align_bytes`, and `can_use_base_plus_offset`; they do not currently carry
    an authoritative base-object extent or proven byte range.

- Boundary decision:
  - Retain the current `allow_opaque_ptr_base && stored_type == I8`
    compatibility rule for now. It is a behavior-preserving bridge for
    byte-addressed opaque pointer paths, not a valid long-term semantic carrier.
  - Do not demote or broaden prepared `memory_accesses` based only on
    `PointerValue`, `byte_offset`, and `can_use_base_plus_offset`. Those facts
    say a backend may form base+offset addressing for the selected access, but
    they do not prove that the offset is within a known source object or that a
    wider scalar access stays inside a valid byte range.
  - The desired owner is a structured provenance/range carrier attached before
    the provenance access check and then preserved into prepared
    `memory_accesses`. `can_address_scalar_subobject` may remain the
    fail-closed compatibility gate until that carrier exists, but it should not
    be treated as the durable ownership boundary.
  - The current compatibility rule should narrow only after every opaque pointer
    route that needs byte-offset access can provide explicit extent/range facts.
    Otherwise the cleanup would turn into a silent capability regression or a
    target-prepared policy guess.

Structured extent/range facts required before replacement:

- Input:
  - Stable base identity: pointer SSA/value name, local slot id, global
    `LinkNameId`, formal/byval/sret identity, or explicit unknown-runtime base.
  - Base-object extent when known: object size in bytes, valid minimum and
    maximum addressable byte offsets, and whether the object is complete or
    intentionally open-ended.
  - Access range: requested byte offset, access size, alignment, address space,
    volatility, and the scalar type being loaded/stored.
  - Layout authority: structured type/layout id or scalar-layout facts when the
    base has a known aggregate/scalar object representation; rendered type text
    may remain a fallback diagnostic, not the primary proof.
  - Dynamic-array facts when present: element count, stride, dynamic index
    value, and the resulting bounded range when the index is constant or
    otherwise proven in range.
- Output:
  - Either a proven byte-range admission that the access is fully within the
    base extent or an explicit unknown/opaque verdict that keeps the current
    compatibility path.
  - A prepared `memory_accesses` record that can distinguish "base+offset is
    syntactically usable" from "base+offset is proven within a known object
    range".
  - Fail-closed diagnostics or route notes for missing base identity, unknown
    extent, negative or overflowing offset, access-size overflow, stale
    prepared rows, and dynamic-index ranges that cannot be proven.

Follow-up idea text:

```text
Title: Add structured opaque pointer byte-range provenance for prepared memory accesses

Intent:
Replace the `allow_opaque_ptr_base && stored_type == I8` compatibility bridge
with a structured pointer-provenance carrier that records base identity,
known object extent, requested byte range, layout authority, and dynamic-array
range facts before publishing prepared `memory_accesses`. Keep existing opaque
byte-addressed behavior until each route can either prove the access range or
fail closed explicitly.

Owned files:
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- pointer-address producers in `src/backend/bir/lir_to_bir/memory/`,
  `src/backend/bir/lir_to_bir/module.cpp`, and relevant call-return pointer
  carriers
- prepared publication in `src/backend/prealloc/addressing.hpp` and lookup /
  publication-plan consumers that distinguish syntactic base+offset use from
  proven in-range provenance
- focused BIR/prepared tests for opaque pointer byte offsets, unknown extents,
  dynamic arrays, stale prepared rows, and fail-closed out-of-range access

Proof:
- build proof
- `backend_lir_to_bir_notes`
- `backend_prepared_lookup_helper_test`
- `backend_riscv_prepared_edge_publication_test`
- focused AArch64/x86 prepared memory-access subsets that consume
  `PreparedAddressBaseKind::PointerValue` and `can_use_base_plus_offset`
- the existing 286 prepared handoff subset if stdarg pointer carriers are
  touched

Reject signals:
- removing the compatibility rule before every opaque pointer route has a
  structured range fact or explicit fail-closed verdict
- treating `can_use_base_plus_offset` as proof of object-range validity
- accepting arbitrary `i8`-based offsets without checking access size,
  overflow, and known base extent when that extent is available
- using rendered type text as the primary range authority when a structured
  layout id or explicit base extent exists
- weakening stale-row, duplicate-row, or cross-block prepared
  `memory_accesses` rejection to keep a narrow testcase green
- adding target-specific acceptance in prepared/prealloc instead of publishing
  target-independent provenance facts from BIR lowering
```

## Suggested Next

Proceed to Step 6. Either choose the smallest safe code cleanup from Steps 3-5
or generate follow-up ideas. Based on the Step 5 decision, opaque pointer
byte-offset provenance is not a safe narrow cleanup unless Step 6 is limited to
creating the follow-up idea above.

## Watchouts

Do not treat prepared `memory_accesses` as the source of provenance truth. They
are downstream publications of the selected access and currently lack base
extent/range authority. The `allow_opaque_ptr_base && stored_type == I8` rule is
retained only as compatibility; removing or widening it without a structured
range carrier would be route drift.

## Proof

Not run (decision-only). No `test_after.log` was created because this packet
made only documentation/state updates in `todo.md`.
