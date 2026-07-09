Status: Active
Source Idea Path: ideas/open/639_pointer_loaded_from_global_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement one semantic authority packet

# Current Packet

## Just Finished

Step 3 implemented one semantic authority packet for the same-block
pointer-loaded-from-global shape. Prepared addressing now publishes and prints
`pointer_loaded_from_global_required=yes` plus a complete
`pointer_loaded_from_global` authority for the `src/pr46309.c` row
`%t15 = bir.load_global ptr @q` followed by
`%t16 = bir.load_local i32 %t16.addr, addr %t15`.

The fact carries pointer value `%t15`, producer `load_global` at
`block_1 inst=0`, source global `q`, pointer width/extent `8/8`, selected
local-memory use `block_1 inst=1 offset=0 width=4`, default producer/selected
address spaces, and explicit freshness. RV64 pointer-value base-plus-offset
consumption now requires that complete fact only when prepared marks a
same-block loaded-global pointer authority requirement. Non-loaded-global
pointer policies remain on their existing routes.

Focused tests cover an accepted same-block loaded-global pointer local-memory
access and a stale intervening global-store row that requires authority but
does not receive the complete fact.

## Suggested Next

Run Step 4 validation and lifecycle triage. The `src/pr46309.c` loaded-global
row now advances past the prior `block_1 inst=1` authority boundary, but the
supplemental allowlist still fails overall: `pr46309` reaches a later
local-memory owner, `pr58984` currently fails in call ABI/result lowering, and
`pr66556` still fails in local-memory ownership.

## Watchouts

- Keep direct `addr @symbol` local-memory rows under idea 631 and prepared
  global value-location rows under idea 621.
- Keep aggregate/byval/sret/stack-home rows under idea 633 and related ABI
  policy; `pr58984` and `pr66556` should not drive this packet.
- Missing producer, missing freshness, non-default address space, volatile
  access, incomplete global source identity, incomplete extent/width,
  ambiguous multiple producers, and stale cross-call/global publication remain
  fail-closed for loaded-global pointer rows.
- Do not infer loaded-pointer authority from source filenames, final symbol
  names, final assembly layout, register assignment, or BIR adjacency alone.

## Proof

Canonical proof passed:
`cmake --build --preset default --target backend_prepare_stack_layout_test -j1 && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`.

Supplemental probe command was run after a serial full-build retry to avoid a
parallel `cc1plus` resource kill:
`cmake --build --preset default && ALLOWLIST=build/agent_state/639_step1_pointer_loaded_from_global.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/639_step3_pointer_loaded_from_global.log 2>&1`.
The probe log is `build/agent_state/639_step3_pointer_loaded_from_global.log`
and reports `total=3 passed=0 failed=3`; the prepared authority evidence for
`pr46309` is captured at
`build/agent_state/639_step3_pointer_loaded_from_global/pr46309.dump-prepared-bir.txt`.
