Status: Active
Source Idea Path: ideas/open/239_aarch64_intrinsic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Print Structured CRC And Vector Intrinsic Records

# Current Packet

## Just Finished

Completed `plan.md` Step 4 structured printer spelling for the ready AArch64
intrinsic machine-node families only.

- Selected `Crc/Crc32W` records now print `crc32w` from explicit W-register
  fields.
- Selected `VectorMemory/VectorLoad` records now print `ld1 {vN.16b}, [xN]`
  from the result vector register and pointer memory base register.
- Selected `VectorOperation/VectorAdd` records now print NEON vector `add`
  from explicit `vN.16b` result/lhs/rhs register fields.
- Incomplete, non-selected, wrong-shape, wrong-register-bank, or
  scalar-FP-carrier-shaped CRC/vector records still fail closed without
  instruction lines.
- Existing scalar `FAbs` selected dispatch remains green.

## Suggested Next

Execute `plan.md` Step 5 for the remaining barrier/cache/pause-hint/
builtin-address intrinsic families. The next packet should either prove
complete carrier authority and implement those families with the same
record-first pattern, or record the missing authority as blockers and request a
dependency split.

## Watchouts

- CRC/vector printer support is intentionally limited to selected structured
  records; do not widen it to intrinsic spelling alone, generic call plans,
  archived scratch registers, or unselected payloads.
- Vector load currently accepts the selected zero-offset pointer form used by
  the carrier path; offset/post-index forms need their own authority before
  being printed.
- Barrier/cache/pause-hint/builtin-address work is blocked on upstream semantic
  and prepared intrinsic carriers; adding name-only records for these families
  would be route drift.
- Keep unsupported x86-only and incomplete-fact paths fail-closed.

## Proof

Passed delegated proof; output preserved at `test_after.log`.

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_machine_printer|aarch64_instruction_dispatch|aarch64_target_instruction_records)'; } 2>&1 | tee test_after.log`

`git diff --check` passed.
