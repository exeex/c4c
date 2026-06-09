# 146 Call-argument materialization call owner

## Goal

Move call-argument source-producer materialization policy from
`prepared_lookups.*` toward call ownership.

## Why This Exists

Idea 141 found that call-argument materialization is call-specific policy over
same-block source producers. It should depend on a shared same-block
materialization owner rather than remain as a residual prepared lookup API.

## In Scope

- Move `PreparedCallArgumentSourceProducerMaterialization`.
- Move `prepared_call_argument_binary_producer_opcode_is_materializable`.
- Move `find_prepared_call_argument_source_producer_materialization`.
- Place the public owner in `src/backend/prealloc/calls.hpp` with dependency on
  the same-block/source-producer materialization owner from idea 144.

## Out Of Scope

- Moving general same-block scalar producer helpers into call ownership.
- Changing call-plan construction or ABI lowering.
- Moving AArch64 call emission, register spelling, or scratch policy into
  prealloc.
- Mixing ABI move-bundle ownership into this route.

## Acceptance Criteria

- Call-argument materialization declarations are call-owned and reuse shared
  same-block/source-producer logic.
- No AArch64-only shortcut replaces semantic prepared facts.
- Proof includes `cmake --build --preset default` and
  `ctest --test-dir build -R '^backend_' --output-on-failure`.

## Reviewer Reject Signals

- Call ownership duplicates same-block materialization logic instead of
  depending on the shared owner.
- The route turns call-argument lookup into an AArch64-only shortcut.
- ABI move-bundle work is mixed in without a clear staged boundary.
