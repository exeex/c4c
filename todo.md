# Current Packet

Status: Active
Source Idea Path: ideas/open/176_lir_type_ref_structured_equality.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The LIR Comparison Surface

## Just Finished

Step 1 selected the bounded LIR comparison surface:
`c4c::codegen::lir::LirTypeRef::operator==(const LirTypeRef&,
const LirTypeRef&)` in `src/codegen/lir/types.hpp`.

Initial files/functions to inspect first:

- `src/codegen/lir/types.hpp`: `LirTypeRef`, especially
  `operator==(const LirTypeRef&, const LirTypeRef&)`,
  `operator!=`, `has_struct_name_id()`, `struct_name_id()`, and
  `struct_type()`.
- `src/codegen/lir/verify.cpp`: verifier consumers that already require
  `StructNameId`, especially `verify_signature_type_ref_semantics`,
  `verify_function_signature_return_type_ref_mirror`,
  `verify_function_signature_param_type_ref_mirror`,
  `verify_call_return_type_ref_mirror`, `verify_call_arg_type_ref_mirror`,
  `verify_extern_decl_shadows`, and `verify_global_type_ref_shadows`.
- `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp`,
  `tests/frontend/frontend_lir_call_type_ref_test.cpp`,
  `tests/frontend/frontend_lir_extern_decl_type_ref_test.cpp`, and
  `tests/frontend/frontend_lir_global_type_ref_test.cpp` for focused
  metadata-rich `LirTypeRef` fixtures.

Exact old comparison to replace or guard:

```cpp
friend bool operator==(const LirTypeRef& lhs, const LirTypeRef& rhs) {
  return lhs.text_ == rhs.text_;
}
```

Structured payload facts available at the comparison site:
`LirTypeRef::kind_`, `integer_bit_width_`, and, most importantly,
`StructNameId struct_name_id_` with `kInvalidStructName` as the no-id sentinel.
The first implementation packet should make equal rendered text insufficient
when both sides are metadata-rich struct refs and their `StructNameId` payloads
differ. Keep no-id compatibility explicit and narrow.

## Suggested Next

Implement Step 2/3 for the selected `LirTypeRef` equality surface: add a focused
probe proving two `LirTypeRef::struct_type("<same rendered text>", different
StructNameId)` values do not compare equal, then update
`LirTypeRef::operator==` to compare `StructNameId` when both sides carry one.
Use the existing LIR metadata fixtures rather than changing printer text.

## Watchouts

- Do not expand into template record owner identity; that belongs to idea 177.
- Do not synthesize structured ids from rendered names at the comparison site.
- Keep printer/display text stable unless a deliberate metadata-observability
  change is required and tested.
- Preserve legacy/no-id compatibility: two raw or hand-authored refs with no
  `StructNameId` can still compare by rendered text.
- Guard only the `StructNameId`-carrying equality path in the first
  implementation packet; avoid widening into template record owner identity or
  backend ABI owner rules.
- `c4c-clang-tool` confirmed the relevant symbols in `src/codegen/lir/types.hpp`
  and `src/codegen/lir/verify.cpp`; no implementation files were changed in
  this inspection packet.

## Proof

No build or test proof required for this inspection-only packet. Suggested
focused proof command for the first implementation packet:

```sh
cmake --build build --target frontend_lir_function_signature_type_ref_test frontend_lir_call_type_ref_test frontend_lir_extern_decl_type_ref_test frontend_lir_global_type_ref_test && ctest --test-dir build -R 'frontend_lir_(function_signature|call|extern_decl|global)_type_ref' --output-on-failure
```
