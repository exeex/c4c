Status: Active
Source Idea Path: ideas/open/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Guard AArch64 Machine Dispatch Boundaries

# Current Packet

## Just Finished

Plan Step 4 - Guard AArch64 Machine Dispatch Boundaries completed for the
complete no-result AArch64 DMB barrier representative.

Added and proved boundary coverage:

- Kept the existing dispatch boundary proving a complete `BarrierDmb` prepared
  carrier emits only the return node, reports `unsupported_intrinsic_family`,
  and does not select scalar, CRC, vector, or call machine records.
- Added the matching machine-printer guard showing a complete `BarrierDmb`
  carrier cannot be printed through the selected scalar intrinsic path and
  produces no machine instruction lines.
- Preserved existing scalar/CRC/vector selected-machine dispatch and printer
  coverage; no DMB selected record, printer spelling, call fallback, or
  atomic-fence substitution was added.

## Suggested Next

Proceed to the final validation/closure packet for this active plan: run the
supervisor-selected broader proof, then ask the plan owner to decide whether
the barrier/cache/hint/builtin carrier plan is complete or needs a follow-up
split for non-DMB families.

## Watchouts

- Barrier selected-machine support remains absent by design. Do not treat the
  closed dispatch/printer boundary as selected DMB lowering.
- The complete prepared DMB carrier still has no result home and no value home
  for the immediate operand; the immediate value itself is the carrier
  authority.
- Cache-maintenance, pause/hint, and builtin-address representatives remain
  outside this completed DMB packet and should be handled only by lifecycle
  decision or later source intent.

## Proof

Delegated proof for the Step 4 AArch64 machine dispatch/printer boundary
packet:

- `set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_instruction_dispatch|aarch64_machine_printer|prepared_printer)'; } 2>&1 | tee test_after.log`
- Result: passed, 3/3 tests green.
- Proof log: `test_after.log`
