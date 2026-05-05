# Current Packet

Status: Active
Source Idea Path: ideas/open/143_typespec_identity_normalization_boundary.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Triage Broad Validation Regression After Field Removal

## Just Finished

Step 6 broad-validation triage cleared the delegated focused C failure bucket:
`c_testsuite_src_00170_c`, `llvm_gcc_c_torture_src_20000605_1_c`,
`llvm_gcc_c_torture_src_20001124_1_c`, and
`llvm_gcc_c_torture_src_20011008_3_c`. The fixes keep forward record nodes as
structured TypeSpec carriers, let completeness checks upgrade those forward
carriers to later complete definitions by parser-owned identity, preserve
parser-local enum tag identity for compatible forward enum declarations, and
stop HIR owner lookup from treating bare parser `tag_text_id` values as
module-owned structured keys. No tests were weakened and no rendered-tag
consumer fallback was added.

## Suggested Next

Continue Step 6 by deciding the next supervisor packet from the remaining
full-suite baseline families: gcc torture `pr71083.c`, plus the C++
parser/metadata and EASTL owner-dependent template disambiguation failures.

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
- `00170.c` now compiles and runs; forward enum declaration identity is treated
  as the same parser-local enum tag when the definition arrives.
- `20000605-1.c` and `20011008-3.c` now compile and run; typedefs to forward
  records are accepted once the matching complete record definition is known.
- `20001124-1.c` now compiles and runs; `inode->i_sb` keeps the `super_block`
  owner instead of resolving a bare parser `tag_text_id` through the HIR module
  text table as `file`.
- `test_baseline.log` still records these full-suite failure families not
  cleared by this delegated packet: parser TypeSpec structured metadata,
  EASTL/template owner-dependent parser disambiguation, and gcc torture
  `pr71083.c`.
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
`cmake --build build --target c4cll && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_src_00170_c|llvm_gcc_c_torture_src_20000605_1_c|llvm_gcc_c_torture_src_20001124_1_c|llvm_gcc_c_torture_src_20011008_3_c)$'`.

The delegated proof now passes 4/4. Passing tests:
`c_testsuite_src_00170_c`, `llvm_gcc_c_torture_src_20000605_1_c`,
`llvm_gcc_c_torture_src_20001124_1_c`, and
`llvm_gcc_c_torture_src_20011008_3_c`.
