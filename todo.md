Status: Active
Source Idea Path: ideas/open/310_prepared_indirect_call_string_argument_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Indirect-Call String Argument Facts

# Current Packet

## Just Finished

Step 2 extended the existing LIR-to-BIR string pointer argument rewrite so it
now runs over both direct and indirect call pairs. Direct calls still use the
existing direct-global typed-call parser; indirect calls use the typed-call
parser and the same `collect_string_pointer_aliases()` facts to rewrite pointer
arguments that resolve to string-pool constants into named `@...` string
values before stack layout observes them.

`backend_prepare_stack_layout` now keeps the direct-call string argument
fixture and keeps the BIR-only indirect-call consumer fixture. It also adds a
LIR-origin indirect-call fixture that starts with an all-zero
`getelementptr @.L.str...`, lowers through LIR-to-BIR, runs the stack-layout
path, and verifies the resulting prepared
`PreparedAddressMaterializationKind::StringConstant` record. No stack-layout
coordinator changes were needed.

## Suggested Next

Supervisor should review and commit the Step 2 slice, then choose the next
packet from the active plan.

## Watchouts

The rewrite remains call-pair based and does not infer from `00189.c`, callee
names, argument indexes, assembly, or AArch64 lowering. If future work observes
functions where lowered BIR call counts diverge from LIR call counts for the
same direct/indirect lane, that should be handled as a producer-pairing issue,
not by adding target-specific string guessing. The strengthened local fixture
uses a synthetic indirect callee pointer and private string name so the unit
coverage is not coupled to `00189.c`, `stdout`, `fprintfptr`, `.str1`, or a
fixed indirect-call argument index.

## Proof

Passed. Proof log: `test_after.log`.

Command:
`{ cmake --build --preset default --target backend_prepare_stack_layout_test c4cll && ctest --test-dir build -R '^backend_prepare_stack_layout$' --output-on-failure && ./build/c4cll --dump-prepared-bir --mir-focus-function main tests/c/external/c-testsuite/src/00189.c | rg 'address_materialization .*inst_index=4 kind=string_constant .*text=\.str1 '; } > test_after.log 2>&1`

Additional supervisor validation after review:
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 139/139.
The monotonic regression-guard parser was not accepted for this proof shape
because the CTest pass count was unchanged; the meaningful before/after delta
is the prepared-BIR materialization check moving from absent to present.
