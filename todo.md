Status: Active
Source Idea Path: ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Audit Residual High-Risk Paths

# Current Packet

## Just Finished

Step 2 audited residual high-risk metadata-rich frontend-to-BIR paths for
rendered-string recovery after complete structured misses:

| Surface | Classification | Evidence |
| --- | --- | --- |
| HIR registry | No new blocker | `CompileTimeState::find_template_def(const Node*, rendered_name)` and `has_template_def(const Node*, rendered_name)` still route complete declaration-key misses through structured keys and only use rendered lookup for no-declaration compatibility. Retained string-only `find_template_def(name)` calls observed in `hir_types.cpp` are compatibility/preservation paths already outside generated metadata-rich routes. |
| Constructor/member owner lookup | No new blocker | Direct constructor and method/member helpers retain complete owner-key miss fences. `find_struct_static_member_decl(const HirStructMemberLookupKey&)`, `find_struct_static_member_const_value(const HirStructMemberLookupKey&)`, method lookup helpers, and direct constructor lowering have closed coverage for stale rendered owner/member fallback. Rendered overloads remain documented no-owner/no-metadata compatibility. |
| Generated template paths | Blocker 201 remains resolved | Closed idea 201 covers generated call, replay, deduction, collection, seed, and retry paths. Reaudit found `function_decl_nodes_` supplying structured declarations into `call.cpp`, `deduction.cpp`, `hir_build.cpp`, and `engine.cpp`; complete structured declaration misses fail closed instead of recovering through `find_template_def(name)`/`has_template_def(name)`. |
| Generated member paths | Blocker 202 remains resolved | Closed idea 202 covers the `scalar_control.cpp` generated-member payload. Reaudit found structured owner/member candidates use `HirStructMemberLookupKey`; rendered `find_struct_static_member_decl` and `find_struct_static_member_const_value` are skipped when a structured candidate exists and misses. |
| HIR-to-LIR call/type metadata | No new blocker | Direct call carriers preserve `LinkNameId`; generated signatures and call args carry structured return/parameter/type-ref, byval, sret, variadic, and operand metadata. Retained `signature_text`, `callee_type_suffix`, and `args_str` are final spelling/raw compatibility mirrors and not the metadata-rich authority. |
| LIR call payload | No new blocker | `LirCallOp` still carries `direct_callee_link_name_id`, `callee_signature`, `return_type`, `arg_type_refs`, and `structured_args`. LIR-to-BIR `parse_typed_call` prefers non-empty `structured_args` and structured signatures; legacy rendered parsing is reached only when structured payloads are absent or for raw compatibility validation. |
| BIR lowering/global provenance | No new blocker | BIR direct calls fail closed for metadata-rich direct calls without structured signatures or with unknown `LinkNameId`. Global lowering builds `GlobalTypes`/`FunctionSymbolSet` as raw-import/no-id compatibility tables while emitted BIR globals, calls, loads, stores, and pointer values carry `LinkNameId` when available. `resolve_initializer_symbol_link_name_id` treats present-but-unknown ids as closed misses, and provenance slot keys include `LinkNameId` plus route-local spelling/offset. |

No new semantic rendered-string authority remains after the fresh residual
audit. Blockers 201 and 202 remain resolved by closed evidence, and no new
narrow blocker idea is required for this step.

## Suggested Next

Execute Step 3: run milestone validation for the frontend-to-BIR closure gate.
The supervisor should select fresh focused frontend/HIR/BIR proof, compare
matching before/after logs with the regression guard, and record whether the
accepted full-suite baseline remains valid for closure.

## Watchouts

This audit did not create a new blocker because all observed rendered paths
were classified as display/final spelling, route-local handles, or explicit
no-metadata/raw compatibility. Step 3 should still treat any validation failure
or stale baseline as a closure blocker, not as permission to start backend
restart inside this gate.

## Proof

Audit-only `todo.md` update per supervisor packet. No build or tests were run,
and no root-level logs were created or modified.
