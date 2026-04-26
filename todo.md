Status: Active
Source Idea Path: ideas/open/119_bir_block_label_structured_identity_for_assembler.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Render Labels Through Structured Identity

# Current Packet

## Just Finished

Step 4 printer rendering completed in `src/backend/bir/bir_printer.cpp` and
`tests/backend/backend_prepare_structured_context_test.cpp`.

The BIR printer now routes block labels through a small helper that resolves
`BlockLabelId` with `bir::Module::names.block_labels` and falls back to the raw
string when no structured spelling is available. The helper is used for block
headers, `bir.br`, `bir.cond_br`, `bir.phi`, semantic phi observations, and
`MemoryAddress::BaseKind::Label` rendering without moving ownership of the
block-header trailing colon.

Added direct tests proving id-preferred rendering for every updated surface and
raw-string fallback behavior for the same surfaces.

## Suggested Next

Next coherent packet: move the next downstream label consumer to structured
block identity, keeping raw-string fallback behavior until a later cleanup idea
removes legacy strings.

## Watchouts

- Existing focused BIR dump tests stayed byte-stable under the delegated proof.
- Keep raw label strings as fallbacks until a later cleanup idea removes them.
- Do not expand this plan into MIR target codegen migration.
- Same-spelled labels such as `entry` across functions will share an id under
  the module table, which matches the prepared backend spelling-table model.
- The printer helper intentionally treats an absent/invalid id as fallback-only
  and does not try to intern or repair labels during rendering.

## Proof

Passed. Proof log: `test_after.log`.

Command run exactly:
`( cmake --build build-backend --target c4cll backend_prepare_structured_context_test && ctest --test-dir build-backend/tests/backend -R '(backend_prepare_structured_context|backend_cli_dump_bir_(is_semantic|vla_goto_stackrestore_cfg|focus_block_entry_00204|focus_value_t1_00204))$' --output-on-failure ) > test_after.log 2>&1`

Result: `c4cll` and `backend_prepare_structured_context_test` built successfully and all
5 selected backend tests passed.
