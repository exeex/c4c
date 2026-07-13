Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 5.2
Current Step Title: Close active-source string-authority guard debt

# Current Packet

## Just Finished

- Steps 2--4 are complete: the reviewed schema checkpoint, owner/generation
  identities, private storage and order, facade/views, scoped builders,
  move-only RawBir publication, and foundation verification landed across
  `a89624c62` through `07545736f`.
- Step 5.1 is complete across `2e7d9ecb6` through `d03f45cda`: the active
  minimal importer publishes verified RawBir, the backend consumer no longer
  compiles prealloc/MIR, the default build passes, and backend tests are
  limited to the retained interface graph.
- The retained LIR-to-BIR interface test is green; unsupported semantic forms
  still reject explicitly rather than falling back to legacy or disappearing.

## Suggested Next

- Execute only Step 5.2: exclude `src/backend/legacy/**` from the active-source
  string-authority scan and add the exact, evidence-backed ABI-spelling
  classification for `ModuleData::functions_by_link_name_` required by the
  accepted schema checkpoint.
- Prove the guard plus its self-test, the retained LIR-to-BIR interface, and
  broader CTest before advancing to Step 6.

## Watchouts

- Do not classify individual `src/backend/legacy/**` hits; that tree is
  reference-only and must be outside active-code scanner scope.
- Do not broadly exempt active BIR.  The only active BIR guard hit is the
  checkpoint-approved exact link-name uniqueness/merge index; `FunctionId`,
  not the string, remains stable entity identity.
- Do not jump to Step 9 or add a BIR-to-MIR shell before Steps 6--8 expand the
  supported semantic surface.  There is no new BIR-to-MIR target yet.
- Historical importer and MIR test sources remain unregistered references and
  must not be re-added wholesale.

## Proof

- Latest completed packet: default configure/build and
  `backend_lir_to_bir_interface` passed; compile metadata contains no
  translation unit under `src/backend/legacy`, `src/backend/prealloc`, or
  `src/backend/mir`.
- Current prerequisite failure: broader CTest is 3029/3030 because
  `string_authority_guard` reports archived legacy declarations plus
  `ModuleData::functions_by_link_name_`.
- Step 5.2 proof is pending; overwrite this section with its exact commands and
  results when the packet completes.
