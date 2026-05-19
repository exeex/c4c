Status: Active
Source Idea Path: ideas/open/310_prepared_indirect_call_string_argument_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the Existing Direct-Call String Argument Authority

# Current Packet

## Just Finished

Step 1 localized the existing direct-call string argument authority and the
missing indirect-call branch.

Direct-call string argument authority is split across two producer surfaces:
`src/backend/bir/lir_to_bir.cpp` materializes string constants and
`rewrite_direct_call_string_pointer_args()` rewrites direct BIR call pointer
arguments that alias all-zero LIR `getelementptr @.str*` results into named
string values such as `@.L.str.call`; then
`src/backend/prealloc/stack_layout/coordinator.cpp`
`append_call_argument_address_materialization()` recognizes those named
`@...` string values through `direct_string_constant_name()` and publishes a
`PreparedAddressMaterializationKind::StringConstant` record with result value
identity, text identity, byte offset 0, and default address space.

The exact missing branch is in the LIR-to-BIR rewrite pass: it only collects
direct LIR calls (`collect_direct_lir_calls()`) and direct BIR calls
(`collect_direct_bir_calls()`), so indirect calls are never considered even
when `collect_string_pointer_aliases()` already knows an indirect call argument
SSA value aliases a string-pool `getelementptr`. The stack-layout coordinator
then only sees the indirect call argument as `%t2`, not `@.str1`, so
`direct_string_constant_name()` cannot publish the string materialization.

Focused current-state evidence:
`./build/c4cll --dump-bir --mir-focus-function main tests/c/external/c-testsuite/src/00189.c`
shows the outer call as `%t5 = bir.call i32 %t0(ptr %t1, ptr %t2, i32 %t4)`.
`./build/c4cll --dump-prepared-bir --mir-focus-function main tests/c/external/c-testsuite/src/00189.c`
shows prepared addressing for `main` only has global-symbol accesses for
`%t0`, `%t1`, and `%t3`; the outer call arg1 remains a register source `%t2`
with no prepared string-constant materialization.

## Suggested Next

Implement Step 2 by extending the existing LIR-to-BIR string-pointer rewrite
surface, not AArch64 lowering: generalize
`rewrite_direct_call_string_pointer_args()` so it also pairs indirect LIR calls
with indirect BIR calls and rewrites pointer arguments whose operands resolve
through `collect_string_pointer_aliases()`. The smallest likely implementation
surface is `src/backend/bir/lir_to_bir.cpp`, plus focused prepare coverage in
`tests/backend/bir/backend_prepare_stack_layout_test.cpp` that mirrors the
direct-call fixture with an indirect callee and a string pointer argument.

## Watchouts

- Do not repair this by guessing in AArch64 from `00189.c`, `stdout`,
  `fprintfptr`, `.str1`, argument index, assembly text, or one-string
  heuristics.
- Do not edit expectations, allowlists, unsupported classifications, runner
  behavior, timeout policy, CTest registration, proof logs, or test contracts.
- Do not expand this prerequisite into idea 309's AArch64 callee/register
  preservation; return to idea 309 only after the prepared string argument fact
  exists.
- Keep the rewrite semantic and call-pair based: indirect callee identity is
  irrelevant to the string argument fact, and the accepted proof should show an
  arbitrary indirect pointer call can publish the same prepared string
  materialization as a direct call.
- Preserve existing direct-call behavior and the direct-call fixture. The
  coordinator path already handles `@string` call arguments once BIR presents
  them with string identity.

## Proof

Read-only localization packet. No build or CTest was required and no
`test_after.log` was written.

Recommended supervisor proof command for the Step 2 implementation packet:
`cmake --build build --target backend_prepare_stack_layout_test c4cll && ctest --test-dir build -R '^backend_prepare_stack_layout$' --output-on-failure && ./build/c4cll --dump-prepared-bir --mir-focus-function main tests/c/external/c-testsuite/src/00189.c`
