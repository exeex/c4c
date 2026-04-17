Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by extending the prepared x86 handoff from the
single-block local-slot pointer-chain lane to a bounded local-slot
compare-against-zero guard chain with early returns and one intervening local
store. The selected proving cluster now moves together:
`c_testsuite_x86_backend_src_00005_c`,
`c_testsuite_x86_backend_src_00020_c`, and
`c_testsuite_x86_backend_src_00103_c` all pass on the x86 backend path.

## Suggested Next

Start `plan.md` Step 4 by measuring the truthful same-family effect beyond the
three proving probes: run the `x86_backend` checkpoint and inspect the next
nearby local-slot cases before widening the x86 handoff beyond the current
acyclic guard-chain family.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Keep the first slice in the local-memory family with already-supported
  control flow.
- Shared `lir_to_bir` capability growth should explain the result more than any
  x86-local patching.
- The chosen cluster is intentionally the simplest local-scalar lane:
  `00005` exercises local scalar plus `int*`/`int**` addressed load and store,
  `00020` is the straight-line `int*`/`int**` load-only variant, and `00103`
  keeps the same lane but adds `void*`/`void**` casts over a local scalar slot.
- Leave array-walk and element-indexed `gep` shapes such as `00032`, `00037`,
  `00073`, and `00130` out of the first packet unless the selected scalar lane
  cannot lower without the same fixed-offset machinery.
- Leave aggregate or union-backed local stores such as `00042` and `00046`,
  VLA/control-flow-heavy `alloca` shapes such as `00143` and `00207`, and
  global-data or `printf`-heavy mixed cases such as `00163`, `00176`,
  `00182`, and `00205` outside the first slice.
- The x86 handoff remains intentionally narrow: x86_64, parameter-free,
  local-slot-only, acyclic compare-against-zero guard chains with immediate
  i32 return leaves and no shared-join reconstruction. This is enough for the
  selected family lane but not a claim of broader structured control-flow
  support.
- The renderer now accepts local `&slot` pointer seeding, repeated local-slot
  pointer-chain loads, one bounded local scalar store, and follow-on
  compare-against-zero blocks. Do not widen it into array walking, global
  address families, or general CFG joins without a new packet.
- `tests/backend/backend_lir_to_bir_notes_test.cpp` did not need note changes
  for this packet because the semantic lowering boundary stayed the same; the
  progress was entirely in the x86 prepared-module consumer.

## Proof

Ran the delegated proof exactly:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -R '^backend_lir_to_bir_notes$' --output-on-failure >> test_after.log 2>&1 && ctest --test-dir build -R '^backend_x86_handoff_boundary$' --output-on-failure >> test_after.log 2>&1 && ctest --test-dir build -R '^c_testsuite_x86_backend_src_(00005|00020|00103)_c$' --output-on-failure >> test_after.log 2>&1`.
`backend_lir_to_bir_notes` passed, `backend_x86_handoff_boundary` passed, and
`c_testsuite_x86_backend_src_00005_c`,
`c_testsuite_x86_backend_src_00020_c`, and
`c_testsuite_x86_backend_src_00103_c` all passed. Proof log:
`test_after.log`.
