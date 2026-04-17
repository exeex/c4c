Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by naming the first local-memory lane as
stack-resident scalar slots reached through local pointer chains, with a
bounded proving cluster of `c_testsuite_x86_backend_src_00005_c`,
`c_testsuite_x86_backend_src_00020_c`, and
`c_testsuite_x86_backend_src_00103_c`.

## Suggested Next

Implement the first code packet in shared `lir_to_bir` memory lowering for
stack-local scalar/object slots, local `&slot` address formation, one or two
levels of local pointer dereference for addressed loads/stores, and only
fixed-offset stack/object addressing needed by the selected cluster.

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

## Proof

Ran `ctest --test-dir build -L x86_backend --output-on-failure >
test_after.log 2>&1` to refresh the current failure surface. The proof is
adequate for this non-code Step 1 packet because it captured the current
`x86_backend` local-memory notes used to choose the lane and proving cluster.
Proof log: `test_after.log`.
