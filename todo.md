Status: Active
Source Idea Path: ideas/open/352_aarch64_block_label_emission_ordering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative And Reclassify Residuals

# Current Packet

## Just Finished

Plan Step 4 completed: `00176` was rerun after the AArch64 branch/label
repair, and the remaining runtime segfault was localized without
implementation changes.

Reclassification: the residual `00176` segfault is a new first bad fact
outside idea 352 block label/emission ordering.

Prepared/generated control-flow evidence:

- Prepared `partition` still has the expected CFG after preparation:
  `for.cond.2` true=`block_2` false=`block_3`, `block_2` true=`block_4`
  false=`block_5`, `block_3` returns, `block_4` branches to `block_5`, and
  `block_5` branches to the loop latch.
- Generated `partition` preserves the repaired branch/label path:
  `.LBB90_2` compares loop `i` against right and branches to `.LBB90_3` or
  `.LBB90_4`; `.LBB90_3` compares selected `array[i]` against pivot and emits
  `b.lt .LBB90_6` / `b .LBB90_7`; `.LBB90_6` contains the in-loop `swap(i,
  index)` body; `.LBB90_7` branches to the loop latch. The previous unlabeled
  later body after the return epilogue is no longer the first bad fact.

New bad fact evidence:

- Prepared `partition` entry stores `%p.left` into `%lv.pivotIndex`,
  `%lv.index`, and `%lv.i`, with `%p.left` home=`x0` and `%p.right` home=`x1`.
- Generated `partition` does not publish those incoming formals into the local
  frame slots before use. At `partition+16` it executes `ldr w9, [sp]` and
  stores that stale stack value to `[sp, #16]` for the first local load; later it
  reads `[sp, #140]` into `[sp, #4]`, and the loop/header/body read `[sp, #12]`
  and `[sp, #8]` for `i` and `index` without matching initialization from
  `w0`.
- GDB on the first `partition(0, 15)` call confirmed `w0=0`, `w1=15`, but the
  first generated local load read `[sp] = -6128` and stored that value to the
  local spill slot. The first return to `quicksort` produced `x0=-136721336`
  instead of a partition index in `[0, 15]`; the next recursive call then used
  `left=-136721335`, `right=15`.
- The eventual segfault is stack exhaustion/deep recursion ending in `swap`,
  with debugger registers showing nonsensical swap arguments (`x0=2863407775`,
  `x1=4146082357`) and a very deep `quicksort` backtrace. This follows from the
  bad partition index, not from missing block labels or return ordering.

## Suggested Next

Next packet: route a separate AArch64 local/formal frame-slot publication
initiative. Start with a focused case where a function stores incoming scalar
formals into local slots, reads them before and after a call, and returns a
local-derived value. Prove whether the owner is `store_local`/`load_local`
lowering for formal-backed values, frame-slot addressing, or publication moves
from register homes into local stack slots.

## Watchouts

- Do not widen idea 352 to cover this residual; the current evidence crosses
  from block label/emission ordering into local/formal value publication and
  frame-slot initialization.
- The generated `swap` body also shows suspicious stack-slot reads for selected
  array update values, but the earliest concrete bad fact for this packet is
  already in `partition` entry before the repaired `block_2` branch path.
- Keep the repaired `.LBB90_6`/`.LBB90_7` path intact if a later packet uses
  `00176` as a representative again.
- Avoid any `00176`-, `partition`-, block-number-, or label-suffix-specific
  workaround. The next repair should be a semantic local/formal publication
  rule with focused backend coverage.

## Proof

Delegated proof run for this localization packet:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_branch_control_lowering|backend_aarch64_return_lowering|backend_aarch64_function_traversal|backend_prepare_block_only_control_flow|c_testsuite_aarch64_backend_src_00176_c)$' | tee test_after.log`

Result: build had no work to do; CTest subset was 5/6. Passing tests:
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_branch_control_lowering`,
`backend_aarch64_return_lowering`,
`backend_aarch64_function_traversal`, and
`backend_prepare_block_only_control_flow`. Failing representative:
`c_testsuite_aarch64_backend_src_00176_c` with
`[RUNTIME_NONZERO] ... exit=Segmentation fault`.

Additional localization evidence collected from generated assembly,
`--dump-prepared-bir --mir-focus-function partition`, and GDB batch runs. These
debugger/dump probes were evidence-only and did not replace the delegated proof.

Proof log: `test_after.log`.
