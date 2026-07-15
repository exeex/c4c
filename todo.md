# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.37
Current Step Title: Receive the one 824-authorized DirectScalar return-value parameter authority row

## Just Finished

- Lifecycle switch: closed 824 after its accepted producer/schema/verifier
  handoff (`fac485148`); 734 resumes for exactly its matching receiver row.

## Suggested Next

- Step 7.37: receive only the structured `LirRet.return_value_parameter_authority`
  tuple described in `ideas/open/734_lir_to_new_bir_container_completeness.md`.

## Watchouts

- Do not infer parameter authority from return text, signatures, names,
  rendered operands, diagnostics, or `monostate`; do not receive another
  parameter form or reopen Steps 7.35--7.36.

## Proof

- Handoff acceptance: `fac485148`; fresh
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed 6/6, with matching before/after 6/6 non-regression guard.
