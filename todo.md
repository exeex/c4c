# Backend Reboot From BIR Spine

Status: Active
Source Idea: ideas/open/46_backend_reboot_bir_spine.md
Source Plan: plan.md

## Current Active Item

- active route:
  semantic BIR is the new truth surface, `prepare` owns target legality, and
  target backends should eventually ingest prepared BIR only
- runbook repair:
  `plan.md` now carries the ordered remaining capability backlog so future
  executor packets can be chosen from a durable semantic queue rather than
  rediscovering the next testcase family from chat
- review checkpoint:
  a route audit judged the code path still aligned to the source idea but
  called for a `todo.md` packet reset before more execution because the first
  indirect-call slice was already complete
- current capability family:
  backlog item 5 call-lane widening on the riscv64 backend-route surface:
  indirect calls now carry the first mixed integer-class arg family, the
  first ptr-capable result family, and the first combined ptr-arg plus
  ptr-result family through semantic BIR and native asm; the first adjacent
  non-leading single-ptr arg family and the first adjacent two-ptr arg family
  are now also green, and the first adjacent non-leading mixed three-arg
  `i32, ptr, ptr` family is now green as well; the next packet should widen
  further across adjacent mixed multi-`ptr` signatures without reopening
  stack-call ABI work
- current packet shape:
  keep backlog item 5 moving outward from the repaired entry boundary:
  widen the standing riscv64 indirect-call lane from the now-green leading,
  trailing, paired-leading, and first non-leading mixed three-arg ptr
  signatures to the next adjacent multi-ptr-capable integer-class family
  while keeping the existing eight-register arg lane fixed and avoiding
  ABI-shaped stack-call work
- candidate proving surface:
  add the next paired param/local riscv64 route tests for a multi-`ptr`
  integer-class indirect-call signature with at least one non-leading `ptr`
  that still fits inside the current eight-register lane
  keep `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, and the current
  one-arg through eight-arg indirect-call plus `two_arg_*` direct-call route
  tests as standing sentinels while backlog item 5 widens through adjacent
  honest integer-class indirect-call signatures

## Immediate Target

- keep packet selection attached to the ordered semantic backlog in `plan.md`
- carry the now-green addressed-global work forward by moving to the next
  semantic family instead of stretching backlog item 4 past its proving surface
- carry the now-green one-arg through six-arg indirect-call surface forward
  by widening adjacent ptr-capable integer-class call signatures and results,
  now including the first non-leading single-ptr arg family, instead of
  reopening richer direct-call metadata or jumping ahead to ABI-shaped call
  work
- avoid reintroducing testcase-shaped routing while broadening the shared
  entry-signature and call lanes

## Done Condition For The Active Packet

- `branch_if_eq.c` still lowers to clean BIR
- the existing direct-call sentinels, one-arg indirect-call tests, and
  rewrite-only two-arg route tests stay green on riscv64
- simple param-carried and local-slot indirect helper wrappers whose callee
  signature includes at least one `ptr` arg or returns `ptr` still lower
  through the same riscv64 backend-route surface instead of reopening
  host-runtime x86 fallback or jumping ahead to ABI-shaped call work
- semantic call lowering keeps callee identity, result type, and minimal arg
  metadata available for later prepare/ABI shaping without performing that
  shaping inside `lir_to_bir`
- the widened call-lane route proofs still cover internal helper-shaped bodies
  without introducing target-specific shortcuts
- no new direct route, rendered-text matcher, or tiny case-family special path
  is introduced

## Latest Packet Progress

- completed:
  the first honest non-leading mixed three-arg ptr family now stays on the
  same shared semantic-BIR/prepared-BIR riscv64 route surface as the earlier
  single-ptr, two-ptr, and ptr-result work: no backend route rewrite was
  needed because the existing integer-class riscv64 arg lane already treats
  adjacent `ptr` values like the neighboring `i32` lane regardless of
  ordinal, so helper wrappers with an `i32, int *, int * -> i32` callee
  signature lower through the standing eight-register indirect-call lane
  without reopening fallback routes or ABI-shaped stack-call work
  new route proofs cover `indirect_i32_two_ptr_arg_param_call.c` and
  `indirect_i32_two_ptr_arg_local_call.c` as native asm with the expected
  callee preserve into `t0`, `i32` arg move into `a0`, adjacent `ptr` arg
  moves into `a1` and `a2`, and final `jalr ra, t0, 0`, while
  `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, and the earlier
  one-arg through eight-arg indirect-call plus `ptr`-shaped and `two_arg_*`
  direct-call sentinels stayed green beside them
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the two new riscv64 route tests passed, the broad `^backend_` subset stayed
  flat at `225` failing tests before and after, the total backend test surface
  increased from `363` to `365`, and supervisor-side regression guard passed
  with `passed=138 -> 140`, `failed=225 -> 225`, and `0` new failing tests
  this keeps backlog item 5 moving on an honest shared integer-class
  indirect-call lane by proving the first adjacent non-leading mixed
  three-arg multi-`ptr` family without introducing route-specific shortcuts
- completed:
  the first honest two-ptr indirect-call family now stays on the same shared
  semantic-BIR/prepared-BIR riscv64 route surface as the earlier single-ptr
  arg and ptr-result work: no backend route rewrite was needed because the
  existing integer-class riscv64 arg lane already treats multiple `ptr`
  values like adjacent integer-class args, so helper wrappers with an
  `int *, int * -> i32` callee signature lower through the standing
  eight-register indirect-call lane without reopening fallback routes or
  ABI-shaped stack-call work
  new route proofs cover `indirect_two_ptr_arg_param_call.c` and
  `indirect_two_ptr_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, paired `ptr` arg moves into `a0` and `a1`, and final
  `jalr ra, t0, 0`, while `branch_if_eq.c`, `call_helper.c`,
  `local_arg_call.c`, and the earlier one-arg through eight-arg indirect-call
  plus `ptr`-shaped and `two_arg_*` direct-call sentinels stayed green beside
  them
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the two new riscv64 route tests passed, the broad `^backend_` subset stayed
  flat at `225` failing tests before and after, the total backend test surface
  increased from `361` to `363`, and supervisor-side regression guard passed
  with `passed=136 -> 138`, `failed=225 -> 225`, and `0` new failing tests
  this keeps backlog item 5 moving on an honest shared integer-class
  indirect-call lane by proving adjacent multi-`ptr` arg support without
  introducing route-specific shortcuts
- completed:
  the first honest non-leading ptr-arg indirect-call family now stays on the
  same shared semantic-BIR/prepared-BIR riscv64 route surface as the earlier
  leading-ptr and ptr-result work: no backend route rewrite was needed because
  the existing integer-class riscv64 arg lane already treats `ptr` like `i32`
  regardless of argument ordinal, so helper wrappers with an `i32, ptr -> i32`
  callee signature lower through the standing eight-register indirect-call lane
  without reopening fallback routes or ABI-shaped stack-call work
  new route proofs cover `indirect_i32_ptr_arg_param_call.c` and
  `indirect_i32_ptr_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, `i32` arg move into `a0`, trailing `ptr` arg move into
  `a1`, and final `jalr ra, t0, 0`, while `branch_if_eq.c`,
  `call_helper.c`, `local_arg_call.c`, and the earlier one-arg through
  eight-arg indirect-call plus `ptr`-shaped and `two_arg_*` direct-call
  sentinels stayed green beside them
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the two new riscv64 route tests passed, the broad `^backend_` subset stayed
  flat at `225` failing tests before and after, the total backend test surface
  increased from `359` to `361`, and local before/after comparison passed with
  `passed=134 -> 136`, `failed=225 -> 225`, and `0` new failing tests
  this keeps backlog item 5 moving on an honest shared integer-class
  indirect-call lane by proving `ptr` support outside the already-covered
  leading-arg shapes without introducing route-specific shortcuts
- completed:
  the first honest mixed ptr-arg plus ptr-result indirect-call family now
  stays on the same shared semantic-BIR/prepared-BIR riscv64 route surface as
  the earlier ptr-arg-only and ptr-result-only work: no backend route rewrite
  was needed because the existing integer-class riscv64 arg lane already
  treated `ptr` like `i32` for args while the current call-result and
  function-return lane already kept `ptr` in `a0`
  new route proofs cover `indirect_ptr_arg_ptr_return_param_call.c` and
  `indirect_ptr_arg_ptr_return_local_call.c` as native asm with the expected
  callee preserve into `t0`, ptr arg move into `a0`, trailing `i32` arg move
  into `a1`, final `jalr ra, t0, 0`, and pointer result returned through
  `a0`, while `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, and the
  earlier one-arg through eight-arg indirect-call plus `two_arg_*` direct-call
  sentinels stayed green beside them
  proof command attempted:
  `bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1'`
  proof log:
  `test_after.log`
  proof status:
  the two new riscv64 route tests passed, the broad `^backend_` subset stayed
  flat at `225` failing tests before and after, the total backend test surface
  increased from `357` to `359`, and supervisor-side regression guard passed
  with `passed=132 -> 134`, `failed=225 -> 225`, and `0` new failing tests
  this keeps backlog item 5 on an honest semantic integer-class indirect-call
  lane while widening from single-dimension ptr coverage to the first mixed
  ptr-arg plus ptr-result family without reopening direct routes or
  ABI-shaped stack-call work
- completed:
  the first honest ptr-return indirect-call family now stays on the same
  shared semantic-BIR/prepared-BIR riscv64 route surface as the earlier
  i32-return indirect-call work: `backend.cpp` now treats `ptr` like `i32`
  for the current riscv64 call-result and function-return lane, so indirect
  calls can return the first `ptr` result shapes through native asm without
  reopening fallback routes or smuggling stack-call ABI work into lowering
  new route proofs cover `indirect_ptr_return_param_call.c` and
  `indirect_ptr_return_local_call.c` as native asm with the expected callee
  preserve into `t0`, ptr arg move into `a0`, direct `jalr ra, t0, 0`, and
  pointer result returned through `a0`, while `branch_if_eq.c`,
  `call_helper.c`, `local_arg_call.c`, and the earlier one-arg through
  eight-arg indirect-call plus `two_arg_*` direct-call sentinels stayed green
  beside them
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the two new riscv64 route tests passed, the broad `^backend_` subset stayed
  flat at `225` failing tests before and after, the total backend test surface
  increased from `355` to `357`, and local before/after comparison passed with
  `passed=130 -> 132`, `failed=225 -> 225`, and `0` new failing tests
  this keeps backlog item 5 on an honest semantic integer-class indirect-call
  lane while widening from ptr args into ptr results without reopening direct
  routes or ABI-shaped stack-call work
- completed:
  the first honest ptr-capable indirect-call family now stays on the same
  shared semantic-BIR/prepared-BIR riscv64 route surface as the earlier
  i32-only indirect-call work: `backend.cpp` now treats `ptr` like `i32` for
  the current integer-class riscv64 arg lane, so indirect calls can move the
  first mixed `ptr, i32 -> i32` signature through the existing eight-register
  arg scheduler without reopening ABI-shaped stack-call work
  new route proofs cover `indirect_ptr_arg_param_call.c` and
  `indirect_ptr_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, ptr arg move into `a0`, trailing `i32` arg move into
  `a1`, and final `jalr ra, t0, 0`, while `branch_if_eq.c`,
  `call_helper.c`, `local_arg_call.c`, and the earlier one-arg through
  eight-arg indirect-call plus `two_arg_*` direct-call sentinels stayed green
  beside them
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the two new riscv64 route tests passed, the broad `^backend_` subset stayed
  flat at `225` failing tests before and after, the total backend test surface
  increased from `353` to `355`, and local before/after comparison passed with
  `passed=128 -> 130`, `failed=225 -> 225`, and `0` new failing tests
  this resumes backlog item 5 on an honest semantic integer-class call lane
  without reintroducing direct-route fallbacks or smuggling ABI work into call
  lowering
- completed:
  the first honest ten-parameter indirect-call proving family is now green on
  the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier one-stack-slot entry work: the existing backend entry-state mapping
  already kept multiple caller-stack params addressable through
  `value_stack_offsets`, so no backend route rewrite was needed to carry two
  stack-passed incoming `i32` values into the standing eight-register
  indirect-call lane
  new route proofs cover `indirect_eight_arg_two_stack_param_call.c` and
  `indirect_eight_arg_two_stack_local_call.c` as native asm with the expected
  callee preserve into `t0`, arg rewrites from `a2/a3/a4/a5/a6/a7` into
  `a0/a1/a2/a3/a4/a5`, both incoming caller-stack reloads via
  `lw a6, 16(sp)` and `lw a7, 24(sp)`, and final `jalr ra, t0, 0`, while
  `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, and the earlier
  one-arg through eight-arg indirect-call plus `two_arg_*` direct-call
  sentinels stayed green beside them
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the two new riscv64 route tests passed, the broad `^backend_` subset stayed
  flat at `225` failing tests before and after, the total backend test surface
  increased from `351` to `353`, and supervisor-side regression guard passed
  with `passed=126 -> 128`, `failed=225 -> 225`, and `0` new failing tests
  this confirms the next adjacent entry-signature seam without reopening
  fallback paths, ABI-shaped lowering, or testcase-shaped route logic
- completed:
  the first honest nine-parameter indirect-call family now stays on the same
  shared semantic-BIR/prepared-BIR route surface as the earlier one-arg
  through eight-arg indirect-call work: `backend.cpp` now keeps incoming
  riscv64 integer and pointer params beyond `a0` through `a7` addressable
  through their caller-stack slots instead of rejecting the whole function
  body, and the existing shared indirect-call arg move lane can now reload the
  ninth incoming `i32` directly into `a7` when forming the call
  new route proofs cover `indirect_eight_arg_stack_param_call.c` and
  `indirect_eight_arg_stack_local_call.c` as native asm with the expected
  callee preserve into `t0`, arg rewrites from `a1/a2/a3/a4/a5/a6/a7` into
  `a0/a1/a2/a3/a4/a5/a6`, the final incoming stack arg reload via
  `lw a7, 16(sp)`, and final `jalr ra, t0, 0`, while
  `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, and the earlier
  one-arg through eight-arg indirect-call plus `two_arg_*` direct-call
  sentinels stayed green beside them
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the two new riscv64 route tests passed, the broad `^backend_` subset stayed
  flat at `225` failing tests before and after, and the total backend test
  surface increased from `349` to `351` with no new failing tests
  this repairs the real route boundary that blocked further call widening
  without reopening fallback seams or smuggling ABI-shaped metadata into
  semantic lowering
- completed:
  the first honest eight-arg indirect-call family now stays on the same shared
  semantic-BIR/prepared-BIR route surface as the earlier one-arg through
  seven-arg indirect-call work: `backend.cpp` now treats `a7` as part of the
  supported outgoing riscv64 integer-call register set, so the existing shared
  call-arg move scheduler and indirect-callee preserve logic can lower the
  first `i32, i32, i32, i32, i32, i32, i32, i32 -> i32` indirect family
  without reopening fallback seams or adding arg-count-shaped special handling
  while staying inside the current eight-register function-entry lane by using
  seven carried args plus one immediate final arg instead of forcing a
  separate nine-parameter signature packet first
  new route proofs cover `indirect_eight_arg_param_call.c` and
  `indirect_eight_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, arg rewrites from `a1/a2/a3/a4/a5/a6/a7` into
  `a0/a1/a2/a3/a4/a5/a6`, an immediate load into `a7`, and final
  `jalr ra, t0, 0`, while
  `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, the earlier one-arg
  through seven-arg indirect-call tests, and the `two_arg_*` direct-call
  sentinels stayed green beside them
  proof command attempted:
  `bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R ^backend_ >> test_after.log 2>&1'`
  proof log:
  `test_after.log`
  proof status:
  first attempted route proofs exposed a real packet boundary: param-carried
  and local-slot callee variants with eight carried integer args create a
  nine-parameter function signature and still fall back before the widened
  outgoing-call lane runs, so the packet was repaired in `todo.md` to keep the
  first eight-arg indirect slice inside the existing entry-signature contract
  the repaired `^backend_` proof then passed the new
  `indirect_eight_arg_param_call.c` and `indirect_eight_arg_local_call.c`
  route tests, kept the standing one-arg through seven-arg indirect-call and
  `two_arg_*` direct-call sentinels green, and remained blocked only by
  pre-existing non-owned backend failures outside this packet
  supervisor-side regression guard against `test_before.log` and
  `test_after.log` passed with `passed=122 -> 124`, `failed=225 -> 225`, and
  `0` new failing tests
  this keeps backlog item 5 on the honest semantic-BIR/prepared-BIR riscv64
  path for the first eight-arg indirect family without reopening host-runtime
  x86 fallback seams or widening into ABI-shaped call metadata
- completed:
  the first honest seven-arg indirect-call family now stays on the same shared
  semantic-BIR/prepared-BIR route surface as the earlier one-arg through
  six-arg indirect-call work: `backend.cpp` now treats `a6` as part of the
  supported outgoing riscv64 integer-call register set, so the existing shared
  call-arg move scheduler and indirect-callee preserve logic can lower the
  first `i32, i32, i32, i32, i32, i32, i32 -> i32` indirect family without
  reopening fallback seams or adding arg-count-shaped special handling
  new route proofs cover `indirect_seven_arg_param_call.c` and
  `indirect_seven_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, arg rewrites from `a1/a2/a3/a4/a5/a6/a7` into
  `a0/a1/a2/a3/a4/a5/a6`, and final `jalr ra, t0, 0`, while
  `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, the earlier one-arg
  through six-arg indirect-call tests, and the `two_arg_*` direct-call
  sentinels stayed green beside them
  proof command attempted:
  `bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R ^backend_ >> test_after.log 2>&1'`
  proof log:
  `test_after.log`
  proof status:
  the delegated `^backend_` command preserved this slice's new route proofs and
  standing route sentinels, but the overall subset remains blocked by many
  pre-existing non-owned backend failures outside this packet
  this keeps backlog item 5 on the honest semantic-BIR/prepared-BIR riscv64
  path for the first seven-arg indirect family without reopening host-runtime
  x86 fallback seams or widening into ABI-shaped call metadata
- completed:
  the first honest six-arg indirect-call family now stays on the same shared
  semantic-BIR/prepared-BIR route surface as the earlier one-arg through
  five-arg indirect-call work: `backend.cpp` now treats `a5` as part of the
  supported outgoing riscv64 integer-call register set, so the existing shared
  call-arg move scheduler and indirect-callee preserve logic can lower the
  first `i32, i32, i32, i32, i32, i32 -> i32` indirect family without
  reopening fallback seams or adding arg-count-shaped special handling
  new route proofs cover `indirect_six_arg_param_call.c` and
  `indirect_six_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, arg rewrites from `a1/a2/a3/a4/a5/a6` into
  `a0/a1/a2/a3/a4/a5`, and final `jalr ra, t0, 0`, while `branch_if_eq.c`,
  `call_helper.c`, `local_arg_call.c`, the earlier one-arg through five-arg
  indirect-call tests, and the `two_arg_*` direct-call sentinels stayed green
  beside them
  proof command attempted:
  `bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R ^backend_ >> test_after.log 2>&1'`
  proof log:
  `test_after.log`
  proof status:
  the delegated `^backend_` command preserved this slice's new route proofs and
  standing route sentinels, but the overall subset remains blocked by many
  pre-existing non-owned backend failures outside this packet
  this keeps backlog item 5 on the honest semantic-BIR/prepared-BIR riscv64
  path for the first six-arg indirect family without reopening host-runtime
  x86 fallback seams or widening into ABI-shaped call metadata
- completed:
  the first honest five-arg indirect-call family now stays on the same shared
  semantic-BIR/prepared-BIR route surface as the earlier one-arg through
  four-arg indirect-call work: `backend.cpp` now treats `a4` as part of the
  supported outgoing riscv64 integer-call register set, so the existing shared
  call-arg move scheduler and indirect-callee preserve logic can lower the
  first `i32, i32, i32, i32, i32 -> i32` indirect family without reopening
  fallback seams or adding arg-count-shaped special handling
  new route proofs cover `indirect_five_arg_param_call.c` and
  `indirect_five_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, arg rewrites from `a1/a2/a3/a4/a5` into
  `a0/a1/a2/a3/a4`, and final `jalr ra, t0, 0`, while `branch_if_eq.c`,
  `call_helper.c`, `local_arg_call.c`, the earlier one-arg through four-arg
  indirect-call tests, and the `two_arg_*` direct-call sentinels stayed green
  beside them
  this keeps backlog item 5 on the honest semantic-BIR/prepared-BIR riscv64
  path for the first five-arg indirect family without reopening host-runtime
  x86 fallback seams or widening into ABI-shaped call metadata
- completed:
  the first honest four-arg indirect-call family now stays on the same shared
  semantic-BIR/prepared-BIR route surface as the earlier one-arg through
  three-arg indirect-call work: `backend.cpp` now treats `a3` as part of the
  supported outgoing riscv64 integer-call register set, so the existing shared
  call-arg move scheduler and indirect-callee preserve logic can lower the
  first `i32, i32, i32, i32 -> i32` indirect family without reopening fallback
  seams or adding arg-count-shaped special handling
  new route proofs cover `indirect_four_arg_param_call.c` and
  `indirect_four_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, arg rewrites from `a1/a2/a3/a4` into `a0/a1/a2/a3`, and
  final `jalr ra, t0, 0`, while `branch_if_eq.c`, `call_helper.c`,
  `local_arg_call.c`, the earlier one-arg through three-arg indirect-call
  tests, and the `two_arg_*` direct-call sentinels stayed green beside them
  this keeps backlog item 5 on the honest semantic-BIR/prepared-BIR riscv64
  path for the first four-arg indirect family without reopening host-runtime
  x86 fallback seams or widening into ABI-shaped call metadata
- completed:
  the first honest three-arg indirect-call family now stays on the same shared
  semantic-BIR/prepared-BIR route surface as the earlier one-arg and two-arg
  indirect-call work: `backend.cpp` now treats `a2` as part of the supported
  outgoing riscv64 integer-call register set, preserves indirect callees
  against any live outgoing arg register they would otherwise be rewritten by,
  and resolves the call-arg register rewrites with one shared move schedule
  instead of the previous two-register-only cases
  new route proofs cover `indirect_three_arg_param_call.c` and
  `indirect_three_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, arg rewrites from `a1/a2/a3` into `a0/a1/a2`, and final
  `jalr ra, t0, 0`, while `branch_if_eq.c`, `call_helper.c`,
  `local_arg_call.c`, the earlier one-arg indirect-call tests, the landed
  two-arg indirect-call tests, and the `two_arg_*` direct-call sentinels
  stayed green beside them
  this keeps backlog item 5 on the honest semantic-BIR/prepared-BIR riscv64
  path for the first three-arg indirect family without reopening host-runtime
  x86 fallback seams or widening into ABI-shaped call metadata
- completed:
  the first honest riscv64 indirect-call lane now stays on the same shared
  semantic-BIR/prepared-BIR route surface as the earlier direct-call work:
  `lir_to_bir_module.cpp` lowers SSA-callee one-arg indirect calls as
  `bir::CallInst{is_indirect = true, callee_value = ...}` instead of rejecting
  them, `bir_printer.cpp` and `bir_validate.cpp` accept that honest indirect
  call form, and `backend.cpp` now preserves ptr params plus ptr local slots
  well enough to emit native riscv64 `jalr` for the first `i32 -> i32`
  indirect-call family without reintroducing fallback seams
  new riscv64 route proofs now cover `indirect_param_call.c` and
  `indirect_local_call.c` as native asm with the expected ptr-callee preserve,
  arg move into `a0`, and final `jalr ra, ..., 0`, while
  `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, and the existing
  `two_arg_*` direct-call route tests stayed green beside them
  this moves backlog item 5 beyond the exhausted rewrite-only direct-call
  surface and into a real indirect-call lane on the honest riscv64 route
  boundary without adding testcase-shaped dispatch or reopening the rejected
  host-runtime x86 fallback seam
- completed:
  pointer-valued stores through addressed pointer-global slots now stay on the
  semantic BIR route: when a pointer load preserves a pointer-global object
  address such as `@gp`, later `store ptr` through that alias lowers as
  `bir.store_global @gp, ptr %...` instead of dropping to raw LLVM IR
  pointer-global loads now preserve both the pointer-global object alias chain
  and the current known addressed-global pointee after an in-function pointer
  store, so a later `load ptr, ptr @gp` reuses the just-stored addressed-global
  target rather than the static initializer
  new riscv64 route proofs now cover `*ggp = gq; return gp[1];` and
  `**gggp = gq; return gp[1];` through semantic BIR as explicit
  `bir.store_global @gp, ptr %...` plus the post-store addressed-global reload
  from `@arr, offset 8` rather than raw LLVM fallback
  `bir.store_global` now prints non-zero byte offsets, so BIR route proofs can
  distinguish addressed writes from base-global writes instead of hiding the
  semantic target slot in the textual proof surface
  new riscv64 route proofs now cover `gp[1] = 9; return arr[1];`,
  `(*ggp)[1] = 9; return arr[2];`, and `gp[1] = 'o'; return g_text[1];`
  through semantic BIR as offset-preserving addressed-global stores plus
  matching addressed-global reloads rather than raw LLVM fallback
  preserved pointer-global object addresses as addressable globals instead of
  collapsing them into their pointee address too early; a new riscv64 route
  proof now covers `int **ggp = &gp; return (*ggp)[1];` through semantic BIR as
  `bir.load_global ptr @ggp`, then `bir.load_global ptr @gp`, then
  `bir.load_global i32 @arr, offset 8` rather than raw LLVM fallback
  corrected recursive pointer-global address resolution so constant-offset
  aliases scale through the resolved pointee stride instead of preserving the
  frontend's `getelementptr inbounds (i8, ptr @gp, i64 N)` byte-gep artifact;
  `defined_pointer_global_pointer_offset.c` now proves
  `int *gp = &arr[1]; int *gpp = gp + 1; return gpp[1];` lowers through BIR as
  `bir.load_global ptr @gpp` followed by `bir.load_global i32 @arr, offset 12`
  on the riscv64 route surface rather than the incorrect byte-offset result
  pointer-global object addresses now survive `ptrtoint` / `inttoptr`
  roundtrips instead of only already-resolved addressed-global data pointers;
  the strengthened `global_int_pointer_roundtrip.c` route proof now covers
  `(**(int ***)(long)&ggp)[1]` through semantic BIR as `bir.load_global ptr
  @ggp`, then `bir.load_global ptr @gp`, then `bir.load_global i32 @arr,
  offset 4` rather than falling back once the cast roundtrip preserved the
  intermediate pointer-global object address
  string-backed addressed globals now use the same extra pointer-global
  indirection lane as integer arrays; new riscv64 route proofs cover
  `char **ggp = &gp; return (*ggp)[1];` and `(*ggp)[1] = 'o'; return g_text[1];`
  through semantic BIR as `bir.load_global ptr @ggp`, then
  `bir.load_global ptr @gp`, then addressed `bir.load_global i8 @g_text,
  offset 1` / `bir.store_global @g_text, offset 1, i8 ...` rather than
  dropping to raw LLVM fallback
  pointer-global object-address roundtrip stores were already semantically
  supported; new riscv64 route proofs now cover `*(int **)(long)&gp = gq;
  return gp[0];` and `**(int ***)(long)&ggp = gq; return gp[1];` through
  semantic BIR as `bir.store_global @gp, ptr ...` followed by the later
  `bir.load_global ptr @gp` and addressed `bir.load_global i32 @arr, offset 4`
  / `offset 8` reloads rather than raw LLVM fallback
  direct addressed stores on nested integer-array globals now stay on the
  semantic BIR route too; new riscv64 route proofs cover `arr[1][0] = 9;
  return arr[1][0];`, `ext_arr[1][0] = 9; return ext_arr[1][0];`, and
  `int *p = &arr[0][0]; p[2] = 9; return arr[1][0];` as explicit
  `bir.store_global @arr/@ext_arr, offset 8, i32 9` plus matching addressed
  reloads rather than raw LLVM fallback
  simple struct-backed globals now stay on the semantic BIR route too; new
  riscv64 route proofs cover direct scalar field stores on `v.x` / `v.y`,
  anonymous and named designated-init scalar field reads at `@s` offsets 0/4/8,
  and a designated pointer field read from `struct S { int a; int *p; }`
  through semantic BIR as `bir.load_global ptr @s, offset 8` followed by
  `bir.load_global i32 @x` rather than dropping back to LLVM-text fallback
  aggregate global lowering now decodes simple struct layouts from LIR type
  declarations, preserves honest byte offsets for struct-field GEPs, and keeps
  pointer-field global initializers as named pointer slots so the first
  dereference can recover the addressed global's scalar lane instead of losing
  alias information at the aggregate boundary
  addressed pointer-field stores inside struct-backed globals now preserve
  runtime alias information too; new riscv64 route proofs cover both
  `s.p = gp; return *s.p;` and `int **pp = &s.p; *pp = gp; return **pp;`
  through semantic BIR as `bir.store_global @s, offset 8, ptr %...` followed
  by the later `bir.load_global ptr @s, offset 8` and addressed
  `bir.load_global i32 @y` instead of falling back to the static initializer
  or raw LLVM output
  nested aggregate GEPs rooted in addressed globals now stay on the semantic
  BIR route too; relative addressed-global GEP lowering no longer stops after
  one scalar-style index, so new riscv64 route proofs cover
  `return s.inner.xs[2];`, `s.inner.xs[1] = 9; return s.inner.xs[1];`, and
  `int *p = &s.inner.xs[0]; p[2] = 9; return s.inner.xs[2];` as direct
  `bir.load_global i32 @s, offset 8` plus addressed
  `bir.store_global @s, offset 4/8, i32 9` rather than raw LLVM fallback
  nested addressed-global pointer fields now have explicit route coverage too;
  new riscv64 route proofs cover `return *s.inner.p;`, `s.inner.p = gp;
  return *s.inner.p;`, and `int **pp = &s.inner.p; *pp = gp; return **pp;`
  through semantic BIR as `bir.load_global ptr @s, offset 8` /
  `bir.store_global @s, offset 8, ptr ...` followed by the later
  `bir.load_global i32 @x/@y` rather than raw LLVM fallback
  multi-step addressed-global pointer-field rewrites now have explicit route
  coverage too; new riscv64 route proofs cover `s.p = gp; *pp = gq; return
  *s.p;` and `s.inner.p = gp; *pp = gq; return *s.inner.p;` through semantic
  BIR as two ordered `bir.store_global @s, offset 8, ptr ...` updates, a later
  `bir.load_global ptr @s, offset 8`, and the final addressed-global reload
  from `@z` rather than stale initializer knowledge or raw LLVM fallback
  root-level aggregate global arrays now lower through the same aggregate lane
  as struct roots instead of being rejected before GEP lowering; new riscv64
  route proofs cover `pairs[1].y` and `pairs[1].x = 9; return pairs[1].x;`
  through semantic BIR as `bir.load_global i32 @pairs, offset 12` and
  `bir.store_global @pairs, offset 8, i32 9` plus the matching addressed
  reload rather than raw LLVM fallback
  root-level aggregate arrays with pointer-valued fields now have explicit
  route coverage too; new riscv64 route proofs cover `return *pairs[1].p;`,
  `pairs[1].p = gp; return *pairs[1].p;`, and `int **pp = &pairs[1].p;
  *pp = gp; return **pp;` through semantic BIR as `bir.load_global ptr @pairs,
  offset 24`, `bir.store_global @pairs, offset 24, ptr ...`, and the later
  addressed-global reload from `@y` rather than raw LLVM fallback
  aggregate pointer-field initializers now reuse pointer-global alias
  resolution instead of requiring direct `&global` roots only; plain struct
  roots, nested struct roots, and root-level array-of-struct roots can now
  statically initialize pointer fields from `gp` / `gpp`-style globals and
  still reload the resolved addressed-global target through semantic BIR
  rather than rejecting those initializers during module-global lowering
  offset-bearing pointer-global aliases no longer stop aggregate-root reloads
  at direct pointer globals like `@gpp`; the aggregate lane now preserves the
  honest underlying addressed-global offset for scalar reloads while leaving
  `ptr` consumers free to keep the existing pointer-global object-alias route
  when the source was `&gp` or `&gpp`
  new riscv64 route proofs now cover `struct S s = {.p = gpp, .a = 1}; return
  s.p[1];`, `struct Outer s = {1, {gpp}}; return s.inner.p[1];`, and
  `struct Pair pairs[2] = {{1, &x}, {2, gpp}}; return *pairs[1].p;`
  through semantic BIR as `bir.load_global ptr @s/@pairs, offset ...`
  followed by addressed-global reloads from `@arr, offset 12/12/8` rather
  than raw LLVM fallback, direct `@gpp` reloads, or aggregate-initializer
  rejection
  aggregate pointer-field initializers can now preserve pointer-global object
  addresses instead of flattening them too early; new riscv64 route proofs
  cover plain-struct, nested-struct, and root-array aggregates initialized
  from `&gp` / `&gpp`, through semantic BIR as ordered
  `bir.load_global ptr @s/@pairs`, then `bir.load_global ptr @gpp/@gp`,
  then the final addressed-global reload from `@arr` rather than raw LLVM
  fallback or premature pointer-value flattening
  aggregate-loaded pointer globals now rebase to their known addressed-global
  pointee when they are used as data pointers, while preserving the extra load
  step for object-alias chains; plain-struct, nested-struct, and root-array
  aggregate route proofs now show `gp` / `gpp`-initialized pointer fields as
  final `bir.load_global i32 @arr, offset 4/8/12` reloads instead of
  `@gp/@gpp`-rooted pseudo-addresses, and a new root-array object-alias proof
  covers `&gpp` preserving `bir.load_global ptr @gpp` before the addressed
  reload from `@arr, offset 12`
  new riscv64 route proofs now cover `struct S s = {.p = gpp, .a = 1};
  return s.p[1];`, `struct Outer s = {1, {gpp}}; return s.inner.p[1];`, and
  `struct Pair pairs[1] = {{2, &gpp}}; return (*pairs[0].pp)[1];` through
  semantic BIR as aggregate-field pointer loads followed by the addressed
  global reload from `@arr, offset 12` rather than the old `@gpp`-rooted
  fallback surface
  deeper root-level nested aggregate arrays now have explicit route coverage
  too; new riscv64 route proofs cover `groups[1].inner.xs[2]`,
  `groups[1].inner.xs[1] = 9; return groups[1].inner.xs[1];`, and
  `int *p = &groups[0].inner.xs[0]; p[5] = 9; return groups[1].inner.xs[2];`
  through semantic BIR as direct `bir.load_global i32 @groups, offset 20` and
  addressed `bir.store_global @groups, offset 16/20, i32 9` rather than raw
  LLVM fallback, confirming the existing addressed-global aggregate lowering
  scales across array-root then struct-field then nested-array descent without
  testcase-shaped routing
  deeper root-level nested aggregate arrays with pointer-valued fields now
  have explicit route coverage too; new riscv64 route proofs cover
  `return *groups[1].inner.p;`, `groups[1].inner.p = gp; return
  *groups[1].inner.p;`, and `int **pp = &groups[1].inner.p; *pp = gp;
  return **pp;` through semantic BIR as `bir.load_global ptr @groups,
  offset 40`, `bir.store_global @groups, offset 40, ptr ...`, and the later
  addressed-global reload from `@y` rather than raw LLVM fallback, confirming
  the addressed-global aggregate lane also scales across array-root, nested
  struct descent, and pointer-field access without reintroducing testcase-
  shaped routing
  scalar pointer globals initialized from aggregate-root addressed globals now
  preserve the parsed leaf scalar type instead of collapsing back to the
  aggregate storage byte type; new riscv64 route proofs cover direct
  `int *gp = &pairs[1].b; return *gp;` and the same root-array field alias
  through `int **gpp = &gp; return (*gpp)[0];` as `bir.load_global ptr @gp`
  or the ordered `bir.load_global ptr @gpp`, then `bir.load_global ptr @gp`,
  followed by `bir.load_global i32 @pairs, offset 12` rather than raw LLVM
  fallback
  plain struct-root field-address initializers no longer collapse to `null`
  upstream of BIR; `ConstInitEmitter` now builds typed constant GEPs across
  struct-field and array-index descent for addressable global aggregates, so
  new riscv64 route proofs cover both `int *gp = &s.xs[1]; return *gp;` and
  `int **gpp = &gp; return (*gpp)[0];` through semantic BIR as
  `bir.load_global ptr @gp` or the ordered `bir.load_global ptr @gpp`, then
  `bir.load_global ptr @gp`, followed by `bir.load_global i32 @s, offset 8`
  rather than a `null` global initializer or raw LLVM fallback
  the same typed constant-GEP lane already scales through nested struct-root
  aggregate descent too; new riscv64 route proofs cover
  `int *gp = &s.inner.xs[1]; return *gp;` and `int **gpp = &gp; return
  (*gpp)[0];` through semantic BIR as `bir.load_global ptr @gp` or the
  ordered `bir.load_global ptr @gpp`, then `bir.load_global ptr @gp`,
  followed by `bir.load_global i32 @s, offset 12` rather than a `null`
  initializer or raw LLVM fallback
- checkpoint:
  a supervisor-side broader validation checkpoint on 2026-04-13 confirmed the
  backlog item 4 addressed-global lane is green at the current route boundary:
  `backend_codegen_route_riscv64_branch_if_eq_defaults_to_bir` plus the 26
  addressed-global route tests spanning defined pointer globals, nested struct
  arrays, struct pointer arrays, and pointer-field object-alias initializers
  all passed together, so the stale item-4 follow-on note is retired here
- completed:
  the first direct-call proving surface is now routed onto explicit riscv64
  backend-route tests instead of the host x86 runtime lane:
  `backend_codegen_route_riscv64_call_helper_defaults_to_asm` proves the
  extern helper declaration case emits native riscv64 asm with `call helper`,
  and `backend_codegen_route_riscv64_local_arg_call_defaults_to_asm` proves
  the single-local-arg helper case emits native riscv64 asm with `call
  add_one` plus the existing direct local-slot/load/store path
  this keeps backlog item 5 attached to the riscv64 backend-route surface the
  runbook asked for, preserves `branch_if_eq.c` as the standing BIR sentinel,
  and avoids reopening the rejected host-runtime x86 proof seam
- completed:
  the first four two-arg direct-call shapes now stay on that same explicit
  riscv64 backend-route surface too: `backend.cpp` accepts up to two `i32`
  params and direct-call args in the existing `a0/a1` lane, including the
  small guarded move ordering needed to avoid self-clobber when named values
  already live in argument registers
  new riscv64 route proofs now cover `two_arg_helper.c`,
  `two_arg_local_arg.c`, `two_arg_second_local_arg.c`, and
  `two_arg_both_local_arg.c` as native asm with `add_pair` in `a0/a1`,
  preserving the semantic-BIR call lane instead of dumping raw `bir.call`
  text to the assembler
- checkpoint:
  a broader supervisor-selected proof that also included
  `backend_runtime_two_arg_*` remained red before and after this slice on the
  host `x86_64-unknown-linux-gnu` runtime path; those tests still assemble raw
  BIR text outside the owned riscv64 route surface, so this packet kept the
  acceptance proof on explicit riscv64 backend-route coverage rather than
  reopening the rejected host-runtime fallback seam
- completed:
  the first single-rewrite two-arg direct-call shapes now stay on that same
  explicit riscv64 backend-route surface too; new route proofs cover
  `two_arg_first_local_rewrite.c`, `two_arg_second_local_rewrite.c`,
  `two_arg_both_local_first_rewrite.c`, and
  `two_arg_both_local_second_rewrite.c` as native asm with the expected local
  slot reload, in-place `addi ..., 0` rewrite, and final `a0/a1` call setup
  before `call add_pair`
  this keeps backlog item 5 on the honest semantic-BIR/prepared-BIR riscv64
  path for the first rewrite variants without reopening raw-BIR asm output,
  direct-route fallbacks, or testcase-shaped target shortcuts
- completed:
  the first double-rewrite two-arg direct-call shape now stays on that same
  explicit riscv64 backend-route surface too; a new route proof covers
  `two_arg_both_local_double_rewrite.c` as native asm with both local-slot
  reloads, both in-place `addi ..., 0` rewrites, the final `a0/a1` call setup,
  and `call add_pair`
  this closes the current rewrite-only direct-call proving surface on the
  honest semantic-BIR/prepared-BIR riscv64 path without reopening raw-BIR asm
  output, direct-route fallbacks, or testcase-shaped target shortcuts
- completed:
  the first honest two-arg indirect-call lane now stays on that same explicit
  riscv64 backend-route surface too; `backend.cpp` now preserves the incoming
  third integer-class argument register on function entry, so semantic BIR
  functions with `ptr, i32, i32` params can reach the existing indirect-call
  emitter instead of falling back before emission
  new route proofs cover `indirect_two_arg_param_call.c` and
  `indirect_two_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, arg rewrites from `a1/a2` into `a0/a1`, and final
  `jalr ra, t0, 0`, while the earlier one-arg indirect and `two_arg_*`
  direct-call sentinels stayed green beside them
  this keeps backlog item 5 on the honest semantic-BIR/prepared-BIR riscv64
  path for the first two-arg indirect family without reopening host-runtime
  x86 fallback seams or widening into ABI-shaped call metadata
- blocked:
  none in owned files for this packet
- remaining next:
  decide whether backlog item 2 should next keep widening entry-signature
  coverage past the first stack-passed integer lane, or whether backlog item 5
  should resume outward call-lane work from this repaired nine-parameter
  boundary without reopening direct-route fallbacks or jumping ahead to
  ABI-shaped call work
- proof:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
- proof log:
  `test_after.log`

## Parked While This Packet Is Active

- intrinsic/runtime operation lowering
- real stack layout, liveness, and regalloc
- target backend ingestion rewrites beyond the shared BIR/prepare spine
