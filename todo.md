Status: Active
Source Idea Path: ideas/open/287_post_286_prepared_boundary_interface_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Decide Call Argument ABI Suffix Ownership

# Current Packet

## Just Finished

Completed Step 3 call-argument ABI suffix ownership decision. No
implementation files were changed.

Decision:

- `alignstack` ownership:
  - Preferred structured owner: an adjacent call-argument ABI annotation carried
    from `OwnedLirTypedCallArg` into `LirCallArg`, then consumed by BIR call
    lowering when building `CallArgAbiInfo`.
  - Reason: `alignstack(...)` is not type identity. It is a source-side ABI
    alignment annotation for a concrete call argument, so it should not live in
    `LirTypeRef::str()` or be parsed by semantic BIR from rendered type text.
    Existing `LirCallArg` already carries AArch64 HFA lane metadata, making it
    the smallest compatible carrier for another per-argument ABI fact.
  - Legacy compatibility: `strip_call_arg_abi_type_suffix` may remain only for
    raw/no-ref or positional-mirror calls that lack the structured ABI
    annotation. The structured path should be considered non-owner if it still
    needs to parse `alignstack(...)`.

- Byval/carrier shape ownership:
  - Preferred structured owner: `CallArgAbiInfo` owns lowered ABI layout facts
    such as `size_bytes`, `align_bytes`, register/stack placement,
    `byval_copy`, and AArch64 HFA lane index/count after semantic lowering.
    `LirCallArg` may carry source-side ABI annotations needed to compute those
    facts, but it should not replace `CallArgAbiInfo` as the lowered ABI
    contract.
  - Carrier aggregate shape should be derived from `LirTypeRef` plus structured
    layout tables when present, not from a rendered type suffix. The existing
    rendered fallback can stay as a named legacy bridge for hand-built/no-ref
    LIR only.

- Struct identity ownership:
  - Preferred structured owner: `LirTypeRef::struct_name_id` owns source type
    identity across the LIR/BIR boundary.
  - BIR call lowering should fail closed on missing or mismatched structured
    identity for metadata-bearing aggregate/byval arguments, while retaining
    the existing selected-layout fallback only for explicit legacy/no-ref
    compatibility.

Smallest safe implementation packet:

- Owned files:
  - `src/codegen/lir/call_args.hpp`
  - `src/codegen/lir/ir.hpp`
  - `src/codegen/lir/call_args_ops.hpp`
  - `src/codegen/lir/hir_to_lir/call/args.cpp`
  - `src/backend/bir/lir_to_bir/calling.cpp`
  - focused backend/LIR tests covering the existing `alignstack` HFA carrier
    path
- Change:
  - Add a structured optional per-call-argument ABI metadata field for stack
    alignment, e.g. `stack_align_bytes` or a small `LirCallArgAbiMetadata`
    field, to `OwnedLirTypedCallArg` and `LirCallArg`.
  - Populate it in the AArch64 variadic HFA carrier-array path instead of
    appending ` alignstack(...)` to the carrier type string. Keep the rendered
    `args_str` compatible only if the printer/parser contract still needs the
    suffix for raw text output.
  - In `BirFunctionLowerer::lower_call_inst`, prefer the structured metadata
    when computing aggregate layout and `CallArgAbiInfo::align_bytes`; use
    `strip_call_arg_abi_type_suffix` only in explicitly legacy branches where
    neither `structured_args` nor `arg_type_refs` provide a structured owner.
  - Preserve all current mismatch/fail-closed behavior for missing
    `StructNameId`, mismatched legacy type spelling, lane-count mismatch, and
    local slot shape mismatch.
- Proof:
  - Build proof plus focused subset:
    `backend_lir_to_bir_notes`,
    `backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`, and
    `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.
  - Add or update a focused note/unit test proving the structured carrier path
    no longer requires parsing `alignstack(...)` from `LirTypeRef::str()`, and
    a legacy/no-ref test proving the compatibility strip path still works.
- Reject signals:
  - The patch only renames `strip_call_arg_abi_type_suffix` or moves it without
    introducing a structured owner.
  - Metadata-bearing structured calls still require `alignstack(...)` in
    `LirTypeRef::str()` to pass.
  - `alignstack` is stored on `LirTypeRef` as type identity.
  - `CallArgAbiInfo` placement or byval shape is recomputed from rendered text
    when structured `LirCallArg`/`LirTypeRef` data is present.
  - Legacy rendered compatibility is deleted before raw/no-ref LIR has an
    equivalent structured owner.

## Suggested Next

Proceed to Step 4. Decide the AArch64 variadic HFA lane expansion boundary:
whether semantic BIR should keep assembling lane values and `CallArgAbiInfo`
records inline or whether a target ABI helper should own that expansion behind
a structured input/output contract.

## Watchouts

The Step 3 code packet is safe only as a structured-owner quarantine. Do not
delete rendered compatibility outright, and do not attach `alignstack` to
`LirTypeRef` as if it were type identity. Keep Step 4 separate: HFA lane
expansion has broader target-policy ownership than the suffix carrier cleanup.

## Proof

Not run (decision-only). No `test_after.log` was created because this packet
made only documentation/state updates in `todo.md`.
