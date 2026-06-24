Status: Active
Source Idea Path: ideas/open/344_rv64_pointer_typed_select_publication_self_move.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Localize The Self-Move

# Current Packet

## Just Finished

Lifecycle switched from the parked consteval inline asm template string runbook
to the unrelated RV64 backend-route blocker found during Step 6 broad proof.

## Suggested Next

Execute Step 1: reproduce
`backend_codegen_route_riscv64_pointer_typed_select_publication`, inspect the
generated `mv t0, t0`, and identify the backend path that emits or fails to
remove the self-move.

## Watchouts

- Keep the forbidden-snippet contract for `mv t0, t0` intact.
- Do not treat `test_baseline.new.log` as accepted proof unless the supervisor
  explicitly dispositions it.
- Do not special-case the failing test name, fixture path, or exact emitted
  assembly.
- The previous source idea remains open and parked until this broad-proof
  blocker is resolved or explicitly dispositioned.

## Proof

No implementation validation was run by the plan owner. The lifecycle switch is
based on the delegated full-suite result:
`backend_codegen_route_riscv64_pointer_typed_select_publication` failed because
generated RV64 assembly still contained forbidden snippet `mv t0, t0`.
