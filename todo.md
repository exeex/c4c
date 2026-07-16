Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Delete semantic string escape hatches

# Current Packet

## Just Finished

Completed Step 3 packet to delete the remaining implicit string conversion
escape hatches from `LirBinaryOpcodeRef` and `LirCmpPredicateRef` in
`src/codegen/lir/types.hpp`.

Removed both `operator const std::string&()` and `operator std::string_view()`
from each wrapper while preserving explicit `.str()`, `typed()`, equality,
concatenation, and stream output helpers. The compile-enforced proof found no
direct users needing repair.

## Suggested Next

Supervisor should review and commit this Step 3 opcode/predicate wrapper slice
if accepted, then continue with the next remaining Step 3 string escape hatch
candidate. Do not widen the next packet into Step 4 adapter removal.

## Watchouts

- Do not restore a mutable `LirTypeRef::str()` accessor or add a test-only
  backdoor.
- Do not restore mutable `operator std::string&()` or replace it with another
  mutable text escape hatch.
- Do not restore const `LirTypeRef` implicit text conversions; use `.str()` or
  typed/native access at direct users.
- Do not restore implicit `std::string` or `std::string_view` conversions on
  `LirBinaryOpcodeRef` or `LirCmpPredicateRef`; direct users should call
  `.str()` or use `typed()` as appropriate.
- Do not add a renamed generic runtime-text factory; remaining text-backed
  constructions should stay behind named compatibility factories or explicit
  constructors until their own packet deletes them.
- Do not delete const `str()`, additional named compatibility factories,
  non-`LirTypeRef` wrapper conversions, or equality/classification helpers in
  the same packet.
- Step 3 owns verifier-side textual reparsing and textual
  equality/classification authority. Do not mix it with Step 4 adapter removal.
- Signature-store mirror text equality is intentionally preserved at the
  function signature store boundary; it is output-boundary consistency, not
  semantic type authority.
- `same_signature_store_type_fact(...)` in `src/codegen/lir/verify.cpp` and
  `LirModule::same_function_signature_entry(...)` behavior were left intact.
- The remaining `lhs.str() == rhs.str()` search hit in `verify.cpp` is the
  preserved signature-store mirror equality surface, not inline-asm ordinary
  binding agreement.
- The remaining `mirror.str()` checks in `exact_plain_scalar_mirror(...)` are
  integer mirror/output consistency checks, not floating scalar
  classification.
- `function_signature_line(...)` parsing of `signature_text`,
  `aggregate_signature_param_mirror_matches_type(...)` byval fragment checks,
  and direct aggregate signature mirror checks are compatibility/output
  validation surfaces; leave them alone unless separately selected.
- The remaining `LirTypeRef("double")` hit in
  `src/codegen/lir/hir_to_lir/call/target.cpp` is direct double-call authority
  output construction, not the selected-route semantic test.
- Clean-search checks in `target.cpp` found no remaining
  `return_type.str() == "double"/"float"/"x86_fp80"/"fp128"` selected direct
  scalar floating classification, and no remaining local `LirTypeRef("double")`
  selected-route probes.
- `src/codegen/lir/operands.hpp` still has separate string conversions on
  operand refs; that is outside this packet.

## Proof

Proof run passed:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'; } > test_after.log 2>&1`.
`test_after.log` contains the delegated build plus focused CTest output with
`frontend_lir_call_type_ref` passing.
The supervisor-selected proof was sufficient for this packet.
