Status: Active
Source Idea Path: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit the Multi-Block Rejection Boundary

# Current Packet

## Just Finished

Lifecycle activation only. No implementation has started.

## Suggested Next

Execute Step 1 as an analysis/audit packet:

- Prepare a temporary allowlist containing:
  - `src/20000113-1.c`
  - `src/20000205-1.c`
  - `src/20010119-1.c`
  - `src/20030216-1.c`
- Run:

```sh
CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-multiblock-allowlist.txt \
  scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

- Inspect the per-case logs and current RV64 prepared object route.
- Classify each rejection into exactly one primary layer:
  BIR/prepared-BIR shape, RV64 target block preservation, MIR/pseudo lowering,
  object emission single-block gate, or unrelated globals/strings/ABI blocker.
- Record a concise boundary table here before any backend implementation edit.

## Watchouts

- Do not edit backend implementation during Step 1.
- Do not move CFG semantics into MIR, assembler, or object emission.
- Do not ask the assembler to understand loops, if/else, switch, liveness, or
  inline-asm operand relationships.
- If a representative case is blocked by globals/strings/data sections, mark it
  as an idea 357 blocker instead of forcing it into 356.

## Proof

Not run for activation. Step 1 proof is the narrow audit command above plus a
recorded boundary table.
