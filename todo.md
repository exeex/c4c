# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Migrate direct dispatch-producer consumers

## Just Finished

- Plan Step 2.2 removed direct same-block select-producer discovery and the
  generated edge-publication producer-lookup fallback from
  `dispatch_producers.cpp`. Select adaptation and publication queries now
  require the owned, attached prepared lookups and fail closed when the common
  producer record is absent or inconsistent.

## Suggested Next

- Execute Plan Step 2.3 against the select/comparison value-home lookup
  rebuilding family.

## Watchouts

- The focused select-producer fixture now attaches the common prepared lookup
  authority it consumes. The pre-existing selected-global-load fused-branch
  stale-stack-home failure remains unchanged and is outside this packet's
  owned implementation files.

## Proof

- `cmake --build --preset default` passed. The delegated exact four-test CTest
  subset remains at the recorded baseline of 3/4 passing; only the pre-existing
  `backend_aarch64_instruction_dispatch` selected-global-load fused-branch
  stale-stack-home expectation fails. Output is recorded in `test_after.log`.
