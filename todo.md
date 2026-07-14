# Current Packet

Status: Active
Source Idea Path: ideas/open/769_lir_global_initializer_label_address_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement structured publication and verifier validation

## Just Finished

- Step 1 completed: mapped the one `LirGlobal` structured
  initializer-element / global-lowering publisher / LIR-verifier seam in
  `docs/lir_global_initializer_label_address_authority/step1_representation_map.md`.
  Existing metadata retains only `LinkNameId`; a label-address element needs
  the enclosing `LinkNameId` plus function-scoped `LirBlockId`. The first
  downstream boundary is Raw-BIR/importer global lowering, which has no such
  element and remains out of scope.

## Suggested Next

- Implement only the mapped generic `LirGlobal` initializer-element,
  publication from constant/global lowering, and matching LIR verifier checks;
  add direct frontend-LIR positive and malformed proof. Do not cross the
  Raw-BIR/importer boundary or touch direct/local/carrier routes.

## Watchouts

- `blockaddress(...)` rendered text and function-ID-only metadata remain
  insufficient. `LirBlockId` needs its `LinkNameId` owner to avoid
  cross-function ambiguity. Raw-BIR/importer consumption is a separately
  scoped downstream blocker, not authorization to expand Step 2.

## Proof

- Step 1 evidence: targeted AST/source queries and `git diff --check`; no
  build or CTest. Step 2 requires a fresh build plus selected focused
  frontend-LIR positive/malformed proof, recorded in `test_after.log`.
