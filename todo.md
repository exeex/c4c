Status: Active
Source Idea Path: ideas/open/639_pointer_loaded_from_global_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate and decide next lifecycle action

# Current Packet

## Just Finished

Step 4 re-ran the representative row probe and focused negative proof for idea
639 after Step 3 commit `f89def249`.

`src/pr46309.c` has the intended complete authority for the accepted row:
`%t15 = bir.load_global ptr @q` at `block_1 inst=0` followed by
`%t16 = bir.load_local i32 %t16.addr, addr %t15` at `block_1 inst=1`.
The current prepared dump prints
`pointer_loaded_from_global_required=yes pointer_loaded_from_global=yes` with
pointer `%t15`, producer `load_global`, source global `q`, pointer
width/extent `8/8`, selected use `block_1 inst=1 offset=0 width=4`, default
producer/selected address spaces, and `pointer_fresh=yes`. That confirms the
row moved past the prior `block_1 inst=1` loaded-global authority boundary.

The three-row probe still reports `total=3 passed=0 failed=3`, but the current
first owners are residuals:

- `src/pr46309.c`: still `unsupported_local_memory_access`, now after the
  accepted loaded-global row. Current prepared evidence also shows later
  ordinary pointer/local-memory and publication residuals, including
  `logic.rhs.22 inst=0` on pointer `%p.p` and `logic.end.25` join publication
  rows for `%t34` with `status=missing_publication`; this is not the prior
  loaded-global `%t15` authority gap.
- `src/pr58984.c`: `unsupported_call_abi` at `main entry block_index=0
  instruction_index=23`, callee `foo`, `args=1`, `planned_args=1`,
  `result=i32 %t7`. The prepared dump shows the call as
  `bir.call i32 foo(ptr byval(size=4, align=4) %lv.o)` plus aggregate/byval
  parameter-copy local-memory setup, so this remains ABI/byval work outside
  idea 639.
- `src/pr66556.c`: `unsupported_local_memory_access`. The prepared dump has
  loaded pointer globals such as `@k` and `@f`, but the visible residuals are
  mixed aggregate/global bitfield and ordinary pointer/local-memory ownership:
  `load_local ptr %lv.n`, `access block=logic.end.7 inst_index=16
  base=pointer_value pointer=%t23 size=2`, global byte-storage aggregate
  accesses, and direct-global select-chain/store-source rows. It remains
  outside the accepted Step 3 packet.

Negative ownership remains intact: direct global-symbol rows stay with idea
631, prepared global value-location rows stay with idea 621,
aggregate/byval/stack-home rows stay with idea 633 and related ABI policy,
string-constant local memory stays outside idea 639, and the focused stale
loaded-global test still proves missing freshness fails closed.

## Suggested Next

Recommend plan-owner closure for idea 639: the source acceptance criterion is
satisfied by one complete-authority pointer-loaded-from-global row moving past
its previous owner, while the remaining representative failures classify to
other existing policy/ABI owners. Continue residual work under the appropriate
follow-up ideas instead of widening idea 639.

## Watchouts

- The required row-probe command fails overall because all three
  representative source files still fail; treat that as residual
  classification, not as a Step 3 authority regression.
- The allowlist helper does not refresh `dump-prepared-bir` artifacts; current
  Step 4 dumps were regenerated under `build/agent_state/639_step4_validation/`.
- Do not use `pr58984` byval/aggregate setup or `pr66556` aggregate/global
  bitfield local-memory residuals to widen pointer-loaded-from-global policy.
- Preserve the fail-closed requirements for missing producer, missing
  freshness, non-default address space, volatile access, incomplete source
  identity, incomplete extent/width, ambiguous producers, and stale
  publication.

## Proof

Required proof command:
`cmake --build --preset default && ALLOWLIST=build/agent_state/639_step1_pointer_loaded_from_global.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1`.
Result: build succeeded; row probe exited nonzero with `total=3 passed=0
failed=3`, preserving root `test_after.log`.

Focused proof command:
`cmake --build --preset default --target backend_prepare_stack_layout_test -j1 && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$' > build/agent_state/639_step4_backend_prepare_stack_layout.log 2>&1`.
Result: passed, `100% tests passed, 0 tests failed out of 1`.

Current Step 4 evidence:
`build/agent_state/639_step4_validation/representative_rows.md`,
`build/agent_state/639_step4_validation/pr46309.dump-prepared-bir.txt`,
`build/agent_state/639_step4_validation/pr58984.dump-prepared-bir.txt`, and
`build/agent_state/639_step4_validation/pr66556.dump-prepared-bir.txt`.
