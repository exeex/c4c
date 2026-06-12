Status: Active
Source Idea Path: ideas/open/225_phase_e2_fused_compare_operand_producer_helper_private_pass_context.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The AArch64 Comparison-Lowering Consumer

# Current Packet

## Just Finished

Step 2 - Extract The Private Agreement Reader is complete.

Implemented `detail::FusedCompareOperandProducerIdentityPassContext` and
`detail::read_agreeing_route7_fused_compare_operand_producer_facts(...)` in
`src/backend/mir/aarch64/codegen/comparison.cpp`. The selected Route
7/prepared fused compare operand producer identity read now runs through this
private pass-context-style boundary after retained public prepared facts are
available.

The public prepared fallback remains unchanged: absent route data, invalid
references, duplicate/conflict cases, mismatches, unfused cases, and
non-agreement all keep returning the existing prepared facts or `std::nullopt`
from the public prepared helper path. No branch target selection, suffix
mapping, fused legality, hazards, emitted-register state, final assembler
order, printer/debug rows, wrappers, helper-oracle strings, or expected strings
were changed.

## Suggested Next

Execute Step 3 from `plan.md`: migrate the selected AArch64
comparison-lowering consumer accounting to the private boundary while keeping
target branch policy and all retained public/helper-oracle surfaces explicit.

## Watchouts

- The private boundary currently lives in the AArch64 comparison-lowering
  implementation and wraps only the selected Route 7/prepared agreement read.
- Keep branch target policy, suffix mapping, fused legality, hazards,
  emitted-register state, final assembler order, printer/debug rows, wrappers,
  helper-oracle strings, and expected strings out of route authority.
- Treat unsupported downgrades, baseline refreshes, helper renames, wrapper
  moves, timeout masking, and expectation rewrites as non-progress.
- Preserve prepared fallback for absent-route, invalid-reference,
  duplicate/conflict, mismatch, unfused, and non-agreement paths.
- Do not migrate `fused_compare_uses_selected_operand(...)` or BIR/Route 7
  aggregate lookup APIs in Step 2; they are retained branch-support and
  aggregate/oracle surfaces.
- Do not change helper-oracle names, statuses, failure-mode strings, machine
  printer rows, branch-control expected strings, or x86 wrapper behavior.

## Proof

Supervisor-selected proof ran exactly:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_machine_printer' > test_after.log 2>&1
```

Result: passed, 3/3 tests.

Test subset:

- `backend_prepared_lookup_helper`
- `backend_aarch64_branch_control_lowering`
- `backend_aarch64_machine_printer`

Proof log path: `test_after.log`
