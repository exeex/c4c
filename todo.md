# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.16
Current Step Title: Publish builtin-ffs cttz call-result/add-use authority (complete)

## Just Finished

- Completed Plan Step 7.16 for only the i32/i64 builtin-ffs `llvm.cttz` call
  and its exact existing add-one lhs use.
- Allocated the call result through `fresh_value`, published a module-owned
  callee `LinkNameId`, native Cttz kind, exact nonvariadic integer/i1 signature
  and matching return/argument refs, and preserved the result ID as the
  add-one lhs.
- Kept the prepared value argument honest monostate SSA or literal Immediate
  presentation when unavailable and published the structural false flag as
  exact i1 `LirIntegerImmediate{0}`.
- Added exact-ID and misleading-display positives plus invalid/duplicate,
  unknown/cross-function, missing/conflicting/unresolved callee, malformed
  signature/count/type/mirror, wrong argument authority, and nonfalse flag
  rejections. The matrix records the exact boundary.

## Suggested Next

- Select the next bounded Step-7 ordinary-value identity row from the source
  idea and establish its exact producer/use boundary before implementation.

## Watchouts

- Preserve the Step-7.16 contract: only PI's i32/i64 ffs cttz call is newly
  authoritative, with native Cttz kind, exact result/add edge, module callee
  ID, fixed integer/i1 signature, prepared value compatibility, and immediate
  false.
- Preserve the repaired arg0 boundary: native authority must be a valid
  current-function SSA ID, while monostate SSA or Immediate presentation is
  accepted without acquiring literal payload authority.
- Keep verifier recognition structural and authority-first; never infer the
  intrinsic, arguments, result, or use from rendered call text.
- Preserve Steps 3, 7.14, and 7.15. Continue excluding other intrinsic/builtin/
  direct/indirect calls, ABI or variadic work, pointer/vector/aggregate/object
  families, CFG/parameters, inline assembly, and BIR.

## Proof

- Fresh `cmake --build --preset default` passed after the Step-7.16 literal
  prepared-value compatibility repair.
- `ctest --test-dir build -R '^(frontend_lir_call_type_ref|positive_sema_ok_call_builtin_runtime_c|llvm_gcc_c_torture_src_builtin_bitops_1_c)$' --output-on-failure > test_after.log`
  passed 3/3: focused authority coverage plus both exact full-guard regressions.
- Focused coverage proves authority-free i32/i64 literal inputs retain
  Immediate presentation, while payload-authoritative literals and unknown SSA
  IDs reject.
- The supervisor's matched full regression guard passed 3033/3033 before and
  after the literal-input repair, with delta 0 passed / 0 failed and no new
  failures.
- `git diff --check` passed for the complete Step-7.16 slice.
