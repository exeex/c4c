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
  `review/route_alignment_after_indirect_call_proof_run.md` judged the active
  plan still aligned to the source idea but called for a route reset before
  more execution because the riscv64 indirect-call work had drifted into
  proof-only arg-count churn
- current capability family:
  backlog item 5 is now reset onto semantic callee-provenance work instead of
  further riscv64 arg-count widening:
  the accepted route already proves the shared indirect-call lane through the
  first mixed `ptr` families, the first ptr-capable result families, and the
  first honest seventeen-stack-slot integer-class surface at twenty-five args,
  which is enough width evidence for now
- current packet shape:
  stop packeting backlog item 5 as "next wider indirect-call signature"
  work; the next executor slice should target a semantic family that still
  requires real lowering/backend changes, specifically indirect callee
  provenance through memory-carried or merge-preserved function-pointer values
  instead of more width-only wrapper proofs
- candidate proving surface:
  the next honest proving surface should force semantic handling of callee
  identity rather than one more width proof: prefer internal backend-route
  cases where the indirect callee is loaded from shared memory/state
  structures or preserved across CFG merge/select-style value flow, while
  keeping `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, and the
  standing one-arg through twenty-five-arg indirect-call plus `two_arg_*`
  direct-call route tests only as sentinels, not as the packet-selection
  mechanism

## Immediate Target

- keep packet selection attached to the ordered semantic backlog in `plan.md`
- carry the now-green addressed-global work forward by moving to the next
  semantic family instead of stretching backlog item 4 past its proving surface
- freeze further indirect-call width-only proofs beyond the accepted
  twenty-five-arg family unless a future packet also repairs a real shared
  lowering/backend limitation
- keep backlog item 5 on call lowering, but shift from width counting to
  semantic callee-provenance coverage:
  globals, loaded/stored function pointers, and merge-preserved callee values
  should lower through BIR without reopening fallback routes
- avoid reintroducing testcase-shaped routing while broadening the shared
  entry-signature and call lanes
- do not add new exact rendered-asm snippet ladders whose main effect is to
  prove one more named arg-count case

## Done Condition For The Active Packet

- `branch_if_eq.c` still lowers to clean BIR
- the existing direct-call sentinels, one-arg indirect-call tests, and
  rewrite-only two-arg route tests stay green on riscv64
- the next accepted call-lane slice must include real lowering/backend work for
  callee provenance or signature semantics; proof-only width growth is not a
  done condition
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
  reviewer scrutiny in
  `review/route_alignment_after_indirect_call_proof_run.md`
  judged the active runbook still faithful to the source idea but rejected
  further riscv64 width-only indirect-call packets as route drift:
  the accepted proving surface now stops at the twenty-five-arg integer-class
  family, the reverted twenty-six-arg proof-only slice must not be
  reintroduced, and the active route is reset onto semantic callee-provenance
  work before more execution
- completed:
  the first honest twenty-five-arg integer-class indirect-call family now
  stays on the same shared semantic-BIR/prepared-BIR riscv64 route surface as
  the earlier one-arg through twenty-four-arg work without reopening the
  backend route or requiring a backend algorithm edit: the existing
  spill-as-you-go stack-arg materialization path already scales to a
  seventeenth stack-passed callee arg, so the caller continues to preserve
  the standing register-lane shuffle for the first eight integer-class args,
  loads the next sixteen wrapper stack args one by one through `t1`, spills
  them into the aligned 144-byte temporary call area at `0(sp)` through
  `120(sp)`, materializes the twenty-fifth immediate callee arg into that
  same scratch lane, stores it at `128(sp)`, and then restores both the call
  area and saved `ra` after `jalr` without reintroducing fallback routes or
  widening into ABI-shaped call work
  new route proofs cover `indirect_twenty_five_arg_param_call.c` and
  `indirect_twenty_five_arg_local_call.c` as native asm, while
  `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, and the earlier
  one-arg through twenty-four-arg indirect-call plus `ptr`-shaped and
  `two_arg_*` direct-call sentinels stayed in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded, the two new riscv64 route tests passed as
  tests `#325` and `#326`, the broad `^backend_` subset still returned
  non-zero because it remains dominated by standing unrelated failures, and
  the total backend route surface increased from `401` to `403` while the
  standing failed-test count held at `225`, so the first seventeen-stack-slot
  indirect-call family is now covered on the shared riscv64 lane without a
  broader regression increase
- completed:
  the first honest twenty-four-arg integer-class indirect-call family now
  stays on the same shared semantic-BIR/prepared-BIR riscv64 route surface as
  the earlier one-arg through twenty-three-arg work without reopening the
  backend route or requiring a backend algorithm edit: the existing
  spill-as-you-go stack-arg materialization path already scales to a
  sixteenth stack-passed callee arg, so the caller continues to preserve the
  standing register-lane shuffle for the first eight integer-class args,
  loads the next fifteen wrapper stack args one by one through `t1`, spills
  them into the aligned 128-byte temporary call area at `0(sp)` through
  `112(sp)`, materializes the twenty-fourth immediate callee arg into that
  same scratch lane, stores it at `120(sp)`, and then restores both the call
  area and saved `ra` after `jalr` without reintroducing fallback routes or
  widening into ABI-shaped call work
  new route proofs cover `indirect_twenty_four_arg_param_call.c` and
  `indirect_twenty_four_arg_local_call.c` as native asm, while
  `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, and the earlier
  one-arg through twenty-three-arg indirect-call plus `ptr`-shaped and
  `two_arg_*` direct-call sentinels stayed in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded, the two new riscv64 route tests passed as
  tests `#323` and `#324`, the broad `^backend_` subset still returned
  non-zero because it remains dominated by standing unrelated failures, and
  the total backend route surface increased from `399` to `401`, so the first
  sixteen-stack-slot indirect-call family is now covered on the shared
  riscv64 lane while supervisor-side regression judgment remains outstanding
- completed:
  the first honest twenty-three-arg integer-class indirect-call family now
  stays on the same shared semantic-BIR/prepared-BIR riscv64 route surface as
  the earlier one-arg through twenty-two-arg work without reopening the
  backend route or requiring a backend algorithm edit: the existing
  spill-as-you-go stack-arg materialization path already scales to a fifteenth
  stack-passed callee arg, so the caller continues to preserve the standing
  register-lane shuffle for the first eight integer-class args, loads the next
  fourteen wrapper stack args one by one through `t1`, spills them into the
  aligned temporary call area, materializes the twenty-third immediate callee
  arg into that same scratch lane, and then restores both the call area and
  saved `ra` after `jalr` without reintroducing fallback routes or widening
  into ABI-shaped call work
  new route proofs cover `indirect_twenty_three_arg_param_call.c` and
  `indirect_twenty_three_arg_local_call.c` as native asm, while
  `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, and the earlier
  one-arg through twenty-two-arg indirect-call plus `ptr`-shaped and
  `two_arg_*` direct-call sentinels stayed in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded, the two new riscv64 route tests passed as
  tests `#321` and `#322`, the broad `^backend_` subset still returned
  non-zero because it remains dominated by standing unrelated failures, and
  the total backend route surface increased from `397` to `399`, so the first
  fifteen-stack-slot indirect-call family is now covered on the shared
  riscv64 lane while supervisor-side regression judgment remains outstanding
- completed:
  the first honest twenty-two-arg integer-class indirect-call family now
  stays on the same shared semantic-BIR/prepared-BIR riscv64 route surface as
  the earlier one-arg through twenty-one-arg work without reopening the
  backend route or requiring a backend algorithm edit: the existing
  spill-as-you-go stack-arg materialization path already scaled to a
  fourteenth stack-passed callee arg, so the caller continues to preserve the
  standing register-lane shuffle for the first eight integer-class args,
  loads the next thirteen wrapper stack args one by one through `t1`, spills
  them into the aligned 112-byte temporary call area at `0(sp)` through
  `96(sp)`, materializes the twenty-second immediate callee arg into that
  same scratch lane, stores it at `104(sp)`, and then restores both the call
  area and saved `ra` after `jalr` without reintroducing fallback routes or
  widening into ABI-shaped call work
  new route proofs cover `indirect_twenty_two_arg_param_call.c` and
  `indirect_twenty_two_arg_local_call.c` as native asm with the expected
  saved-`ra` prologue, the aligned `addi sp, sp, -112`, the thirteen
  stack-backed wrapper args reloaded one by one through `t1` with matching
  stores at `0(sp)`, `8(sp)`, `16(sp)`, `24(sp)`, `32(sp)`, `40(sp)`,
  `48(sp)`, `56(sp)`, `64(sp)`, `72(sp)`, `80(sp)`, `88(sp)`, and `96(sp)`,
  the twenty-second callee arg materialized into that same scratch lane and
  stored at `104(sp)`, the leading register arg moves through `a0`..`a6`,
  the incoming stack-passed eighth wrapper arg loaded into `a7`,
  `jalr ra, t0, 0`, and final `addi sp, sp, 112`, while `branch_if_eq.c`,
  `call_helper.c`, `local_arg_call.c`, and the earlier one-arg through
  twenty-one-arg indirect-call plus `ptr`-shaped and `two_arg_*` direct-call
  sentinels stayed in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded, the two new riscv64 route tests passed as
  tests `#319` and `#320`, the broad `^backend_` subset still returned
  non-zero because it remains dominated by standing unrelated failures, and
  the total backend route surface increased from `395` to `397`, so the first
  fourteen-stack-slot indirect-call family is now covered on the shared
  riscv64 lane while supervisor-side regression judgment remains outstanding
- completed:
  the first honest twenty-one-arg integer-class indirect-call family now
  stays on the same shared semantic-BIR/prepared-BIR riscv64 route surface as
  the earlier one-arg through twenty-arg work without reopening the backend
  route or requiring a backend algorithm edit: the existing spill-as-you-go
  stack-arg materialization path already scaled to a thirteenth stack-passed
  callee arg, so the caller continues to preserve the standing register-lane
  shuffle for the first eight integer-class args, loads the next twelve
  wrapper stack args one by one through `t1`, spills them into the aligned
  112-byte temporary call area at `0(sp)` through `88(sp)`, materializes the
  twenty-first immediate callee arg into that same scratch lane, stores it at
  `96(sp)`, and then restores both the call area and saved `ra` after `jalr`
  without reintroducing fallback routes or widening into ABI-shaped call work
  new route proofs cover `indirect_twenty_one_arg_param_call.c` and
  `indirect_twenty_one_arg_local_call.c` as native asm with the expected
  saved-`ra` prologue, the aligned `addi sp, sp, -112`, the twelve
  stack-backed wrapper args reloaded one by one through `t1` with matching
  stores at `0(sp)`, `8(sp)`, `16(sp)`, `24(sp)`, `32(sp)`, `40(sp)`,
  `48(sp)`, `56(sp)`, `64(sp)`, `72(sp)`, `80(sp)`, and `88(sp)`, the
  twenty-first callee arg materialized into that same scratch lane and stored
  at `96(sp)`, the leading register arg moves through `a0`..`a6`, the
  incoming stack-passed eighth wrapper arg loaded into `a7`, `jalr ra, t0, 0`,
  and final `addi sp, sp, 112`, while `branch_if_eq.c`, `call_helper.c`,
  `local_arg_call.c`, and the earlier one-arg through twenty-arg
  indirect-call plus `ptr`-shaped and `two_arg_*` direct-call sentinels
  stayed in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded, the two new riscv64 route tests passed as
  tests `#317` and `#318`, the broad `^backend_` subset still returned
  non-zero because it remains dominated by standing unrelated failures, and
  the total backend route surface increased from `393` to `395`, so the first
  thirteen-stack-slot indirect-call family is now covered on the shared
  riscv64 lane while supervisor-side regression judgment remains outstanding
- completed:
  the first honest twenty-arg integer-class indirect-call family now stays on
  the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier one-arg through nineteen-arg work without reopening the backend
  route or requiring a backend algorithm edit: the existing spill-as-you-go
  stack-arg materialization path already scaled to a twelfth stack-passed
  callee arg, so the caller continues to preserve the standing register-lane
  shuffle for the first eight integer-class args, loads the next eleven
  wrapper stack args one by one through `t1`, spills them into the aligned
  96-byte temporary call area at `0(sp)` through `80(sp)`, materializes the
  twentieth immediate callee arg into that same scratch lane, stores it at
  `88(sp)`, and then restores `sp` after `jalr` without reintroducing
  fallback routes or widening into ABI-shaped call work
  new route proofs cover `indirect_twenty_arg_param_call.c` and
  `indirect_twenty_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, the aligned `addi sp, sp, -96`, the eleven stack-backed
  wrapper args reloaded one by one through `t1` with matching stores at
  `0(sp)`, `8(sp)`, `16(sp)`, `24(sp)`, `32(sp)`, `40(sp)`, `48(sp)`,
  `56(sp)`, `64(sp)`, `72(sp)`, and `80(sp)`, the twentieth callee arg
  materialized into that same scratch lane and stored at `88(sp)`, the
  leading register arg moves through `a0`..`a6`, the incoming stack-passed
  eighth wrapper arg loaded into `a7`, `jalr ra, t0, 0`, and final
  `addi sp, sp, 96`, while `branch_if_eq.c`, `call_helper.c`,
  `local_arg_call.c`, and the earlier one-arg through nineteen-arg
  indirect-call plus `ptr`-shaped and `two_arg_*` direct-call sentinels
  stayed in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded, the two new riscv64 route tests passed as
  tests `#315` and `#316`, the broad `^backend_` subset still returned
  non-zero because it remains dominated by standing unrelated failures, and
  the total backend route surface increased from `391` to `393`, so the first
  twelve-stack-slot indirect-call family is now covered on the shared
  riscv64 lane while supervisor-side regression judgment remains outstanding
- completed:
  the first honest nineteen-arg integer-class indirect-call family now stays
  on the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier one-arg through eighteen-arg work without reopening the backend
  route: the existing spill-as-you-go stack-arg materialization path already
  scaled to an eleventh stack-passed callee arg, so the caller continues to
  preserve the standing register-lane shuffle for the first eight
  integer-class args, loads the next ten wrapper stack args one by one
  through `t1`, spills them into the aligned 96-byte temporary call area at
  `0(sp)` through `72(sp)`, materializes the nineteenth immediate callee arg
  into that same scratch lane, stores it at `80(sp)`, and then restores `sp`
  after `jalr` without reintroducing fallback routes or widening into
  ABI-shaped call work
  new route proofs cover `indirect_nineteen_arg_param_call.c` and
  `indirect_nineteen_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, the aligned `addi sp, sp, -96`, the ten stack-backed
  wrapper args reloaded one by one through `t1` with matching stores at
  `0(sp)`, `8(sp)`, `16(sp)`, `24(sp)`, `32(sp)`, `40(sp)`, `48(sp)`,
  `56(sp)`, `64(sp)`, and `72(sp)`, the nineteenth callee arg materialized
  into that same scratch lane and stored at `80(sp)`, the leading register
  arg moves through `a0`..`a6`, the incoming stack-passed eighth wrapper arg
  loaded into `a7`, `jalr ra, t0, 0`, and final `addi sp, sp, 96`, while
  `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, and the earlier
  one-arg through eighteen-arg indirect-call plus `ptr`-shaped and
  `two_arg_*` direct-call sentinels stayed in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded, the two new riscv64 route tests passed as
  tests `#313` and `#314`, the broad `^backend_` subset still returned
  non-zero because it remains dominated by standing unrelated failures, and
  the total backend route surface increased from `389` to `391`, so the first
  eleven-stack-slot indirect-call family is now covered on the shared
  riscv64 lane while supervisor-side regression judgment remains outstanding
- completed:
  the first honest eighteen-arg integer-class indirect-call family now stays
  on the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier one-arg through seventeen-arg work without reopening the backend
  route: the existing spill-as-you-go stack-arg materialization path already
  scaled to a tenth stack-passed callee arg, so the caller continues to
  preserve the standing register-lane shuffle for the first eight
  integer-class args, loads the next nine wrapper stack args one by one
  through `t1`, spills them into the aligned 80-byte temporary call area at
  `0(sp)` through `64(sp)`, materializes the eighteenth immediate callee arg
  into that same scratch lane, stores it at `72(sp)`, and then restores `sp`
  after `jalr` without reintroducing fallback routes or widening into
  ABI-shaped call work
  new route proofs cover `indirect_eighteen_arg_param_call.c` and
  `indirect_eighteen_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, the aligned `addi sp, sp, -80`, the nine stack-backed
  wrapper args reloaded one by one through `t1` with matching stores at
  `0(sp)`, `8(sp)`, `16(sp)`, `24(sp)`, `32(sp)`, `40(sp)`, `48(sp)`,
  `56(sp)`, and `64(sp)`, the eighteenth callee arg materialized into that
  same scratch lane and stored at `72(sp)`, the leading register arg moves
  through `a0`..`a6`, the incoming stack-passed eighth wrapper arg loaded into
  `a7`, `jalr ra, t0, 0`, and final `addi sp, sp, 80`, while `branch_if_eq.c`,
  `call_helper.c`, `local_arg_call.c`, and the earlier one-arg through
  seventeen-arg indirect-call plus `ptr`-shaped and `two_arg_*` direct-call
  sentinels stayed in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded, the two new riscv64 route tests passed as
  tests `#311` and `#312`, the broad `^backend_` subset still returned
  non-zero because it remains at `225` standing failures, the total backend
  route surface increased from `387` to `389`, and supervisor-side regression
  guard passed with `passed=162 -> 164`, `failed=225 -> 225`, and `0` new
  failing tests, so the first ten-stack-slot indirect-call family is now
  covered on the shared riscv64 lane
- completed:
  the first honest seventeen-arg integer-class indirect-call family now stays
  on the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier one-arg through sixteen-arg work without reopening the backend
  route: the existing spill-as-you-go stack-arg materialization path already
  scaled to a ninth stack-passed callee arg, so the caller continues to
  preserve the standing register-lane shuffle for the first eight
  integer-class args, loads the next eight wrapper stack args one by one
  through `t1`, spills them into the aligned 80-byte temporary call area at
  `0(sp)` through `56(sp)`, materializes the seventeenth immediate callee arg
  into that same scratch lane, stores it at `64(sp)`, and then restores `sp`
  after `jalr` without reintroducing fallback routes or widening into
  ABI-shaped call work
  new route proofs cover `indirect_seventeen_arg_param_call.c` and
  `indirect_seventeen_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, the aligned `addi sp, sp, -80`, the eight stack-backed
  wrapper args reloaded one by one through `t1` with matching stores at
  `0(sp)`, `8(sp)`, `16(sp)`, `24(sp)`, `32(sp)`, `40(sp)`, `48(sp)`, and
  `56(sp)`, the seventeenth callee arg materialized into that same scratch
  lane and stored at `64(sp)`, the leading register arg moves through
  `a0`..`a6`, the incoming stack-passed eighth wrapper arg loaded into `a7`,
  `jalr ra, t0, 0`, and final `addi sp, sp, 80`, while `branch_if_eq.c`,
  `call_helper.c`, `local_arg_call.c`, and the earlier one-arg through
  sixteen-arg indirect-call plus `ptr`-shaped and `two_arg_*` direct-call
  sentinels stayed in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded, the two new riscv64 route tests passed as
  tests `#309` and `#310`, the broad `^backend_` subset still returned
  non-zero because it remains at `225` standing failures, and the total
  backend route surface increased from `385` to `387`, so the first nine-
  stack-slot indirect-call family is now covered on the shared riscv64 lane
  while supervisor-side regression judgment remains outstanding
- completed:
  the first honest sixteen-arg integer-class indirect-call family now stays
  on the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier one-arg through fifteen-arg work without reopening the backend
  route: the existing spill-as-you-go stack-arg materialization path already
  scaled to an eighth stack-passed callee arg, so the caller continues to
  preserve the standing register-lane shuffle for the first eight
  integer-class args, loads the next seven wrapper stack args one by one
  through `t1`, spills them into the aligned 64-byte temporary call area at
  `0(sp)` through `48(sp)`, materializes the sixteenth immediate callee arg
  into that same scratch lane, stores it at `56(sp)`, and then restores `sp`
  after `jalr` without reintroducing fallback routes or widening into
  ABI-shaped call work
  new route proofs cover `indirect_sixteen_arg_param_call.c` and
  `indirect_sixteen_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, the aligned `addi sp, sp, -64`, the seven stack-backed
  wrapper args reloaded one by one through `t1` with matching stores at
  `0(sp)`, `8(sp)`, `16(sp)`, `24(sp)`, `32(sp)`, `40(sp)`, and `48(sp)`,
  the sixteenth callee arg materialized into that same scratch lane and
  stored at `56(sp)`, the leading register arg moves through `a0`..`a6`, the
  incoming stack-passed eighth wrapper arg loaded into `a7`, `jalr ra, t0, 0`,
  and final `addi sp, sp, 64`, while `branch_if_eq.c`, `call_helper.c`,
  `local_arg_call.c`, and the earlier one-arg through fifteen-arg
  indirect-call plus `ptr`-shaped and `two_arg_*` direct-call sentinels stayed
  in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded, the two new riscv64 route tests passed as
  tests `#307` and `#308`, the broad `^backend_` subset still returned
  non-zero because it remains at `225` standing failures, and the total
  backend route surface increased from `383` to `385`, so the first eight-
  stack-slot indirect-call family is now covered on the shared riscv64 lane
  while supervisor-side regression judgment remains outstanding
- completed:
  the first honest fifteen-arg integer-class indirect-call family now stays
  on the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier one-arg through fourteen-arg work without reopening the backend
  route: the real blocker was no longer call-area size itself but temp
  pressure in `lower_riscv_call_inst`, because the old pre-materialize-all
  path exhausted the riscv temp pool once an indirect call needed to preserve
  the callee plus seven outgoing stack args; keeping the existing temp-backed
  path for stack-arg counts that still fit while adding a general spill-as-
  you-go path for larger outgoing stacks fixed that honestly, so oversized
  indirect calls now reserve the aligned 64-byte temporary call area first,
  reuse one scratch temp to materialize and spill each outgoing stack arg at
  `0(sp)` through `48(sp)`, load any wrapper stack params with the call-area
  bias applied, and then restore `sp` after `jalr` without reintroducing
  fallback routes or widening into ABI-shaped call work
  new route proofs cover `indirect_fifteen_arg_param_call.c` and
  `indirect_fifteen_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, the aligned `addi sp, sp, -64`, the six stack-backed
  wrapper args reloaded one by one through `t1` with matching stores at
  `0(sp)`, `8(sp)`, `16(sp)`, `24(sp)`, `32(sp)`, and `40(sp)`, the fifteenth
  callee arg materialized into that same scratch lane and stored at `48(sp)`,
  the leading register arg moves through `a0`..`a6`, the incoming stack-passed
  eighth wrapper arg loaded into `a7`, `jalr ra, t0, 0`, and final
  `addi sp, sp, 64`, while `branch_if_eq.c`, `call_helper.c`,
  `local_arg_call.c`, and the earlier one-arg through fourteen-arg
  indirect-call plus `ptr`-shaped and `two_arg_*` direct-call sentinels stayed
  in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded, the two new riscv64 route tests passed as
  tests `#305` and `#306`, the broad `^backend_` subset still returned
  non-zero because it remains at `225` standing failures, the total backend
  route surface increased from `381` to `383`, and supervisor-side regression
  guard passed with `passed=156 -> 158`, `failed=225 -> 225`, and `0` new
  failing tests, so the first seven-stack-slot indirect-call family is now
  covered on the shared riscv64 lane
- completed:
  the first honest fourteen-arg integer-class indirect-call family now stays
  on the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier one-arg through thirteen-arg work without reopening the backend
  route: the existing generic stack-arg materialization path already scaled to
  a sixth stack-passed callee arg, so the caller continues to preserve the
  standing register-lane shuffle for the first eight integer-class args, loads
  the next five wrapper stack args into `t1`, `t2`, `t3`, `t4`, and `t5`,
  materializes the fourteenth immediate callee arg into `t6`, spills all six
  into the same aligned 48-byte temporary call area at `0(sp)`, `8(sp)`,
  `16(sp)`, `24(sp)`, `32(sp)`, and `40(sp)`, and then restores `sp` after
  `jalr` without reintroducing fallback routes or widening into ABI-shaped
  call work
  new route proofs cover `indirect_fourteen_arg_param_call.c` and
  `indirect_fourteen_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, leading register arg moves through `a0`..`a6`, the
  incoming stack-passed eighth wrapper arg loaded into `a7`, the next five
  stack-passed wrapper args loaded into `t1`, `t2`, `t3`, `t4`, and `t5`, the
  fourteenth callee arg materialized into `t6`, the temporary
  `addi sp, sp, -48`, six stores at `0(sp)`, `8(sp)`, `16(sp)`, `24(sp)`,
  `32(sp)`, and `40(sp)`, `jalr ra, t0, 0`, and final `addi sp, sp, 48`,
  while `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, and the
  earlier one-arg through thirteen-arg indirect-call plus `ptr`-shaped and
  `two_arg_*` direct-call sentinels stayed in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded and the two new riscv64 route tests passed as
  tests `#303` and `#304`; the broad `^backend_` subset still returned
  non-zero because it remains at `225` standing failures out of `381`, so the
  total backend route surface increased from `379` to `381` and the first
  six-stack-slot indirect-call family is now covered on the shared riscv64
  lane while supervisor-side regression judgment remains outstanding
- completed:
  the first honest thirteen-arg integer-class indirect-call family now stays
  on the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier one-arg through twelve-arg work without reopening the backend route:
  the existing generic stack-arg materialization path already scaled to a
  fifth stack-passed callee arg, so the caller continues to preserve the
  standing register-lane shuffle for the first eight integer-class args, loads
  the next four wrapper stack args into `t1`, `t2`, `t3`, and `t4`,
  materializes the thirteenth immediate callee arg into `t5`, spills all five
  into the same aligned 48-byte temporary call area at `0(sp)`, `8(sp)`,
  `16(sp)`, `24(sp)`, and `32(sp)`, and then restores `sp` after `jalr`
  without reintroducing fallback routes or widening into ABI-shaped call work
  new route proofs cover `indirect_thirteen_arg_param_call.c` and
  `indirect_thirteen_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, leading register arg moves through `a0`..`a6`, the
  incoming stack-passed eighth wrapper arg loaded into `a7`, the next four
  stack-passed wrapper args loaded into `t1`, `t2`, `t3`, and `t4`, the
  thirteenth callee arg materialized into `t5`, the temporary
  `addi sp, sp, -48`, five stores at `0(sp)`, `8(sp)`, `16(sp)`, `24(sp)`,
  and `32(sp)`, `jalr ra, t0, 0`, and final `addi sp, sp, 48`, while
  `branch_if_eq.c`, `call_helper.c`, `local_arg_call.c`, and the earlier
  one-arg through twelve-arg indirect-call plus `ptr`-shaped and `two_arg_*`
  direct-call sentinels stayed in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded and the two new riscv64 route tests passed as
  tests `#301` and `#302`; the broad `^backend_` subset still returned
  non-zero because it remains at `225` standing failures out of `379`, but
  supervisor-side regression guard passed with `passed=152 -> 154`,
  `failed=225 -> 225`, and `0` new failing tests, so the total backend route
  surface increased from `377` to `379` while the first five-stack-slot
  indirect-call family became covered on the shared riscv64 lane
- completed:
  the first honest twelve-arg integer-class indirect-call family now stays on
  the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier one-arg through eleven-arg work without reopening the backend route:
  the existing generic stack-arg materialization path already scaled to a
  fourth stack-passed callee arg, so the caller continues to preserve the
  standing register-lane shuffle for the first eight integer-class args, loads
  the next three wrapper stack args into `t1`, `t2`, and `t3`, materializes
  the twelfth immediate callee arg into `t4`, spills all four into the same
  aligned 32-byte temporary call area at `0(sp)`, `8(sp)`, `16(sp)`, and
  `24(sp)`, and then restores `sp` after `jalr` without reintroducing
  fallback routes or widening into ABI-shaped call work
  new route proofs cover `indirect_twelve_arg_param_call.c` and
  `indirect_twelve_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, leading register arg moves through `a0`..`a6`, the
  incoming stack-passed eighth wrapper arg loaded into `a7`, the next three
  stack-passed wrapper args loaded into `t1`, `t2`, and `t3`, the twelfth
  callee arg materialized into `t4`, the temporary `addi sp, sp, -32`, four
  stores at `0(sp)`, `8(sp)`, `16(sp)`, and `24(sp)`, `jalr ra, t0, 0`, and
  final `addi sp, sp, 32`, while `branch_if_eq.c`, `call_helper.c`,
  `local_arg_call.c`, and the earlier one-arg through eleven-arg indirect-call
  plus `ptr`-shaped and `two_arg_*` direct-call sentinels stayed in the owned
  proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded and the two new riscv64 route tests passed as
  tests `#299` and `#300`; the broad `^backend_` subset still returned
  non-zero because it remains at `225` standing failures out of `377`, but
  supervisor-side regression guard passed with `passed=150 -> 152`,
  `failed=225 -> 225`, and `0` new failing tests, so the total backend route
  surface increased from `375` to `377` while the first four-stack-slot
  indirect-call family became covered on the shared riscv64 lane
- completed:
  the first honest eleven-arg integer-class indirect-call family now stays on
  the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier one-arg through ten-arg work: the real blocker was a stale
  `kRiscvArgRegs.size() + 2` guard in `lower_riscv_call_inst` that rejected
  calls with more than two stack-passed callee args before the existing
  generic stack-arg materialization path could run, so removing that fixed cap
  now lets the caller preserve the standing register-lane shuffle for the
  first eight integer-class args, materialize the ninth and tenth callee args
  from the wrapper stack into `t1` and `t2`, materialize the eleventh
  immediate callee arg into `t3`, spill all three into a temporary 32-byte
  call area at `0(sp)`, `8(sp)`, and `16(sp)`, and then restore `sp` after
  `jalr` without reintroducing fallback routes or widening into ABI-shaped
  call work
  new route proofs cover `indirect_eleven_arg_param_call.c` and
  `indirect_eleven_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, leading register arg moves through `a0`..`a6`, the
  incoming stack-passed eighth wrapper arg loaded into `a7`, the next two
  stack-passed wrapper args loaded into `t1` and `t2`, the eleventh callee
  arg materialized into `t3`, the temporary `addi sp, sp, -32`, paired
  `sw t1, 0(sp)`, `sw t2, 8(sp)`, and `sw t3, 16(sp)`, `jalr ra, t0, 0`, and
  final `addi sp, sp, 32`, while `branch_if_eq.c`, `call_helper.c`,
  `local_arg_call.c`, and the earlier one-arg through ten-arg indirect-call
  plus `ptr`-shaped and `two_arg_*` direct-call sentinels stayed in the owned
  proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded and the two new riscv64 route tests passed as
  tests `#297` and `#298`; the broad `^backend_` subset still returned
  non-zero because it remains at `225` standing failures out of `375`, so the
  blocker is outside the owned files even though the total backend route
  surface increased from `373` to `375` and the first three-stack-slot
  indirect-call family is now covered on the shared riscv64 lane
- completed:
  the first honest ten-arg integer-class indirect-call family now stays on the
  same shared semantic-BIR/prepared-BIR riscv64 route surface as the earlier
  one-arg through nine-arg work: the caller still preserves the existing
  register-lane shuffle for the first eight integer-class args, now
  materializes the ninth callee arg from the wrapper stack into `t1`,
  materializes the tenth immediate callee arg into `t2`, spills both into the
  existing temporary 16-byte call-area at `0(sp)` and `8(sp)`, and then
  restores `sp` after `jalr` without reintroducing fallback routes or jumping
  ahead to wider stack-call ABI work
  new route proofs cover `indirect_ten_arg_param_call.c` and
  `indirect_ten_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, leading register arg moves through `a0`..`a6`, the
  incoming stack-passed eighth wrapper arg loaded into `a7`, the second
  stack-passed wrapper arg loaded into `t1`, the tenth callee arg
  materialized into `t2`, the temporary `addi sp, sp, -16`, paired
  `sw t1, 0(sp)` and `sw t2, 8(sp)`, `jalr ra, t0, 0`, and final
  `addi sp, sp, 16`, while `branch_if_eq.c`, `call_helper.c`,
  `local_arg_call.c`, and the earlier one-arg through nine-arg indirect-call
  plus `ptr`-shaped and `two_arg_*` direct-call sentinels stayed in the owned
  proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the two new riscv64 route tests passed, the broad `^backend_` subset stayed
  flat at `225` failing tests before and after, the total backend test surface
  increased from `371` to `373`, and supervisor-side regression guard passed
  with `passed=146 -> 148`, `failed=225 -> 225`, and `0` new failing tests
  this keeps backlog item 5 moving on an honest shared integer-class
  indirect-call lane by proving the first two-stack-slot callee-arg family
  without introducing direct routes, rendered-text matchers, or wider
  stack-call ABI shortcuts
- completed:
  the first honest nine-arg integer-class indirect-call family now stays on
  the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier one-arg through eight-arg work: the caller now preserves the
  existing register-lane shuffle for the first eight integer-class args,
  spills exactly one extra integer-class arg into a temporary 16-byte
  call-area slot at `0(sp)` for the call boundary, and then restores `sp`
  after `jalr` without reintroducing fallback routes or jumping ahead to wider
  stack-call ABI work
  new route proofs cover `indirect_nine_arg_param_call.c` and
  `indirect_nine_arg_local_call.c` as native asm with the expected callee
  preserve into `t0`, leading register arg moves through `a0`..`a6`, the
  incoming stack-passed eighth wrapper arg loaded into `a7`, the ninth callee
  arg materialized into `t1`, the temporary `addi sp, sp, -16`, `sw t1, 0(sp)`,
  `jalr ra, t0, 0`, and final `addi sp, sp, 16`, while `branch_if_eq.c`,
  `call_helper.c`, `local_arg_call.c`, and the earlier one-arg through
  eight-arg indirect-call plus `ptr`-shaped and `two_arg_*` direct-call
  sentinels stayed in the owned proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the two new riscv64 route tests passed, the broad `^backend_` subset stayed
  flat at `225` failing tests before and after, the total backend test surface
  increased from `369` to `371`, and supervisor-side regression guard passed
  with `passed=144 -> 146`, `failed=225 -> 225`, and `0` new failing tests
  this keeps backlog item 5 moving on an honest shared integer-class
  indirect-call lane by proving the first stack-passed callee-arg family
  without introducing direct routes, rendered-text matchers, or wider
  stack-call ABI shortcuts
- completed:
  the last remaining adjacent mixed three-arg multi-`ptr` family now stays on
  the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier single-ptr, two-ptr, and first two mixed three-arg families: no
  backend route rewrite was needed because the existing integer-class riscv64
  arg lane already treats adjacent `ptr` values like neighboring `i32` args
  regardless of position, so helper wrappers with a `int *, int *, int -> i32`
  callee signature lower through the standing eight-register indirect-call
  lane without reopening fallback routes or ABI-shaped stack-call work
  new route proofs cover `indirect_two_ptr_i32_arg_param_call.c` and
  `indirect_two_ptr_i32_arg_local_call.c` as native asm with the expected
  callee preserve into `t0`, leading `ptr` arg move into `a0`, second `ptr`
  arg move into `a1`, trailing `i32` arg move into `a2`, and final
  `jalr ra, t0, 0`, while `branch_if_eq.c`, `call_helper.c`,
  `local_arg_call.c`, and the earlier one-arg through eight-arg indirect-call
  plus `ptr`-shaped and `two_arg_*` direct-call sentinels stayed in the owned
  proof surface
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded and the two new riscv64 route tests passed as
  tests `#271` and `#272`; the broad `^backend_` subset still returned
  non-zero because it remains at `225` standing failures out of `369`, so the
  blocker is outside the owned files even though the last adjacent
  `ptr, ptr, i32 -> i32` family is now covered on the shared eight-register
  lane
- completed:
  the next honest adjacent mixed three-arg multi-`ptr` family now stays on
  the same shared semantic-BIR/prepared-BIR riscv64 route surface as the
  earlier single-ptr, two-ptr, and first mixed three-arg work: no backend
  route rewrite was needed because the existing integer-class riscv64 arg
  lane already treats `ptr` values like neighboring `i32` args regardless of
  position, so helper wrappers with a `int *, int, int * -> i32` callee
  signature lower through the standing eight-register indirect-call lane
  without reopening fallback routes or ABI-shaped stack-call work
  new route proofs cover `indirect_ptr_i32_ptr_arg_param_call.c` and
  `indirect_ptr_i32_ptr_arg_local_call.c` as native asm with the expected
  callee preserve into `t0`, leading `ptr` arg move into `a0`, middle `i32`
  arg move into `a1`, trailing `ptr` arg move into `a2`, and final
  `jalr ra, t0, 0`, while `branch_if_eq.c`, `call_helper.c`,
  `local_arg_call.c`, and the earlier one-arg through eight-arg indirect-call
  plus `ptr`-shaped and `two_arg_*` direct-call sentinels stayed green beside
  them
  proof command attempted:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  proof log:
  `test_after.log`
  proof status:
  the delegated build succeeded and the new riscv64 route tests passed under
  the shared backend surface, increasing the backend subset from `365` to
  `367` tests while the new pair completed as tests `#269` and `#270`; the
  exact delegated proof command still returned non-zero because the broad
  `^backend_` subset remains at `225` standing failures out of `367`, so the
  blocker is outside the owned files even though the new adjacent
  `ptr, i32, ptr -> i32` family is now covered and the next remaining
  adjacent family is `ptr, ptr, i32 -> i32`
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
