# Current Packet

Status: Active
Source Idea Path: ideas/open/532_bir_local_array_semantic_gep_header_readiness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Probe Consumer Include Narrowing

## Just Finished

Step 3 of `plan.md` probed direct include narrowing for
`src/backend/bir/bir_local_array_semantic_gep.hpp`. No repository code/header
include replacements were made.

Selected consumers probed:

- `src/backend/bir/lir_to_bir.hpp`: replacing `bir.hpp` with
  `bir_local_array_semantic_gep.hpp` failed syntax-only compilation immediately
  because the focused header is aggregator-ordered and does not provide its own
  `std::string_view`/`Value`/`TypeKind` prerequisites. This consumer also needs
  complete core BIR declarations for lowering-facing module/function surfaces.
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`: replacing
  `../lowering.hpp` with `../../bir_local_array_semantic_gep.hpp` failed at
  the same focused-header prerequisite boundary before reaching the body. AST
  type refs also show this file is tied to `BirFunctionLowerer` and complete
  `bir::Function` route/lowering declarations, so direct narrowing is unsafe.
- `src/backend/prealloc/publication_plans.hpp`: replacing its direct
  `../bir/bir.hpp` include with
  `../bir/bir_local_array_semantic_gep.hpp` syntax-checked, but only because
  earlier includes such as `addressing.hpp` still include `../bir/bir.hpp`.
  `-H` include tracing confirmed the full aggregator still enters through
  `addressing.hpp -> ../bir/bir.hpp -> bir_local_array_semantic_gep.hpp`, so
  this is not an independent or coupling-reducing replacement.
- `src/backend/prealloc/publication_plans.cpp`: AST type refs show broad
  complete-type use of `bir::Function`, `bir::Inst`, and
  `prepare::PreparedBirModule`; it remains on the prealloc publication header
  stack rather than the focused analysis header.
- `tests/backend/mir/backend_publication_plan_record_test.cpp`: replacing
  `publication_plans.hpp` with the focused BIR header failed syntax-only
  compilation at the same missing focused-header prerequisites. The test also
  constructs prepared modules/functions and calls prealloc publication APIs.
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`: replacing
  `lir_to_bir.hpp` with the focused BIR header failed syntax-only compilation;
  AST refs show complete `bir::Function` and lowering APIs are required.
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`: replacing its
  direct `src/backend/bir/bir.hpp` include with the focused header failed
  syntax-only compilation at the focused-header prerequisite boundary; AST refs
  show extensive complete `bir::Function` construction and prealloc use.

## Suggested Next

Proceed to Step 4 validation for the behavior-preserving aggregator-only split,
or ask the plan owner to close if the supervisor accepts that direct include
narrowing is parked outside idea 532.

## Watchouts

- `bir_local_array_semantic_gep.hpp` remains aggregator-only and must keep its
  prerequisite-safe include position inside `bir.hpp`.
- A green replacement probe is not enough here when another include still
  pulls in `bir.hpp`; `publication_plans.hpp` is the concrete example.
- Direct include replacement remains parked until broader BIR core model,
  lowering route, and prealloc publication headers expose narrower prerequisites.

## Proof

Direct-include probe/no build. No repository code/include replacement was made,
so the delegated full proof command was intentionally not run and
`test_after.log` was not rewritten by this packet.

Syntax-only probes were run against temporary copies under
`/tmp/c4c_step3_probe`; c4c-clang-tool type-ref queries were run for
`publication_plans.cpp`, `local_gep.cpp`,
`backend_publication_plan_record_test.cpp`, `backend_lir_to_bir_notes_test.cpp`,
and `backend_prepare_stack_layout_test.cpp`.
