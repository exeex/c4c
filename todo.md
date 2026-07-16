Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Delete mutable LIR type text escape hatches in small packets

# Current Packet

## Just Finished

Completed Step 2 packet to delete the generic
`LirTypeRef::runtime_text` factory. Existing named deprecated compatibility
factories now construct `LirTypeRef` directly, local HIR-rendered compatibility
helpers route through constructors, and focused tests use explicit constructors
for intentionally unstructured type text.

## Suggested Next

Continue Step 2 with the next narrow `LirTypeRef` compatibility surface. A good
next packet is to classify the remaining deprecated named compatibility
factories or const implicit conversions and delete one family without touching
equality/classification helpers in the same slice.

## Watchouts

- Do not restore a mutable `LirTypeRef::str()` accessor or add a test-only
  backdoor.
- Do not restore mutable `operator std::string&()` or replace it with another
  mutable text escape hatch.
- Do not add a renamed generic runtime-text factory; remaining text-backed
  constructions should stay behind named compatibility factories or explicit
  constructors until their own packet deletes them.
- Do not delete const `str()`, remaining named compatibility factories, const
  implicit conversions, or equality/classification helpers in the same packet.
- Required scalar-to-vector splat shuffles now reject incoherent native
  `mask_type` mirrors against vector-store lane count; do not weaken that
  baseline repair.
- Remaining `.str() =` lines in the unowned
  `tests/frontend/frontend_lir_call_type_ref_test.cpp` are `LirOperand`
  presentation mutations, not `LirTypeRef` mutations.
- `rg -n "LirTypeRef::runtime_text" src tests/frontend tests/backend` is now
  clean.

## Proof

Proof run passed:
`cmake --build build && ctest --test-dir build -R
'^(frontend_lir_call_type_ref|frontend_lir_extern_decl_type_ref|backend_lir_to_bir_interface)$'
--output-on-failure > test_after.log 2>&1`.
`test_after.log` contains the focused CTest subset output with 3/3 tests
passing.
