# Global Policy And Symbol-Identity Route

## Diagnostic Trace

First diagnostic action: trace every `LirGlobal` policy and identity field from
`LirGlobal` storage through producer lowering, LIR verification, printing, and
Raw-BIR import.

Primary locations:

- `src/codegen/lir/ir.hpp`: `LirGlobal` carries `name`, `link_name_id`,
  `is_internal`, `is_const`, `linkage_vis`, `qualifier`, `align_bytes`, and
  `is_extern_decl`. `llvm_type`, `llvm_type_ref`, `init_text`,
  `initializer_elements`, and `initializer_function_link_name_ids` are adjacent
  fields but only initializer function links and label-address elements are
  structured semantic references in this route.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`: `lower_global` computes
  `linkage_vis` from `GlobalVar.linkage`, `qualifier` from `is_const` and
  pointer level, preserves `link_name_id`, and copies extern/definition flags.
- `src/codegen/lir/verify.cpp`: current global verifier coverage is for
  structured type shadows and structured initializer label-address elements;
  it does not verify `linkage_vis`, `qualifier`, `is_internal`, `is_const`,
  `is_extern_decl`, or `link_name_id` coherence as a global policy family.
- `src/codegen/lir/lir_printer.cpp`: global printing resolves `link_name_id`
  for the symbol spelling and then emits `linkage_vis`, `qualifier`, type,
  initializer text, and alignment.
- `src/backend/bir/lir_to_bir.cpp`: `decode_global_linkage` parses
  `linkage_vis` into `is_weak` and `SymbolVisibility`; module-surface
  validation checks coherent policy combinations and link-name identity; Raw
  BIR import calls `ModuleBuilder::add_global_object`.
- `src/backend/bir/core/ir.hpp` and `src/backend/bir/core/builder.cpp`:
  `GlobalObject` stores source name, typed object, link-name or fallback
  identity, alignment, `is_internal`, `is_weak`, `is_const`,
  `is_extern_declaration`, visibility, and opaque initializer plus ordered
  initializer function links.

Excluded from this route:

- Global and extern type facts are owned by closed idea 844.
- Initializer text semantics, scanner deletion, and raw initializer parsing are
  outside 848.
- `LirGlobalInitializerLabelAddress` and initializer function-link checks are
  structured semantic initializer references, not linkage/visibility/qualifier
  policy fields.

## Field Dispositions

| Field or fact | Producer evidence | LIR verifier evidence | New-BIR receiver evidence | Disposition |
| --- | --- | --- | --- | --- |
| `link_name_id` symbol identity | `lower_global` copies `GlobalVar.link_name_id` into `LirGlobal.link_name_id`; other LIR reference producers use `LinkNameId` for direct globals. | No general `LirGlobal.link_name_id` verifier gate was found. Initializer label-address elements verify function `LinkNameId`, but global object identity is not checked there. | `validate_module_surface` rejects unresolved, mismatched, or duplicate link-backed globals; `add_global_object` requires the source link-name to resolve to the same spelling as `source_name`. Tests include global identity survival and unresolved-id rejection in `backend_lir_to_bir_notes_test.cpp`, plus Raw-BIR view lookup coverage in `backend_lir_to_bir_interface_test.cpp`. | Receiver-backed, but producer-to-verifier evidence is incomplete. |
| `linkage_vis` external/internal/weak/visibility spelling | `lower_global` builds the field with `make_linkage_vis` from `is_static`, `is_weak`, `is_extern`, and `Visibility`. | No LIR verifier gate was found for malformed `linkage_vis` or disagreement with `is_internal` / `is_extern_decl`. | `decode_global_linkage` admits only coherent prefixes and visibility suffixes and feeds Raw-BIR `is_weak` and `visibility`. Interface tests cover external hidden, protected, internal hidden, weak, extern_weak protected, and malformed rollback through global receipt tests. | Receiver-backed, but LIR verifier coverage is missing. |
| `is_internal` | `lower_global` copies `gv.linkage.is_static`; extern globals force non-internal. | No LIR verifier gate was found for `is_internal` consistency. | `decode_global_linkage` and coherent-shape validation reject contradictions such as internal extern declarations; Raw BIR stores `is_internal`. Interface tests inspect internal ordinary and internal constant definitions. | Receiver-backed, but LIR verifier coverage is missing. |
| `is_const` and `qualifier` | `lower_global` copies `gv.is_const` and emits `"constant "` only for non-pointer const objects; pointer-typed const globals use `"global "`. | No LIR verifier gate was found for `is_const` / `qualifier` consistency. | Module-surface validation admits only coherent qualifier/const/type combinations, including the const-pointer special case, and Raw BIR stores `is_const`. Interface tests inspect ordinary, constant, weak constant, internal constant, and const-pointer definitions. | Receiver-backed, but LIR verifier coverage is missing. |
| `is_extern_decl` | `lower_global` sets extern declarations from `gv.linkage.is_extern`, leaves initializer text empty, and uses `"global "`. | No LIR verifier gate was found for extern declaration policy consistency. | Module-surface validation distinguishes extern and weak extern declarations from initialized definitions and rejects incoherent shapes; Raw BIR stores `is_extern_declaration`. Interface tests inspect external and weak-external declarations. | Receiver-backed, but LIR verifier coverage is missing. |
| `align_bytes` | `lower_global` computes object alignment with `object_align_bytes`. | No LIR verifier gate was found for global alignment. | Module-surface validation rejects negative or non-power-of-two nonzero alignments; Raw BIR stores alignment. Interface tests inspect alignments across global rows. | Receiver-backed, but LIR verifier coverage is missing. |
| `initializer_function_link_name_ids` | HIR-to-LIR collection records function `LinkNameId` references from initializer payloads. | Structured label-address initializer elements verify enclosing function and block identity; the plain function-link vector is not a full initializer semantic verifier. | `add_global_object` resolves initializer function links through the Raw-BIR link-name table and rejects unresolved links. Interface tests preserve ordered initializer links. | Structured reference is receiver-backed; broader initializer semantics are outside 848. |

## Positive And Malformed Evidence

Positive receiver evidence exists in
`tests/backend/bir/backend_lir_to_bir_interface_test.cpp`:

- `test_global_object_receipt_and_views` verifies source order, fallback and
  link-name identity, external declarations, initialized definitions,
  constants, internals, weak definitions, weak extern declarations,
  visibility, alignment, opaque initializer payloads, and ordered initializer
  function links.
- Neighboring global storage tests verify that enum, VRM, complex, pointer,
  array, and aggregate global rows keep the same policy/object facts while
  their type facts are handled separately.

Malformed receiver evidence exists in the Raw-BIR importer and tests:

- `validate_module_surface` rejects malformed or contradictory `linkage_vis`,
  incoherent qualifier/const/extern/internal/weak combinations, invalid
  alignment, unresolved/mismatched/duplicate `link_name_id`, and unresolved
  initializer function links.
- `ModuleBuilder::add_global_object` repeats fail-closed identity and
  initializer-link checks at the builder boundary.
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp` has explicit
  LinkNameId identity drift and unresolved-id rejection checks for the older
  BIR lowering surface.

Missing evidence:

- No global-family LIR verifier gate was found for `linkage_vis`, `qualifier`,
  `is_internal`, `is_const`, `is_extern_decl`, `align_bytes`, or
  `link_name_id` coherence.
- The receiver decodes weak/visibility from `linkage_vis`, so policy authority
  is not yet fully independent of a rendered-like string at the LIR boundary.
- Existing tests prove Raw-BIR receipt and rollback for many policy shapes, but
  they do not prove a complete producer-to-LIR-verifier contract for every
  policy field before import.

## Return Relation

844 may reuse this as evidence that non-type policy fields are separate from
closed global/extern type-fact migration.

734 must not receive a new global policy/symbol-identity row from this evidence
alone. No exact typed Raw-BIR receiver handoff is authorized because the
producer/verifier side still lacks a dedicated global policy authority contract
for the string-carried policy fields. The next implementation owner, if one is
created, should publish and verify structured global policy facts before 734
consumes them.

797 must await either that later producer/verifier route or an explicit
evidence-backed no-change decision that documents why existing Raw-BIR receipt
is sufficient despite the missing LIR verifier boundary.

Declaration rendering is not an authority source for this route. Existing
receiver code must continue to reject malformed or contradictory policy shapes
rather than recovering facts from rendered declarations, display names, or raw
LLVM text.
