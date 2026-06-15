Status: Active
Source Idea Path: ideas/open/275_riscv_memory_accesses_stale_publication_fixture_support.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Fixture Binding

# Current Packet

## Just Finished

No executor packet has run for this active plan yet.

## Suggested Next

Start Step 1 - Locate The Fixture Binding.

## Watchouts

- Do not accept synthetic post-construction mutation as fixture support.
- Preserve the compatible `LoadLocal` fixture and exact `lw a1, 12(s2)` output.
- Keep final stale-publication fail-closed proof out of scope unless the
  supervisor explicitly folds it into this runbook.

## Proof

Lifecycle activation only. No build or test proof required.
