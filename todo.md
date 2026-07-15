# Current Packet

Status: Active
Source Idea Path: ideas/open/816_lir_next_residual_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove and hand off the bounded row

## Just Finished

Step 2 complete — independent audit found no code patch required: the selected
scalar integer output-only `LirInlineAsmOp.ordinary_results[0]` contract is
already committed. `StmtEmitter::emit_inline_asm` assigns a fresh value and a
typed `Output`/index-0 binding; the schema retains the native value ID and
`LirTypeRef`. The verifier requires the exact tuple, a native valid ID, and
current-function ownership. Nearby malformed tests reject missing, invalid,
duplicate, role-mismatched, index-mismatched, type-mismatched, and foreign
forms. Fresh exact focused proof passed: `cmake --build --preset default &&
ctest --test-dir build -j --output-on-failure -R
'^backend_lir_to_bir_interface$'`; matching before/after regression guard
passed (1/1 tests).

## Suggested Next

Execute Step 3 as a bounded semantic handoff/proof task: record the one 734
receiver row allowed to consume the fresh value plus typed Output/index-0
binding, the native-ID/current-function verifier conditions, and the rejected
missing/invalid/duplicate/role/index/type/foreign forms. Compatibility
`result` remains presentation-only; Raw-BIR receipt is deferred to 734.

## Watchouts

Do not derive authority from rendered operands, labels, templates, constraints,
`args_str`, compatibility `result`, or testcase identity. Do not modify
Raw-BIR/importer code, downstream result text, or other inline-assembly
authority families. Read/write and input-bound inline asm remain nonselected;
Step 3 does not claim source-idea completion.

## Proof

Required: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_lir_to_bir_interface$'`; retain matching
before/after evidence if the supervisor selects regression comparison.
