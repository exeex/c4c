Status: Active
Source Idea Path: ideas/open/515_rv64_bankless_conversion_adjacent_stack_slot_moves.md
Source Plan Path: plan.md
Current Step ID: 5-6
Current Step Title: Add Focused Backend Coverage; Validate And Handoff

# Current Packet

## Just Finished

Steps 5 and 6 - Add Focused Backend Coverage; Validate And Handoff completed
the idea 515 coverage/validation handoff after the committed Step 4
fail-closed diagnostic slice.

Step 5 coverage is satisfied by the focused RV64 backend object-emission tests
from the committed Step 4 slice. They cover the accepted same-scalar prepared
stack-slot to stack-slot copy path, the preserved register-source
stack-destination conversion rejection, and the new stack-source
stack-destination conversion-adjacent rejection with mismatched known scalar
types and no explicit conversion materialization contract. The rejection test
asserts both text-object builder and ELF writer diagnostics and proves the path
does not fall through to `generic_move_bundle_materialization_failed`.

Step 6 validation is fresh in `test_after.log`. The focused `pr69447.c` object
probe reports the owner-specific `i16 -> i64` stack-source/stack-destination
conversion-adjacent diagnostic, exits nonzero as unsupported, and leaves no
output object behind. The backend subset is green, and the final
`git diff --check -- todo.md` proof covers this handoff update.

## Suggested Next

Hand off to the plan owner to decide closure for idea 515. From this executor
packet, the source idea appears ready for plan-owner closure: bankless
classification was repaired earlier, conversion-adjacent stack-slot movement
without explicit conversion authority now fails closed with an owner-specific
diagnostic, focused backend coverage exists, and fresh validation is recorded.

## Watchouts

Residual risks are bounded to future work, not this idea's acceptance path:
actual stack-slot conversion materialization remains unsupported unless a
future source idea defines an explicit conversion contract; missing scalar type
authority, non-stack homes, multi-source/non-parallel stack destinations,
unsupported banks or widths, and parallel-copy or cycle-temp stack movement
remain fail-closed rather than widened by this route. No implementation change
was needed in this handoff packet.

## Proof

Proof is captured in `test_after.log`:

- `cmake --build --preset default`
- `rm -f /tmp/pr69447_step6.o; ./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/pr69447.c -o /tmp/pr69447_step6.o 2>&1; status=$?; printf '\nEXIT_STATUS=%s\n' "$status"; if [ -e /tmp/pr69447_step6.o ]; then printf 'OBJECT_PRESENT=yes\n'; else printf 'OBJECT_PRESENT=no\n'; fi`
  reports `unsupported_prepared_move_bundle_classification`,
  `diagnostic_owner=prepared_move_bundle_classifier`,
  `source_type=i16`, `destination_type=i64`,
  `fragment_status=producer_classification_rejected_stack_source_stack_destination_conversion_adjacent_move`,
  `EXIT_STATUS=2`, and `OBJECT_PRESENT=no`.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- `git diff --check -- todo.md`
