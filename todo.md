Status: Active
Source Idea Path: ideas/open/target-neutral-publication-plan-record.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Neutral Publication Plan Data

# Current Packet

## Just Finished

Completed Step 2: `Define Neutral Publication Plan Data`.

Added `prepare::PreparedScalarPublicationPlan` and
`prepare::plan_prepared_scalar_publication` under `src/backend/prealloc/`.
The record carries the source BIR value, Prepared value id/name, destination
`PreparedValueHome`, home kind, neutral storage encoding, optional block-entry
publication fact, and a target hook kind.

The helper can represent these scalar publication intents without AArch64
codegen dependencies:
- register home -> `RegisterHome` hook and `Register` storage encoding
- stack slot home -> `StackSlotHome` hook with slot/offset/size/align facts
- rematerializable immediate -> `RematerializableImmediate` hook with i32/F128
  payload facts
- pointer base plus offset -> `PointerBasePlusOffset` hook with base value and
  byte-delta facts

Added `backend_publication_plan_record`, a focused MIR backend test that
constructs those four home shapes directly and verifies the neutral record
shape. The test includes only prealloc/publication-plan headers and does not
consume AArch64 codegen headers, register views, memory operands, mnemonics,
scratch indexes, or machine instruction records.

## Suggested Next

Execute Step 3 by adapting one AArch64 scalar publication path to consume
`prepare::PreparedScalarPublicationPlan` while preserving current emitted
behavior.

Start with the early home-selection/publication decision in
`emit_value_publication_to_register`; keep target-local register parsing,
scalar views, stack memory operands, immediate materialization, scratch choice,
and machine instruction construction in AArch64 codegen.

## Watchouts

The new neutral record intentionally preserves `PreparedValueHome::register_name`
only behind the destination-home pointer. AArch64 should treat that as prepared
identity input and continue validating/spelling the physical operand locally.

Do not move recursive producer lowering, binary/select/cast materialization,
global-load sequences, narrow-store recovery, register alias checks, memory
operand spelling, or machine instruction construction into prealloc.

## Proof

Ran supervisor-selected proof:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: build succeeded and `157/157` backend tests passed, `0` failed.
Proof log: `test_after.log`.
