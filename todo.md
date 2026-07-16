# Current Packet

Status: Active
Source Idea Path: ideas/open/839_lir_nominal_function_signature_call_composition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the nominal function-signature store

## Just Finished

- Activated `ideas/open/839_lir_nominal_function_signature_call_composition.md`
  after closed 838 aggregate convergence at `574f2a810`.

## Suggested Next

- Execute Step 1: inspect current declaration/call signature construction and
  introduce the smallest module-owned `LirFunctionSignatureRef`/store without
  deleting compatibility mirrors.

## Watchouts

- Do not absorb 829/830 body-use or argument identity, scalar/vector/union
  migration, or generic value-carrier work.
- Do not derive semantic signatures from rendered text, `signature_text`,
  `args_str`, parsed call strings, duplicate `arg_type_refs`, or testcase
  spelling.
- Preserve accepted aggregate owner checks from 838; wrong-module aggregate
  alternatives must fail closed.

## Proof

- Lifecycle-only activation; no build proof required yet.
