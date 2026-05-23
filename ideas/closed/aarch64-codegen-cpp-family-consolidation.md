# AArch64 Codegen CPP Family Consolidation

## Intent

After ownership boundaries and header families are clearer, selectively merge
small or purely connective `.cpp` files in
`src/backend/mir/aarch64/codegen`.

The goal is not to recreate giant implementation files. The goal is to remove
friction from tiny files whose names no longer represent durable concepts.

## Background

The earlier split made large files tractable. Some resulting files still
represent clear concepts and should remain separate, such as larger instruction
selection or value-materialization modules. Others are connective helpers or
thin publication layers that may read better when folded into their family
implementation.

This idea should run after:

- AArch64 Codegen Prepared Boundary Recovery
- AArch64 Codegen Header Family Consolidation

## Work

Review current `.cpp` families and identify merge candidates using these rules:

- merge tiny files when their only purpose is local glue for one sibling module
- keep large files with distinct lowering responsibility separate
- keep files separate when they are likely to be independently modified by
  different backend initiatives
- prefer family-level consolidation, such as `calls_*` into a smaller number of
  `calls` implementation files, over broad cross-family merging

Possible early candidates include:

- `dispatch_publication_common.cpp` into the main publication implementation
- very small call helper files into the nearest `calls` family implementation
- any file that becomes trivial after Prepared boundary recovery

## Constraints

- Do not hide target-independent logic in AArch64 while merging files.
- Do not merge files only to reduce file count if the result becomes harder to
  review.
- Preserve build behavior and existing diagnostics.
- Keep generated diffs reviewable; prefer multiple small consolidation slices.

## Acceptance

- A small set of `.cpp` files is merged based on documented rationale.
- Remaining files have names that correspond to durable implementation concepts.
- Fresh build or targeted backend tests prove the consolidation.

## Completion Note

Closed after merging `dispatch_publication_common.cpp` into
`dispatch_publication.cpp` and `calls_effects.cpp` into `calls_printing.cpp`.
The retained small AArch64 codegen `.cpp` files were re-scanned and kept
because they still name durable route, diagnostics, compatibility, compile, or
call-family boundaries. Backend validation passed with the canonical
`^backend_` regression scope.
