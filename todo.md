Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Delete mutable LIR type text escape hatches in small packets

# Current Packet

## Just Finished

Completed Step 2 packet to delete the two const implicit text conversions from
`LirTypeRef`: `operator const std::string&() const` and
`operator std::string_view() const`. No direct callsite edits were required
because existing users already compile through `.str()`, typed/native access,
or retained non-`LirTypeRef` wrapper conversions.

## Suggested Next

Continue Step 2 with the next narrow `LirTypeRef` compatibility surface. A good
next packet is to classify one remaining named deprecated compatibility factory
family and either delete or replace that family without touching
equality/classification helpers in the same slice.

## Watchouts

- Do not restore a mutable `LirTypeRef::str()` accessor or add a test-only
  backdoor.
- Do not restore mutable `operator std::string&()` or replace it with another
  mutable text escape hatch.
- Do not restore const `LirTypeRef` implicit text conversions; use `.str()` or
  typed/native access at direct users.
- Do not add a renamed generic runtime-text factory; remaining text-backed
  constructions should stay behind named compatibility factories or explicit
  constructors until their own packet deletes them.
- Do not delete const `str()`, remaining named compatibility factories,
  non-`LirTypeRef` wrapper conversions, or equality/classification helpers in
  the same packet.
- Required scalar-to-vector splat shuffles now reject incoherent native
  `mask_type` mirrors against vector-store lane count; do not weaken that
  baseline repair.
- Remaining `.str() =` lines in the unowned
  `tests/frontend/frontend_lir_call_type_ref_test.cpp` are `LirOperand`
  presentation mutations, not `LirTypeRef` mutations.
- `rg -n "LirTypeRef::runtime_text" src tests/frontend tests/backend` is now
  clean.
- `rg -n "operator const std::string&\(\) const|operator std::string_view\(\) const" src/codegen/lir/types.hpp`
  still reports `LirBinaryOpcodeRef` and `LirCmpPredicateRef`; those conversions
  are explicitly outside this packet and should not be treated as `LirTypeRef`
  matches.

## Proof

Proof run passed:
`cmake --build build && ctest --test-dir build -R
'^(frontend_lir_call_type_ref|frontend_lir_extern_decl_type_ref|backend_lir_to_bir_interface)$'
--output-on-failure > test_after.log 2>&1`.
`test_after.log` contains the focused CTest subset output with 3/3 tests
passing. The supervisor-selected proof was sufficient for this packet.
