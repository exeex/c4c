# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed Step 1 packet work in `src/backend/prealloc/prealloc.hpp` and
`src/backend/prealloc/legalize.cpp` by adding explicit branch-condition kind
metadata, edge-transfer records for join traffic, and shared lookup helpers
for prepared control-flow consumers.

## Suggested Next

Start Step 2 by teaching shared lowering to classify loop-carried join traffic
with the new contract so later x86 consumers can stop inferring loop meaning
from CFG shape.

## Watchouts

- Do not activate umbrella idea 57 as executable work.
- Do not pull in idea 59 instruction-selection scope.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth.
- X86 still reads legacy `incomings` and branch fields, so follow-up packets
  should migrate consumers in bounded slices rather than deleting compatibility
  fields early.

## Proof

Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_(prepare_phi_materialize|x86_handoff_boundary)$'`.
Proof passed and was recorded in `test_after.log`.
