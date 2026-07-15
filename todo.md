# Current Packet

Status: Active
Source Idea Path: ideas/open/816_lir_next_residual_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit and select one residual producer row

## Just Finished

Step 1 selected exactly one row: scalar integer output-only
`LirInlineAsmOp.ordinary_results[0]`. `StmtEmitter::emit_inline_asm` publishes
it with `fresh_value()` plus a typed `LirInlineAsmValueBinding` (Output, index
0); the compatibility `result` remains presentation-only. The schema carries
native value ID, `LirTypeRef`, role, and constraint index. `verify_module`
requires the exact scalar result/type/index tuple, a valid output value ID,
binding uniqueness, and records the ordinary result under current-function
ownership. The selected future receiver handoff may consume only that ordinary
result binding; `asm_text`, constraints, `args_str`, and compatibility result
text are not authority. First downstream bad fact: current inline-asm call
lowering still materializes its result from compatibility `inline_asm.result`,
so any no-text receipt belongs to a later single 734 receiver packet, not this
producer route. Excluded without tracing: accepted 796 casts; CFG/PHI,
vector/aggregate, body-parameter, module-shadow, and local/VLA/memory rows.

## Suggested Next

Step 2: preserve this one scalar output-only producer contract and add only
nearby malformed-authority proof if a producer/schema/verifier gap is found;
do not receive it in Raw-BIR.

## Watchouts

Do not derive authority from rendered operands, labels, templates, constraints,
`args_str`, or testcase identity. Do not modify Raw-BIR/importer code or
expand into other authority families. Read/write and input-bound inline asm
remain nonselected.

## Proof

Recommended focused proof for the selected row: `cmake --build --preset
default && ctest --test-dir build -j --output-on-failure -R
'^backend_lir_to_bir_interface$'` (its inline-asm output receipt/rejection
coverage). No proof was run for this audit-only packet.
