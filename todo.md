Status: Active
Source Idea Path: ideas/open/114_lir_type_text_authority_demotion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Type-Text Authority Inventory

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory for the candidate LIR type-text
authority surfaces.

Classification:

- `LirGlobal::llvm_type`: emission-only for the LIR printer, bridge-required
  for BIR/MIR global lowering and initializer parsing, proof-only where
  `LirGlobal::llvm_type_ref` shadows it in the verifier. Leave the field
  untouched; full demotion is blocked by backend global lowering still reading
  `global.llvm_type` as type/layout authority.
- `LirExternDecl::return_type_str`: emission-only for printed extern
  declarations, bridge-required for BIR/MIR extern return lowering, proof-only
  where `LirExternDecl::return_type` shadows it in the verifier. Leave the
  field untouched; backend lowering still tries the text first and only falls
  back to the structured mirror text.
- `LirFunction::signature_text`: emission-only for the LIR printer,
  bridge-required for BIR/MIR function return/parameter ABI parsing, proof-only
  where signature return/parameter `LirTypeRef` mirrors are checked by the
  verifier. Leave the field untouched; backend declaration and parameter
  lowering still parse the rendered signature.
- Selected call return/argument formatted type text: emission-only for printed
  call sites, proof-only/semantic in verifier parity checks. `LirCallOp` already
  carries `return_type` and optional `arg_type_refs`; direct aggregate call
  tests prove `StructNameId` mirrors for return and argument refs, while
  variadic/incomplete parse paths intentionally have no argument mirrors.
- Selected raw `LirTypeRef` text-only comparisons: semantic authority for broad
  instruction type identity because `operator==`, stream output, and many
  operation fields still use raw text. Leave untouched as
  `type-ref-authority-blocked`.
- Verifier checks that can become structured-first with text fallback: safe
  first demotion family. The smallest coherent packet is the `LirCallOp`
  verifier mirror family in `src/codegen/lir/verify.cpp`: keep formatted call
  text as printer/fallback proof, but make available `LirTypeRef`/
  `StructNameId` identity primary for call return and argument validation.

Selected first demotion family: `LirCallOp` verifier call return/argument type
mirror checks. This is smaller than globals, externs, or function signatures
because it is localized to verifier/call mirror validation and does not require
changing backend lowering or final LLVM text emission.

## Suggested Next

Execute `plan.md` Step 2 for the selected `LirCallOp` verifier family: convert
call return/argument validation to structured-first identity where
`LirTypeRef`/`StructNameId` mirrors exist, keeping parsed formatted call text as
fallback/proof for incomplete or non-mirrored calls.

## Watchouts

- Do not touch `LirGlobal::llvm_type`, `LirExternDecl::return_type_str`, or
  `LirFunction::signature_text` as part of the first packet; those are still
  bridge-required by printer/backend consumers.
- Do not change raw `LirTypeRef` equality/output semantics; broad type-ref
  authority remains blocked.
- Keep formatted call-site text byte-for-byte stable in printer output.
- Variadic aggregate calls currently omit argument mirrors when the signature
  cannot parse against emitted ABI arguments; preserve that fallback boundary.
- Avoid testcase-shaped matching or expectation downgrades.

## Proof

Inventory-only packet; no implementation proof required and no
`test_after.log` was produced. Suggested proof for the first demotion family:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_call_type_ref|frontend_lir_|positive_split_llvm_)'`.
