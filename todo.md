Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Delete semantic string escape hatches

# Current Packet

## Just Finished

Completed Step 3 packet to delete scalar comparison operand preservation that
used direct `source_lv.str() == lv` / `source_rv.str() == rv` text equality as
semantic authority in `src/codegen/lir/hir_to_lir/expr/binary.cpp`.

Added comparison-specific operand preservation that keeps native SSA authority
only when the original operand type matches the comparison type, and keeps
integer immediate authority only when the immediate is representable by the
comparison type. Scalar comparison fallback now reconstructs raw normalized
operands instead of treating matching display text as authority.

## Suggested Next

Supervisor should review and commit this Step 3 scalar comparison preservation
slice if accepted, then select the next remaining Step 3 semantic string escape
hatch candidate. Do not widen the next packet into Step 4 adapter removal.

## Watchouts

- `preserve_exact_binary_operand(...)` still contains its existing display
  equality guard for scalar arithmetic; this packet only owned scalar
  comparison operand preservation.
- Scalar comparison preservation now depends on source TypeSpec snapshots taken
  before usual arithmetic conversion. Preserve that distinction if later
  packets refactor the conversion flow.
- Do not restore a mutable `LirTypeRef::str()` accessor or add a test-only
  backdoor.
- Do not restore mutable `operator std::string&()` or replace it with another
  mutable text escape hatch.
- Do not restore const `LirTypeRef` implicit text conversions; use `.str()` or
  typed/native access at direct users.
- Do not restore implicit `std::string` or `std::string_view` conversions on
  `LirBinaryOpcodeRef` or `LirCmpPredicateRef`; direct users should call
  `.str()` or use `typed()` as appropriate.
- Do not restore implicit `std::string&`, `const std::string&`, or
  `std::string_view` conversions on `LirOperand`; direct users should call
  `.str()` or use native operand authority access as appropriate.
- Do not restore mutable `LirOperand::str()`; tests that need stale display
  text should use explicit reconstruction while preserving native authority.
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
- `collect_inst_refs(...)` now routes affected `LirOperand` fields through
  `collect_operand_ref(...)`, so structured link-name authority is preferred
  before fallback text scanning.
- `clang-format` was not available in this environment, so no automatic
  formatting pass was run.

## Proof

Baseline captured before edits with:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'; } > test_before.log 2>&1`.

Proof run passed after edits:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'; } > test_after.log 2>&1`.
`test_after.log` contains the delegated build plus focused CTest output with
`frontend_lir_call_type_ref` passing.
The supervisor-selected proof was sufficient for this packet.
