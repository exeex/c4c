Status: Active
Source Idea Path: ideas/open/call-boundary-move-classification-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Current Call-Boundary Classification

# Current Packet

## Just Finished

Step 1 inventory completed for the AArch64 call-boundary helpers and the
existing prealloc Prepared call-plan surfaces.

Target-neutral Prepared call classification currently embedded in AArch64:
- `calls_argument_sources.cpp` has direct Prepared matching helpers for
  argument plans and ABI bindings: `find_prepared_argument_plan`,
  `find_prepared_argument_binding`, and `find_prepared_result_binding`.
- `calls_moves.cpp` branches on Prepared move phase, destination kind, storage
  kind, ABI index, op kind, source immediate presence, and call argument/result
  association before constructing AArch64 records.
- `calls_preservation.cpp` has Prepared lookup/classification for prior
  preserved call values, including indexed lookup use plus fallback scans over
  call plans.
- `dispatch_calls.cpp` has small Prepared fallback scans for missing
  frame-slot call argument publication and first stack-preserved values when no
  indexed lookup is available.
- `calls_printing.cpp` converts already-selected Prepared clobber and
  preserved-value facts into AArch64 MIR effects/printing records; the facts
  are target-neutral, while the conversion is target-local.
- `src/backend/prealloc/call_plans.cpp` already owns most call fact
  construction: wrapper classification, indirect callee and memory-return
  plans, ABI binding lookup, clobber sets, preserved values, storage encodings,
  and register/stack/immediate/symbol Prepared facts.
- `src/backend/prealloc/regalloc/call_moves.cpp` already produces the
  Prepared move resolutions for call arguments, call results, and returns.

AArch64 ABI policy and target-local decisions that should stay out of prealloc:
- AAPCS64 register spelling, register view selection, and conversion through
  `make_register_operand_from_prepared_authority`.
- sret memory-return address handling, byval copy/register-lane decisions,
  variadic helper details, incoming/outgoing stack sizing, and stack base
  selection.
- f128 carrier interpretation where it selects AArch64 scalar/vector views or
  instruction forms.
- scratch register selection, frame-slot address materialization, immediate
  materialization/casts, aggregate copy code, and offset encodability.
- preservation home population/republication instructions and their AArch64
  register/memory operands.

Concrete AArch64 emission/printing that should remain target-local:
- `CallBoundaryMoveInstructionRecord`, `CallBoundaryAbiBindingInstructionRecord`,
  and `CallInstructionRecord` construction.
- MIR operand construction for registers, immediates, symbols, and memory.
- Machine effect resource construction, status strings, and printed instruction
  formatting in `calls_printing.cpp`.
- Dynamic stack helper lowering, indirect callee materialization, local scalar
  producer materialization, and final call instruction emission.

Smallest first extraction boundary:
- Add a narrow prealloc query/classification helper over existing
  `PreparedCallPlan`, `PreparedMoveBundle`, `PreparedMoveResolution`, and
  `PreparedAbiBinding` facts. It should identify the call-boundary move role
  and return associated Prepared records/facts, not MIR operands.
- Initial returned facts should cover phase, destination role
  (`CallArgumentAbi`, `CallResultAbi`, `FunctionReturnAbi`, `Value`), storage
  kind, ABI index, matched call argument/result plan, matched ABI binding, and
  simple missing/mismatched authority status.
- The helper can first replace the AArch64-local argument/result binding
  matching and the repeated branch predicates that decide whether a move is a
  call argument, call result, return move, or ordinary value move.
- Preservation lookup can be a follow-on helper if the first surface lands
  cleanly; prefer reusing the existing indexed call-plan lookup APIs and only
  preserving current fallback behavior where lookup tables are absent.

## Suggested Next

Step 2 should add the target-neutral prealloc call-boundary move
classification/query helper and focused helper coverage. Keep AArch64 as the
consumer in a later packet so emitted behavior and diagnostics can be compared
without mixing API introduction and target adaptation.

## Watchouts

Keep AAPCS64 sret, variadic lane handling, register spelling, printed call
records, and final call instruction emission target-local. Do not combine this
with entry-formal publication, edge-copy bookkeeping, or operand decoding
migrations.

Target-local hooks needed when AArch64 consumes the helper:
- register conversion/name/view selection
- memory/immediate/symbol operand construction
- diagnostic kind/message selection
- special f128, byval, sret, indirect-callee, and preservation emission paths

## Proof

Inventory/documentation-only packet. No implementation files were modified, so
no build or ctest proof was required or run.
