# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Complete foundational identities, types and module containers

## Just Finished

- Plan Step 2 packet 2C completed: added source-identity-backed ordinary value
  reservation and definition, a typed `ConstantId` store for current structured
  LIR `LirConstInt` and `LirConstFloat`, and immutable constant/source/value
  lookup.
- Integer payloads remain exact signed bits, floating payloads retain the exact
  source `double` bit pattern, and constants participate in ordinary `ValueId`
  def-use without fabricated instructions. Foundation verification rejects
  unresolved, mistyped, foreign, duplicate, or conflicting definitions and
  incoherent source indexes before publication.

## Suggested Next

- Execute the next bounded Step 3A packet only: establish typed Raw-BIR identity,
  storage, views, import, and nearby verification for the first coherent
  globals/string-pool family slice without absorbing externs or initializers.

## Watchouts

- The importer still preserves the zero-parameter, void-definition, first-entry-
  block boundary. Complete signatures, CFG, stack objects, non-constant ordinary
  instructions, and terminators remain later packets.
- Current constants use authoritative function-local `LirValueId`; current
  structured inline asm retains its distinct `LirOperand` semantic binding.
  Do not bridge those domains through rendered-name parsing in later work.
- Globals, strings, externs, symbols, and initializers remain unsupported until
  their Step 3 packets land; Step 3A should not silently absorb the whole family.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'`.
- The focused post-change run passed 1/1 tests. Canonical proof log:
  `test_after.log`.
