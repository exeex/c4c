# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Extract The Shared Text Table Layer
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Reordered the new identity, CFG, and phi initiatives and switched the active
lifecycle state to idea 64 so the shared text/name-table substrate lands before
idea 62 CFG ownership work and idea 63 phi completeness work.

## Suggested Next

Inspect `src/frontend/string_id_table.hpp`, `src/frontend/link_name_table.hpp`,
and current prepared symbolic-string consumers to define the extraction target
and the first semantic id tables needed by backend/prealloc.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not silently absorb idea 62 CFG
  generalization or idea 63 phi completeness work into the first packet.
- Do not replace symbolic strings with raw `TextId` as the final contract.
- Follow the `LinkNameTable` model: semantic ids need dedicated domain meaning,
  not just canonical text storage.

## Proof

Lifecycle switch only. No build or test run was required for this packet.
