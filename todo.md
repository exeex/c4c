Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed Step 4 no-code audit packet for the requested LIR-to-BIR bridge
comments only. No implementation files were edited.

Classification:
- `aggregate.cpp`: retained output/no-id compatibility with aggregate-slot
  owner. `selected_aggregate_type_layout(...)`,
  `lower_byval_aggregate_layout(...)`, `collect_aggregate_params()`, and
  `append_local_aggregate_copy_from_slots(...)` still own rendered aggregate
  slot state, legacy byval params, and aggregate copy planning until
  `LocalAggregateSlots` and byval copy state carry structured type refs or an
  explicit no-id marker.
- `call_abi.cpp`: retained output/no-id compatibility with call ABI owner.
  `lower_signature_aggregate_layout(...)`,
  `parse_function_signature_params(...)`,
  `lower_return_info_from_function(...)`,
  `lower_function_params_with_layouts(...)`, and
  `lower_function_params_fallback(...)` still own hand-built legacy LIR and
  non-enforced target signature parsing until signature return/param metadata
  is mandatory at this boundary.
- `calling.cpp`: retained output/no-id compatibility with call lowering owner.
  The legacy raw no-ref call-arg byval fallback is fenced to args with no
  type-ref carrier and no structured ABI payload; direct callee raw-name and
  variadic aggregate `va_arg` comments remain owned by LinkNameId and variadic
  aggregate type-ref threading respectively.
- `types.cpp`: retained output/no-id compatibility with central aggregate
  layout resolver owner. The raw `TypeDeclMap` fallback is still the central
  no-id route for callers that do not carry `StructNameId`; metadata-bearing
  refs already fail closed through
  `lookup_backend_aggregate_type_ref_layout_result(...)`.
- `memory/provenance.cpp`: retained output/no-id compatibility with provenance
  owner. Scalar-subobject checks still receive rendered aggregate text from
  `GlobalInfo`, `LocalSlotAddress`, and `PointerAddress`; imported-function
  raw symbol lookups are Step 3 LinkNameId fences, not Step 4 aggregate layout
  removal candidates.
- `memory/intrinsics.cpp`: retained output/no-id compatibility with intrinsic
  memory owner. Local memset/memcpy leaf views still derive from
  `LocalAggregateSlots` and pointer slot state without aggregate
  `LirTypeRef/StructNameId` metadata.
- `cfg.cpp`: retained output/no-id compatibility with CFG owner. Aggregate PHI
  planning still stores rendered `LirPhiOp::type_str` in `PhiLoweringPlan`
  and uses the aggregate selected-layout fence for slot alignment.
- `call_abi.cpp`, `calling.cpp`, and `ir.hpp`: no universal/raw fallback comment
  in the audited scope was already safely removable without first changing an
  owner boundary. The relevant `ir.hpp` comments describe compatibility/output
  payloads and context carriers only; no implementation action belongs there in
  this packet.

## Suggested Next

No clearly removable code packet was identified under the requested Step 4
audit scope. Suggested Next: choose one owner-boundary conversion instead of a
deletion pass, starting with `src/backend/bir/lir_to_bir/cfg.cpp`
`plan_phi_lowering(...)`: thread structured aggregate type identity from
`LirPhiOp` into `PhiLoweringPlan` so aggregate PHI slot alignment can stop
using rendered `type_str`. Focused proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|verify_tests_)'; } > test_after.log 2>&1`.

## Watchouts

- Treat all retained comments above as owner-boundary work, not immediate
  deletion candidates.
- Do not delete central raw-text layout fallbacks before the named callers carry
  structured identity or an explicit no-id legacy marker; that would turn
  hand-built/output compatibility into silent backend loss rather than escape
  hatch deletion.
- `calling.cpp` raw no-ref byval fallback is already restricted by structured
  arg/type-ref and ABI-payload checks; removing it requires first making
  metadata mandatory for legacy hand-built calls.
- `memory/provenance.cpp` imported-function raw symbol lookups are LinkNameId
  compatibility fences from Step 3, not aggregate layout bridges.
- Closed Step 3 evidence is still relevant as a guardrail: do not restore
  mutable `LirTypeRef::str()`, mutable `LirOperand::str()`, or implicit LIR
  string conversions while converting retained Step 4 owners.

## Proof

No build required for this no-code audit packet. `test_after.log` was not
updated.
