# Current Packet

Status: Active
Source Idea Path: ideas/open/769_lir_global_initializer_label_address_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map the static initializer representation boundary

## Just Finished

- Lifecycle switch: 768 Steps 1 and 2 remain accepted; its Step 3 static
  initializer probe is parked pending this separately scoped contract.

## Suggested Next

- Inspect `ConstInitEmitter`, global lowering, `LirGlobal`, and verifier to map
  the smallest structured label-address initializer element and identify any
  Raw-BIR/importer boundary. Do not edit direct/local or carrier routes.

## Watchouts

- `blockaddress(...)` rendered text and function-ID-only metadata do not carry
  target-label authority. A Raw-BIR/importer need is a downstream blocker, not
  authorization to expand this plan.

## Proof

- Step 1 is evidence mapping only: use targeted source/AST queries and
  `git diff --check`. Code-changing Steps 2–3 require a fresh build plus the
  selected focused frontend-LIR positive/malformed proof.
