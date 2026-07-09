Status: Active
Source Idea Path: ideas/open/622_repeated_stack_destination_fan_in_order_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Prepared/Prealloc Destination Authority Facts

# Current Packet

## Just Finished

Step 2 publication slice completed for the
`stack_destination_register_fan_in` authority family.

Added a structured prepared move-bundle destination fan-in fact that is
published only when the bundle and all moves already carry
`PreparedMoveAuthorityKind::StackDestinationRegisterFanIn`. The fact exposes
authority kind, prepared owner `prepared_stack_destination_register_fan_in`,
select-materialization preserved-stack-fallback semantics, destination value,
destination stack home/slot/offset, all source homes, and candidate order. The
focused backend consumer contract now asserts that a legal authorized
select-materialization stack-destination bundle publishes this fact for two
register sources plus the preserved stack fallback.

Fail-closed behavior is preserved: missing authority still reports
`ambiguous_non_parallel_multi_source_stack_destination`, unsupported authority
still reports
`unsupported_non_parallel_multi_source_stack_destination_authority`, and
bundle/move disagreement or malformed fan-in facts report
`mismatched_stack_destination_register_fan_in_move_authority`.

## Suggested Next

Implement Step 3 RV64 consumption for the published
`stack_destination_register_fan_in` fact. RV64 should accept only the explicit
prepared fact shape and should continue to reject missing, malformed,
unsupported, or ambiguous repeated stack-destination bundles without inferring
order from move rows.

## Watchouts

This slice did not add RV64 acceptance and did not special-case
`src/pr71631.c`. The currently published authority family is the legal
select-materialization stack-destination shape with two or more register
sources and one preserved stack fallback. Repeated stack destinations inside
out-of-SSA parallel-copy bundles still need an explicit producer fact or a
separate authority family before RV64 can consume them.

## Proof

Ran:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: passed. `test_after.log` is preserved as the proof log.

Formatting note: attempted `clang-format -i` on the touched C++ files, but
`clang-format` is not installed in this environment.
