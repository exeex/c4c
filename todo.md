Status: Active
Source Idea Path: ideas/open/345_builtin_vrm_register_carrier_types_to_regalloc_frontier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Type And Inline Asm Carrier Surfaces

# Current Packet

## Just Finished

Step 1 mapped the source-to-prealloc surfaces for VRM carrier identity.

Exact parser/type surfaces that must change:
- `src/frontend/lexer/token.hpp` and `src/frontend/lexer/token.cpp`: recognize
  `__c4c_builtin_vrm1`, `__c4c_builtin_vrm2`, `__c4c_builtin_vrm4`, and
  `__c4c_builtin_vrm8` as type spellings, or ensure `parse_base_type()` can
  consume those identifier spellings as builtin type names without requiring a
  user typedef first.
- `src/frontend/parser/ast.hpp`: add a dedicated `TypeSpec` carrier for VRM
  register footprint, preferably an enum field such as `BuiltinRegisterCarrier`
  or `VrmRegisterGroupWidth`, alongside the existing `TypeBase`.
- `src/frontend/parser/impl/types/base.cpp`: initialize the four builtin
  spellings in `Parser::parse_base_type()` and set the dedicated carrier plus
  width `1`, `2`, `4`, or `8`.
- `src/frontend/parser/impl/types/types_helpers.hpp`: include the new carrier
  in reparse-token generation, builtin type text parsing, mangled suffix
  parsing/emission, and template argument reparsing.
- `src/frontend/sema/type_utils.cpp`, `src/frontend/hir/hir_ir.hpp`, and
  `src/frontend/sema/canonical_symbol.cpp`: include the new carrier in type
  equality, ordering, hashing, canonical symbol conversion, and typedef /
  template-substitution identity. The existing `is_vector`,
  `vector_lanes`, and `vector_bytes` fields are GCC vector storage metadata and
  must not be reused for VRM carriers.
- `src/frontend/hir/impl/inspect/printer.cpp`: dump the carrier as distinct
  text, e.g. `c4c.vrm1`, `c4c.vrm2`, `c4c.vrm4`, and `c4c.vrm8`, not as an
  integer, pointer, struct, or GCC vector suffix.

Exact HIR/LIR surfaces that must change:
- `src/codegen/lir/hir_to_lir/core.cpp`: teach `llvm_value_ty()`,
  `llvm_return_ty()`, and `llvm_alloca_ty()` / adjacent type helpers to lower
  VRM carriers to a stable non-LLVM-storage spelling or structured `LirTypeRef`
  such as `c4c.vrmN`; do not route through `<N x T>`, integer, pointer, or
  aggregate storage.
- `src/codegen/lir/types.hpp`: add a `LirTypeKind` or equivalent structured
  recognition for VRM carrier refs so verifier/printer paths do not treat
  `c4c.vrmN` as arbitrary raw text.
- `src/codegen/lir/lir_printer.cpp` and `src/codegen/lir/verify.cpp`: allow
  the new stable spelling in dumps and type-ref validation.

Exact BIR surfaces that must change:
- `src/backend/bir/bir.hpp`: extend `bir::TypeKind` or add equivalent value
  metadata for VRM widths; current `TypeKind` only has void, integer, pointer,
  and float/f128 storage kinds.
- `src/backend/bir/lir_to_bir/types.cpp`: lower `c4c.vrm1`, `c4c.vrm2`,
  `c4c.vrm4`, and `c4c.vrm8` into that BIR representation instead of failing
  or decaying through `lower_scalar_storage_type()`.
- `src/backend/bir/bir_printer.cpp`: print the BIR type distinctly as
  `c4c.vrmN` or an equivalent structured rendering.
- `src/backend/bir/lir_to_bir/calling.cpp`: keep existing inline asm metadata
  parsing for `VR`, `VRM2`, and `VRM4`, add `VRM8`, and keep scalar
  constraints as `general`. The type route must supply vector identity before
  prealloc; constraint parsing should validate or reinforce it.

Exact prepared inline asm and prealloc surfaces that must change:
- `src/backend/prealloc/regalloc/classification.cpp`: classify VRM BIR values
  as `PreparedRegisterClass::Vector` and return group width `1`, `2`, `4`, or
  `8` directly from value type/metadata. Current vector-group overrides are
  populated from inline asm metadata before regalloc and must not remain the
  only source of vector identity.
- `src/backend/prealloc/inline_asm.cpp`: preserve existing carrier validation
  against `PreparedValueHome::target_register_identity`, add `VRM8`, and add
  compatibility checks so a vector constraint width matches the VRM value width.
  The existing scalar negative path should still report
  `operandN_home_incompatible_register_class` or an equivalent incompatibility
  when `"VRM2"` is applied to a scalar `long`.
- `src/backend/prealloc/regalloc.hpp`, `src/backend/prealloc/value_locations.hpp`,
  `src/backend/prealloc/prepared_printer/inline_asm.cpp`, and
  `src/backend/prealloc/prepared_printer/regalloc.cpp`: ensure prepared dumps
  expose vector class and contiguous width for VRM values.

Selected representation route:
- Add a dedicated source `TypeSpec` VRM carrier with width enum/value, lower it
  to a stable `LirTypeRef` spelling `c4c.vrmN`, lower that to a dedicated BIR
  type/metadata carrier, and classify that BIR carrier as vector register class
  plus contiguous width in prealloc. Inline asm constraints remain validation
  facts, not the semantic source of VRM identity.

Unsupported ABI/storage surfaces for this runbook unless a later packet makes
them explicit:
- Ordinary function parameter and return ABI for VRM carriers.
- Globals, static storage, alloca-backed storage, load/store, address-taking,
  array/member storage, and initializer semantics for VRM carriers.
- Arithmetic, casts, comparisons, element access, lane semantics, GCC vector
  operations, and element dtype/lane-count modeling.
- Final register allocation placement policy, substitution, object emission,
  `.insn.d` encoding, relocation, linker, and disassembler behavior.

## Suggested Next

Delegate Step 2 to `c4c-executor`: add the source `TypeSpec` VRM carrier,
parse the four builtin spellings, preserve typedef/template identity in the
existing type identity helpers, and add frontend/HIR dump proof for direct and
typedef spellings.

## Watchouts

- Do not treat scalar values, `long`, or `i64` as vector registers.
- Do not reuse GCC vector metadata for VRM carriers.
- The existing prepare route runs `populate_inline_asm_register_group_overrides()`
  before `run_regalloc()`, so `VRM*` constraints can currently force vector
  register group overrides for named operands. That path is useful for
  validation but is not sufficient for source-level VRM identity.
- `VRM8` is currently unsupported by inline asm constraint parsing; `VR`,
  `VRM2`, and `VRM4` are recognized.
- Do not drift into `.insn.d` encoding, EV instruction design, final register
  allocation, linker, relocation, or disassembler work.
- Keep scalar mismatch diagnostics active.

## Proof

Ran delegated proof successfully:
`cmake --build --preset default > test_after.log 2>&1`.

Proof log: `test_after.log` (`ninja: no work to do.`).
