Status: Active
Source Idea Path: ideas/open/225_phase_e2_fused_compare_operand_producer_helper_private_pass_context.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Helper-Family Proof

# Current Packet

## Just Finished

Step 3 - Migrate The AArch64 Comparison-Lowering Consumer is complete.

Confirmed with `c4c-clang-tool-ccdb` that the selected production
comparison-lowering caller is
`lower_prepared_conditional_branch_terminator(...)`, which calls the local
`find_prepared_fused_compare_operand_producer_facts(...)` wrapper. That wrapper
is now the only caller of
`detail::read_agreeing_route7_fused_compare_operand_producer_facts(...)`, so
the selected Route 7/prepared agreement identity read is behind the private
pass-context-style boundary instead of being duplicated directly in the
production lowering path.

No code edit was required for this packet. Retained public surfaces remain
intentional: the two-argument prepared fallback calls are kept inside the local
wrapper for prepared-only, absent-route, invalid-reference,
duplicate/conflict, mismatch, unfused, and non-agreement behavior; public
helper-oracle/test coverage remains on the prepared helper surface; aggregate
BIR/Route 7 lookup APIs remain public for retained non-production and
branch-support accounting; `fused_compare_uses_selected_operand(...)` remains
AArch64 branch-support policy, not the selected identity-read migration target.
Target branch policy, suffix mapping, legality checks, hazards,
emitted-register state, final instruction order, final assembler rows,
printer/debug output, wrappers, x86 behavior, aggregate lookup surfaces, and
expected strings were unchanged.

## Suggested Next

Execute Step 4 from `plan.md`: add focused helper-family proof for the private
agreement boundary and required fallback modes without changing expected
strings, wrapper output, helper-oracle statuses, or printer/debug rows.

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
  aggregate lookup APIs in Step 4; they are retained branch-support and
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
