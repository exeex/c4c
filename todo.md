Status: Active
Source Idea Path: ideas/open/104_hir_safe_legacy_lookup_demotion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The First Safe HIR-Only Demotion Packet

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by selecting the first narrow HIR-only demotion
packet: route the direct `src/frontend/hir/impl/stmt/range_for.cpp` probes of
`struct_methods_` and `struct_method_ret_types_` through the existing
parity-recording helper APIs, `find_struct_method_mangled` and
`find_struct_method_return_type`.

Owned files for the next code packet: `src/frontend/hir/impl/stmt/range_for.cpp`
and `todo.md`.

Review classification authorizing this selection:

> Safe demotion candidates:
> - Parity-recording helper paths after direct rendered-map probes are routed
>   through them and focused parity proof is green.

Additional audit constraint for this family:

> Lowerer struct method rendered maps: `struct_methods_`,
> `struct_method_link_name_ids_`, `struct_method_ret_types_` | `bridge-required`
> | Helper APIs record parity against owner-key mirrors, but return rendered
> lookup results and recurse through `module_->struct_defs` base tags. Direct
> range-for/object/operator probes bypass some helpers. | First route direct
> probes through structured-aware helpers, then flip helpers to structured
> authority after parity proof.

## Suggested Next

Execute Step 3 route-direct-method/member-probes packet for
`src/frontend/hir/impl/stmt/range_for.cpp` only.

## Watchouts

- Do not flip helper authority or delete rendered maps in the next packet.
- Keep lowerer struct method rendered maps bridge-required while routing direct
  probes through the parity-recording helpers.
- Do not downgrade tests or expectations to prove cleanup.

## Proof

Metadata-only proof: `git diff --check`.

Proof command for next code packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_(positive_sema_range_for_|hir_stmt_range_for_helper)'`
