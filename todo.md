# Execution State

Status: Active
Source Idea Path: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Prepared-Module Or Call-Bundle Seam
Plan Review Counter: 5 / 10
# Current Packet

## Just Finished

Plan Step 2.1 repaired the producer-side prepared call-bundle publication seam
for the reduced same-module variadic stack-argument lane. In
`src/backend/prealloc/regalloc.cpp`, the prepared call publisher now resolves
missing variadic `call.arg_abi` entries from `call.arg_types`, uses those
resolved ABI records when classifying call-argument storage and stack offsets,
and publishes the matching `BeforeCall` ABI bindings/move resolution for the
extra stack-passed arguments instead of silently dropping them.

The same packet also extends the x86 prepared-module consumer and supporting
prepared-address publication for the adjacent same-module helper/local-byval
lane that now depends on those canonical handoff facts. The x86 helper-prefix
path can consume the repaired publication and the related authoritative stack /
symbol addressing instead of falling back to the old top-level minimal-return
gate immediately.

I added a focused boundary check in
`tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp` for a
same-module `arg -> myprintf` variadic call with one fixed format argument and
four extra stack-passed pointer arguments lacking explicit `arg_abi`. That
check now passes and confirms the canonical x86 handoff publication:
`rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`, then stack offsets `0`, `8`, `16`.

The reduced reproducer `/tmp/probe_same_module_variadic_stack_ptr.c` no longer
fails with `x86 backend emitter requires the authoritative prepared
call-bundle handoff through the canonical prepared-module handoff`. It now
stops later at the broader top-level x86 route gate:
`x86 backend emitter only supports a minimal single-block i32 return
terminator, a bounded equality-against-immediate guard family with immediate
return leaves including fixed-offset same-module global i32 loads and
pointer-backed same-module global roots, or one bounded compare-against-zero
branch family through the canonical prepared-module handoff`.

That means this owned producer seam is repaired. The next blocker is no longer
prepared call-bundle publication.

## Suggested Next

Take the next packet on the broader x86 prepared-module acceptance gate that now
blocks both `/tmp/probe_same_module_variadic_stack_ptr.c` and
`/tmp/00204_ret_only.c` after the repaired publication plus same-module helper
/ local-byval acceptance closure. The next ownership is the remaining top-level
x86 route selection beyond this packet's publication and helper-prefix seams,
not another revisit of the old prepared call-bundle handoff gap.

## Watchouts

- The long-double helper-return family is still green:
  `/tmp/probe_hfa31.c`, `/tmp/probe_hfa32.c`, `/tmp/probe_hfa34.c`, and
  `/tmp/probe_hfa31_printf.c` remain the last confirmed reduced passes. Do not
  reopen that route unless a new reducer proves a separate `f128` regression.
- The focused publication proof is now
  `check_route_publishes_helper_same_module_variadic_stack_arg_before_call_bundle()`
  in `backend_x86_handoff_boundary_multi_defined_call_test.cpp`. Keep that test
  green when the next packet widens x86 route acceptance.
- `/tmp/probe_same_module_variadic_stack_ptr.c` is still the smallest reducer
  for the repaired producer seam, but it no longer points at prepared
  publication. It now points at the broader x86 same-module acceptance gate.
- `/tmp/00204_ret_only.c` hits the same broader top-level x86 rejection as the
  repaired stack-pointer reducer, so the next packet can stay on that common
  frontier instead of revisiting call-bundle publication.
- `backend_x86_handoff_boundary` passes with the new producer publication check,
  so there is no supported-path regression inside the owned files from this
  packet.

## Proof

Ran the delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'; } > test_after.log 2>&1`.
Current proof is mixed in the final workspace state: `backend_x86_handoff_boundary`
passes, while `c_testsuite_x86_backend_src_00204_c` still fails. The failing
top-level diagnostic is now:
`x86 backend emitter only supports a minimal single-block i32 return
terminator, a bounded equality-against-immediate guard family with immediate
return leaves including fixed-offset same-module global i32 loads and
pointer-backed same-module global roots, or one bounded compare-against-zero
branch family through the canonical prepared-module handoff`.
The canonical proof log remains `test_after.log`. Additional reducer proof for
this packet:

- `build/c4cll --codegen asm --target x86_64-unknown-linux-gnu /tmp/probe_same_module_variadic_stack_ptr.c`
  now reaches the same broader top-level x86 route rejection instead of the old
  authoritative prepared call-bundle handoff exception.
- `build/c4cll --codegen asm --target x86_64-unknown-linux-gnu /tmp/00204_ret_only.c`
  reaches that same broader top-level x86 route rejection.

Together, those results show the owned producer seam is repaired and the next
blocker sits later, outside this packet's owned files.
