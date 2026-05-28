Status: Active
Source Idea Path: ideas/open/58_aarch64_prepared_authority_regression_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Re-Inspect The `78730af2f` Boundary

# Current Packet

## Just Finished

Completed idea 58 Step 2 re-inspection of the `78730af2f` boundary for the
remaining `00196` failure. The boundary commit itself is
`78730af2f15da3f272aaa7d138bffd886908cbd0` (`repair inline va_arg
register-save publication`), touching AArch64
`dispatch_edge_copies.cpp` and `memory.cpp`; its preserved baseline log records
`c_testsuite_aarch64_backend_src_00196_c` as failed.

Current semantic evidence points to block-entry out-of-SSA edge publication for
select/phi carriers across call boundaries, not generic short-circuit CFG,
boolean comparison, load-local, or ALU publication:

- Semantic BIR for `joe() && fred()` is correct:
  `%t31 = call fred`, `%t32 = ne %t31, 0`, `%t34 = phi
  [logic.rhs.end.29, %t32] [logic.skip.28, 0]`.
- Prepared BIR keeps that ownership as `join_transfer logic.end.30
  result=%t34 kind=phi_edge carrier=select_materialization`, with
  `parallel_copy logic.rhs.end.29 -> logic.end.30` moving `%t32 -> %t34`.
- The generated assembly executes the RHS call, then the join-copy path
  overwrites the RHS call result with the left operand's register:
  after `bl fred`, it has `mov x21, x0`, then at the merge copy emits
  `mov x21, x13; mov w13, w21; cmp w13, #0; cset w13, ne`.
  `x13` is a caller-saved register also listed as clobbered by the callsite, so
  this rematerializes from a stale/clobbered left-operand home instead of the
  RHS result `%t32`.
- The same shape explains the other remaining mismatch,
  `joe() && (0 + fred())`: prepared BIR shows `%t83` from the RHS call/add/ne
  feeding `%t85`, while assembly after the RHS call recomputes through the
  left-operand/caller-saved path and prints `1`.

## Suggested Next

Next packet: Step 3 should repair the AArch64 block-entry
out-of-SSA/edge-publication consumer for select-materialization join carriers
when the source value is produced in the predecessor by a call-adjacent
comparison or ALU chain. The repair should make
`lower_predecessor_select_parallel_copy_sources` /
`lower_predecessor_join_source_publication` consume the prepared
edge-publication source producer/home for the incoming edge, avoiding
rematerialization from caller-saved homes clobbered by the RHS call.

Focused proof targets for that repair:

- `c_testsuite_aarch64_backend_src_00196_c` for the observed runtime mismatch.
- `c_testsuite_aarch64_backend_src_00033_c` for nearby short-circuit side
  effects in control flow.
- `c_testsuite_aarch64_backend_src_00164_c` for boolean operator result
  composition.
- `c_testsuite_aarch64_backend_src_00207_c` as the must-stay-green
  short-circuit/conditional-control-flow guard.
- `c_testsuite_aarch64_backend_src_00208_c` for short-circuit `||` with calls
  and local stack objects.

## Watchouts

- Do not reopen closed idea 60 seams unless the `00196` investigation proves a
  shared semantic owner.
- Do not key behavior to `00196`, exact temporary names, literal labels, or
  fixed stack offsets.
- Keep dispatch, dynamic-stack fixed-slot FP anchoring, and `00207` green while
  repairing the remaining family.
- The remaining observed shape is specifically an incoming-edge value
  publication problem when the RHS call/comparison result must feed the boolean
  result and the left operand lives in a caller-saved register that the RHS call
  can clobber.
- Do not repair this by hard-coding `00196`, `joe`, `fred`, `x13`, `%t34`,
  `%t85`, or any specific label. The semantic owner is the prepared
  out-of-SSA edge publication path for select-materialization carriers.
- `c_testsuite_aarch64_backend_src_00207_c` is both a must-stay-green target
  and a same-feature guard for constant short-circuit behavior.

## Proof

No fresh build or test was required for this investigation-only packet, and
`test_after.log` was not modified. Existing proof cited from `test_after.log`:

```sh
(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor|c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00207_c)$') > test_after.log 2>&1
```

Recorded result: build passed. `backend_aarch64_instruction_dispatch`,
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`, and
`c_testsuite_aarch64_backend_src_00207_c` passed.
`c_testsuite_aarch64_backend_src_00196_c` failed with the known runtime
mismatch recorded above. Read-only investigation commands used:

```sh
git show --stat --oneline 78730af2f15da3f272aaa7d138bffd886908cbd0
git show --format= 78730af2f15da3f272aaa7d138bffd886908cbd0 -- src/backend/mir/aarch64/codegen/memory.cpp src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp
./build/c4cll --dump-bir --target aarch64-unknown-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00196.c
./build/c4cll --dump-prepared-bir --target aarch64-unknown-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00196.c
sed -n '1,330p' build/c_testsuite_aarch64_backend/src/00196.c.s
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp build/compile_commands.json
```

`test_after.log` and
`log/baseline_78730af2f15da3f272aaa7d138bffd886908cbd0.log` remain the cited
proof logs.
