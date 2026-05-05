# Current Packet

Status: Active
Source Idea Path: ideas/open/143_typespec_identity_normalization_boundary.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Triage Broad Validation Regression After Field Removal

## Just Finished

Step 6 broad-validation triage fixed the remaining delegated runtime abort in
`llvm_gcc_c_torture_src_20040703_1_c`. The abort was caused by positional
struct global initializer items being ignored by the constant-initializer
emitter, so `cpp_num num = { 0, 3, 0, 0 }` lowered as all zeros and failed the
runtime result checks. Struct global initializer lowering now maps positional
items through the same field-index path as designated items, preserving the
`low = 3` field without adding rendered-tag consumer fallback or weakening
tests.

## Suggested Next

Continue Step 6 by deciding the next supervisor packet from the remaining
full-suite baseline families: C `00170.c`, gcc torture `20001124-1.c` and
`pr71083.c`, plus the C++ parser/metadata and EASTL owner-dependent template
disambiguation failures.

## Watchouts

- Do not delete `TypeSpec::tag` during this runbook.
- Do not add another local rendered-tag fallback in a consumer as progress.
- Preserve stale-rendered-spelling disagreement tests.
- The fixed mirror failures were producer-side identity mismatches. Keep future
  call/GEP aggregate work on normalized `StructNameId` production; do not relax
  `verify.cpp` mirror/text checks to make the remaining parity failures pass.
- The current delegated GEP layout parity leftovers are cleared.
- `00216.c` now compiles and runs in the delegated proof; the prior `@sinit16`
  invalid `%struct.SS` initializer shape is fixed.
- `20040709-{1,2,3}.c` now compile and run in the delegated proof; the prior
  bitfield global initializer type mismatches such as `@sC` are fixed.
- `20040703-1.c` now compiles and runs in the delegated proof; `@num` lowers as
  `%struct.cpp_num { i32 0, i32 3, i32 0, i32 0 }` instead of a zero aggregate.
- `test_baseline.log` still records these full-suite failure families not
  cleared by this delegated packet: parser TypeSpec structured metadata,
  EASTL/template owner-dependent parser disambiguation, `c_testsuite_src_00170_c`,
  and gcc torture `20001124-1.c`/`pr71083.c`.
- The repaired timeout came from open-coded injected reparses of dependent
  template owner prefixes. Future injected parser routes should use
  `parse_injected_base_type()` or otherwise include an EOF token and restore
  the full parser token stream.
- Rejected baseline candidate: accepting the visible lexical alias
  type-argument failure as baseline drift is not valid because
  `frontend_parser_tests` contains the intended non-fabrication guard and the
  regression is local to parser TypeSpec metadata production.
- Keep idea 141 parked until normalized identity boundaries are stable.
- Do not close idea 143 while full ctest still has failures.

## Proof

Current accepted proof is in `test_after.log`:
`cmake --build build --target c4cll && ctest --test-dir build -j --output-on-failure -R '^(llvm_gcc_c_torture_src_20040703_1_c|c_testsuite_src_00216_c|llvm_gcc_c_torture_src_20040709_(1|2|3)_c|llvm_gcc_c_torture_src_const_addr_expr_1_c)$'`.

The delegated proof now passes 6/6. Passing tests:
`llvm_gcc_c_torture_src_20040703_1_c`,
`c_testsuite_src_00216_c`, `llvm_gcc_c_torture_src_20040709_{1,2,3}_c`, and
`llvm_gcc_c_torture_src_const_addr_expr_1_c`.
