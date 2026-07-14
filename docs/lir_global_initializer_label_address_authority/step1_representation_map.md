# Step 1 — Static initializer representation map

## Result

The one LIR seam is a structured global-initializer element owned by
`LirGlobal`, published by global lowering from `ConstInitEmitter`'s HIR walk,
and checked by `verify_module`.  Its label-address alternative must carry the
pair `{ enclosing_function: LinkNameId, target: LirBlockId }`; the pair makes
the target function-scoped and therefore stable without treating rendered
`blockaddress(...)` text as authority.

This is a generic initializer-element seam: it is not an operand, direct-rvalue,
local-storage, or computed-goto carrier contract.

## Evidence map

| Boundary | Present authority | Missing authority / implication |
| --- | --- | --- |
| HIR `LabelAddrExpr` | `fn_link_name_id` identifies the enclosing function. | It retains `label_name` only. Global lowering must resolve that name to the function's stable block identity before publication; spelling alone is not an LIR target identity. |
| `ConstInitEmitter::emit_const_scalar_expr` | Serializes `LabelAddrExpr` as `blockaddress(@function, %ulbl_label)`, including integer-expression forms. | The serialization is presentation text only. It publishes no initializer element. |
| `collect_global_init_function_link_name_ids` and `lower_global` | Every emitted `LirGlobal` receives `initializer_function_link_name_ids`; label-address visits add the enclosing function ID. | The vector loses the target and ordering/element association, so it cannot establish a label address. |
| `LirGlobal` | It has `init_text` plus the function-ID vector, and `llvm_type_ref` for type identity. | It has no structured initializer element or function-scoped block target. The smallest extension point is here, rather than a new direct/local carrier. |
| `verify_module` | It already validates function-scoped `LirBlockId` ownership for terminators and indirect-branch successor mirrors. | It does not visit global initializer semantics. Add one global-initializer validation pass that checks a label-address element's valid function ID, exactly-one matching `LirFunction`, valid block ID within that function, and its display label only as a checked mirror if retained. |

`LirBlockId` is the existing equivalent stable target identity: it is validated
against exactly one block in its enclosing `LirFunction`.  It is intentionally
not globally unique; retaining the enclosing `LinkNameId` is what makes the
pair unambiguous.

## Publisher/verifier seam

The Step 2 packet is limited to:

1. introduce a structured `LirGlobal` initializer-element model with a
   label-address alternative containing `LinkNameId` and `LirBlockId`;
2. make the constant-initializer/global-lowering route publish those elements
   while leaving `init_text` as compatibility/display spelling; and
3. make the LIR verifier validate the pair against `LirModule::functions` and
   the selected function's blocks.

The current `initializer_function_link_name_ids` is function-only metadata and
must not be treated as a substitute for the element.  No direct/local or
carrier route is selected by this map.

## First downstream boundary (not crossed)

The first downstream consumer boundary is Raw-BIR/importer global lowering in
`src/backend/bir/lir_to_bir/globals.cpp`: `lower_scalar_global`,
`lower_global_info`, and aggregate lowering consume `LirGlobal::init_text` and
at most `initializer_function_link_name_ids`, then parse raw initializer
spelling into global-address data.  That path has no importer field for a
function-plus-block label-address element.  Therefore it cannot consume the
new LIR authority without a distinct Raw-BIR/importer contract; this packet
only names that boundary and does not modify it.

## Recorded AST-backed queries

- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/codegen/lir/hir_to_lir/const_init_emitter.cpp /workspaces/c4c/build/compile_commands.json`
- `c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/codegen/lir/hir_to_lir/const_init_emitter.cpp try_emit_global_address_expr /workspaces/c4c/build/compile_commands.json`
- `c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/codegen/lir/hir_to_lir/const_init_emitter.cpp emit_const_init /workspaces/c4c/build/compile_commands.json`
- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/codegen/lir/verify.cpp /workspaces/c4c/build/compile_commands.json`
- `c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/codegen/lir/verify.cpp verify_module /workspaces/c4c/build/compile_commands.json`

The compile database did not expose `src/backend/bir/lir_to_bir/globals.cpp` as
a direct translation unit, so its first-boundary finding is constrained to the
narrow source spans that consume `LirGlobal`; no importer implementation work
was performed.
