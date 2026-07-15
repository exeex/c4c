# Current Packet

Status: Active
Source Idea Path: ideas/open/795_lir_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish the selected parameter-index authority
你該做code review了

## Just Finished

795 Step 2 published checked current-function native body-pointer parameter
definitions at `init_fn_ctx`, consumes matching `DeclRef` uses as SSA, and
retains SSA identity through the selected PtrToInt/subtract chain. `pr21173.c`
now lowers successfully; backend authority coverage includes positive,
malformed, foreign-owner, and type-incoherent parameter definitions.

## Suggested Next

Supervisor: choose the next coherent in-scope packet or review/commit this
completed Step 2 slice; do not advance this executor packet to Step 3.

## Watchouts

Do not use rendered parameter identity, weaken GEP verification, or absorb the
`20060910-1.c` PHI producer failure owned by 806. Publication remains limited
to directly usable native pointer parameters; ABI-expanded/byval/HFA/vector
surfaces remain outside this slice.

## Proof

Fresh `cmake --build --preset default` passed. `ctest --test-dir build -j
--output-on-failure -R '^backend_'` passed (5/5) with output at
`test_after.log`; regression guard against `test_before.log` passed with
`--allow-non-decreasing-passed`. Focused
`backend_lir_selected_pointer_authority` passed, and
`./build/c4cll --codegen llvm tests/c/external/gcc_torture/src/pr21173.c`
succeeded.
