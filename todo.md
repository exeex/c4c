Status: Active
Source Idea Path: ideas/open/577_rv64_20000622_1_runtime_abort_after_call_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Coverage For The Classified Family

# Current Packet

## Just Finished

Step 2 added focused RV64 object-route coverage in
`tests/backend/mir/backend_riscv_object_emission_test.cpp` for the classified
family. The new prepared fixture models three incoming GPR parameters where
the third pointer parameter is materialized through a `ptrtoint` local in
`s1`, survives a nested same-module call, and is then moved from `s1` into
`a0` for a later same-module call argument.

The object-emission harness can prove the emitter honors correct prepared
facts for this semantic shape, but it cannot express the full pre-fix compiler
failure because it does not run the frontend/preparation path that selected the
wrong incoming value in the representative. Kept the suite green and recorded
this as prepared-facts coverage rather than a red end-to-end reproduction.

## Suggested Next

Delegate Step 3: repair the preparation/lowering path that classifies
`d = (long)c` so the pointer-to-integer local is sourced from incoming `a2`
and preserved across the nested same-module call before later GPR argument
publication.

## Watchouts

- The Step 2 object-emission test is not a full end-to-end red reproducer; it
  validates the RV64 prepared object route once the source/prepared facts are
  correct.
- Do not edit expectations, unsupported markers, allowlists, runtime comparison
  behavior, or the gcc_torture runner.
- Do not add filename-specific handling for `src/20000622-1.c`.
- Preserve the same-module call/result behavior repaired under 572.
- The classified bad value is the `baz` local/parameter materialization for
  `d = (long)c`; `bar(a, 1)` still returns 1 into the second `foo` argument.
- Step 3 should target semantic source selection/materialization rather than
  changing runtime comparison files or exact generated symbol names.

## Proof

Proof log: `test_after.log`.

Command run:

- `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1` -> passed
