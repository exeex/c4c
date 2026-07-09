Status: Active
Source Idea Path: ideas/open/642_rv64_global_residual_runtime_mismatch_research.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish The Research Artifact

# Current Packet

## Just Finished

Completed plan Step 3, `Publish The Research Artifact`, for
`src/pr79737-2.c`.

Published
`docs/runtime_mismatch_ownership/04_global_residual_runtime_mismatch.md` and
linked it from `docs/runtime_mismatch_ownership/index.md`.

The durable classification is packed bitfield/global-object layout and
bitfield access lowering, not true runtime support. The artifact records the
Step 1 rerun command, Step 1/2 artifact paths, candidate-owner table, final
9-byte packed global versus 12-byte C4C global evidence, and one focused
follow-up recommendation/proof surface.

## Suggested Next

Start Step 4, `Final Lifecycle Review`, and decide whether idea 642 is
complete or should spawn a separate packed bitfield/global-layout
implementation idea.

## Watchouts

- Do not implement code changes inside this research idea.
- Keep any implementation recommendation separate from idea 642. A fix belongs
  to a packed bitfield/global-layout follow-up, not to this research packet.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime comparison behavior, or accounting.

## Proof

No new build proof was required for this docs-only research publication.
Preserved the existing Step 1 `test_after.log` and used the existing row
artifacts under `build/rv64_gcc_c_torture_backend/src_pr79737-2.c/`.

Existing Step 2 inspection artifacts used under `build/agent_state/`:
- `build/agent_state/642_step2_symbols.txt`
- `build/agent_state/642_step2_relocations.txt`
- `build/agent_state/642_step2_c4c_o_disasm.txt`
- `build/agent_state/642_step2_c4c_bin_disasm.txt`
- `build/agent_state/642_step2_clang_bin_disasm.txt`
