Status: Active
Source Idea Path: ideas/open/190_lir_call_argument_structured_payload_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Existing Call Argument Authority

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory for current LIR call argument authority.

Generated metadata-rich paths:
- Ordinary HIR calls flow through
  `StmtEmitter::resolve_call_target_info` -> `prepare_call_args` ->
  `emit_void_call` / `emit_call_with_result` in
  `src/codegen/lir/hir_to_lir/call/target.cpp` and
  `src/codegen/lir/hir_to_lir/call/args.cpp`.
- `resolve_call_target_info` records `CallTargetInfo::callee_type_suffix` from
  `llvm_fn_type_suffix_str` for known functions or function-pointer signatures,
  and keeps direct-call identity in `callee_link_name_id`.
- `prepare_call_arg` / `prepare_call_args` produce
  `std::vector<OwnedLirTypedCallArg>` with rendered argument type text,
  operand text, and sometimes `LirTypeRef` for aggregate arguments.
- `emit_void_call` and `emit_call_with_result` call
  `make_lir_call_op_with_return_type_ref`, passing `args`,
  `direct_callee_link_name_id`, and `structured_callee_signature`.
- `structured_callee_signature` creates `LirCallSignature` from a known
  `Function` or `FnPtrSig`, including return type ref, fixed parameter text,
  fixed parameter `LirTypeRef`s, variadic, unspecified-params, and void-param
  facts.

Raw/no-metadata compatibility paths:
- Hand-built LIR/test calls can construct `LirCallOp` directly with only
  `callee_type_suffix` and `args_str`.
- Generated intrinsic/builtin helper calls often use `make_lir_call_op` with no
  callee signature or link-name id; these remain text-shaped compatibility
  calls unless a later slice intentionally gives them structured metadata.
- `LirCallOp` currently keeps `callee_type_suffix` and `args_str` as printed
  call spelling, and the printer renders calls through `format_lir_call_site`.

Exact text-authority sites involving `callee_type_suffix` / `args_str`:
- `src/codegen/lir/call_args.hpp` owns the text parser/formatter boundary:
  `format_lir_call_fields`, `parse_lir_call_param_types`,
  `parse_lir_typed_call_args`, `parse_lir_typed_call`,
  `parse_lir_typed_call_or_infer_params`, and
  `parse_lir_direct_global_typed_call`.
- `src/codegen/lir/call_args_ops.hpp` adapts those helpers to `LirCallOp`.
  Even when `callee_signature` exists,
  `parse_lir_typed_call_or_infer_params(const LirCallOp&)` still parses
  `call.args_str` for argument operands and argument type text, then combines
  that with signature parameter facts.
- `src/backend/bir/lir_to_bir/calling.cpp` is the main semantic consumer:
  `BirFunctionLowerer::parse_typed_call` parses `args_str` for metadata-rich
  calls at the `callee_signature` branch, parses `callee_type_suffix` for
  raw variadic/fixed compatibility, and reparses both fields in its fallback
  paths.
- `BirFunctionLowerer::parse_direct_global_typed_call` uses
  `parse_typed_call` when `callee_signature` exists, but otherwise derives
  direct-call parameter count, variadic status, byval pointee type, and
  argument operands from `callee_type_suffix` and `args_str`.
- `BirFunctionLowerer::lower_call_inst` consumes the parsed `ParsedTypedCall`
  / `ParsedDirectGlobalTypedCall` vectors for direct and indirect call lowering;
  argument operands and most argument type text still originate from parsed
  rendered call text there.
- `src/backend/bir/lir_to_bir/module.cpp` also calls
  `BirFunctionLowerer::parse_direct_global_typed_call` while recognizing
  direct `calloc`, so it inherits the same text-authority boundary.

Existing structured facts:
- `LirCallOp::return_type` is a `LirTypeRef`.
- `LirCallOp::direct_callee_link_name_id` preserves direct callee identity.
- `LirCallOp::arg_type_refs` mirrors aggregate argument type fragments when
  `OwnedLirTypedCallArg::type_ref` is present and the rendered text is
  mirrorable.
- `LirCallOp::callee_signature` carries structured return/fixed-parameter
  signature metadata, including variadic and void/unspecified parameter-list
  facts.
- BIR byval aggregate layout already prefers `arg_type_refs` when present via
  `lower_byval_call_arg_layout`; without mirrors it explicitly falls back to
  rendered byval text as legacy compatibility.

## Suggested Next

Execute `plan.md` Step 2 by adding/selecting the structured argument carrier at
the `LirCallOp` boundary. First implementation target should be
`src/codegen/lir/ir.hpp` plus `src/codegen/lir/call_args_ops.hpp`: preserve
`callee_type_suffix` / `args_str` as rendered spelling, but add a generated
structured argument vector populated from `OwnedLirTypedCallArg` in
`make_lir_call_op_with_return_type_ref` so BIR lowering can consume argument
operand, type text, and `LirTypeRef` without reparsing `args_str`.

## Watchouts

- Do not continue the idea 188 closure gate until ideas 190, 191, and 194 are
  closed or the closure-gate source idea is explicitly narrowed.
- Do not treat parser renames or printer-only tests as progress for idea 190.
- Do not let stale rendered `callee_type_suffix` or `args_str` override
  structured metadata when metadata is present.
- `callee_signature` is not enough by itself: it supplies parameter facts, but
  current BIR lowering still parses `args_str` for the actual argument operand
  list and argument type spelling.
- Preserve the explicit raw/no-metadata path for hand-built LIR and intrinsic
  helper calls with no structured carrier.
- Variadic calls need special attention: fixed parameter facts can come from
  `callee_signature`, but variadic tail argument types currently come from
  parsed `args_str`.

## Proof

Inventory-only packet. No build/test command was required, and no proof logs
were created or modified.
