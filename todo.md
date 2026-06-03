Status: Active
Source Idea Path: ideas/open/101_bir_prealloc_missing_call_abi_fallback_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Or Constrain Missing ABI Fallbacks

# Current Packet

## Just Finished

Completed Step 3 by naming retained direct-BIR/bootstrap compatibility repairs
and constraining shared placement helpers to consume existing ABI carriers.
`legalize_module` still performs the retained type-only repairs for direct BIR
or bootstrap modules, but those branches now live under stable compatibility
helper names instead of anonymous inference helpers:

- `direct_bir_call_arg_abi_repair`
- `direct_bir_call_result_abi_repair`
- `direct_bir_function_return_abi_repair`

Shared call-argument placement now uses `resolve_call_arg_abi(call, index)` as
an existing-carrier lookup only. The legacy `resolve_call_arg_abi(target, call,
index)` overload remains source-compatible for existing callers, but delegates
to the carrier-only lookup and no longer synthesizes ABI from `arg_types`.
Register index, stack offset, byval lane width, register names, and placement
logic are preserved once `CallInst::arg_abi` exists.

Plan bank display fallback is retained only through named compatibility
surfaces:

- `direct_bir_call_arg_bank_display`
- `direct_bir_call_result_bank_display`

Those helpers keep prepared-plan display usable for direct-BIR plans without ABI
metadata, while ordinary storage and move authority continue to prefer or
require `arg_abi` and `result_abi`.

Function-return move planning now separates ordinary carrier consumption from
the retained scalar repair. `function_return_storage_kind` requires existing
`Function::return_abi`; `append_return_move_resolution` falls back only through
`direct_bir_function_return_move_repair` when the carrier is absent. Correctly
populated ordinary lowered functions therefore route return moves through
`Function::return_abi`, while direct-BIR/bootstrap scalar return fixtures keep a
named repair path.

## Suggested Next

Execute Step 4: add focused ABI carrier proof for ordinary lowered call args,
params, named call results, and function returns, plus narrow assertions for the
retained direct-BIR/bootstrap compatibility paths where needed.

## Watchouts

- Step 4 should prove ordinary lowered code arrives with carriers before
  prealloc planning; do not let the new `direct_bir_*` repair names become the
  proof path for ordinary source-lowered cases.
- `resolve_call_arg_abi(target, call, index)` still exists for source
  compatibility, but it is no longer target-sensitive ABI inference.
- The plan bank display helpers are intentionally display/bootstrap
  compatibility, not ABI classification authority.

## Proof

Delegated proof command:

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: passed. `test_after.log` contains the delegated build output.
