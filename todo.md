# Current Packet

Status: Active
Source Idea Path: ideas/open/26_riscv_prepared_edge_publication_pointer_base_register_consumer.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Handoff or Close

## Just Finished

Completed Step 5 validation summary and Step 6 handoff/closure decision for the
RISC-V pointer-base prepared edge-publication route.

Supported behavior is exactly scoped to `PointerBasePlusOffset -> Register`
when the pointer base resolves through shared prepared value-home lookup
authority to a register home and the byte delta is present and fits RISC-V
signed-12-bit `addi`. The consumer emits `addi <dst>, <base>, <delta>` for
non-zero deltas and zero-delta `mv <dst>, <base>` through shared
`edge_publications` authority.

Validation evidence is accepted for closure: the focused RISC-V prepared edge
publication subset passed 5/5, the matching regression guard passed, broader
backend validation passed 163/163, the full-suite baseline remains accepted at
3411/3411, and `review/idea26_riscv_pointer_base_edge_publication_review.md`
reported no blocking findings.

## Suggested Next

Recommend closure of idea 26 if the supervisor accepts the scoped handoff and
validation evidence.

## Watchouts

Do not overclaim the route. Source-to-`StackSlot` destinations, stack-source
policy broadening, non-register bases, missing or unresolved base names, missing
or out-of-range deltas, and wider materialization remain unsupported and
fail-closed.

## Proof

Docs/handoff-only packet. No tests were run and no `test_after.log` was created
by this packet.

Previously accepted focused proof:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_codegen_route_riscv64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`.

Result: PASS, 5/5 selected tests passed. Proof log: `test_after.log`.

Supervisor ran the matching regression guard against the focused
`test_before.log` and focused `test_after.log` with non-decreasing pass count
allowed because this packet extended an existing CTest binary. Result: PASS
with 5/5 before and 5/5 after, no new failures.

Broader backend validation is accepted at 163/163. The full-suite baseline is
accepted at 3411/3411. Reviewer result: no blocking findings.
