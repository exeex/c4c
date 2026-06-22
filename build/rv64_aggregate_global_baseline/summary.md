# RV64 Aggregate Global Baseline

## Prepared BIR global/access facts

8:  - [legalize] bootstrap BIR legalize kept semantic CFG and phi form intact while promoting i1 values plus function signature/storage bookkeeping, memory-address/load-store bookkeeping, and call ABI metadata
9:  - [stack_layout] stack layout prepared function 'main' with 0 home slot(s) and 0 bytes of frame space
13:  - [regalloc] regalloc seeded function 'main' with 4 allocation constraint(s) and 4 interference edge(s) from active liveness; assigned 4 register(s) and 0 stack slot(s)
15:bir.func @main() -> i32 {
17:  bir.store_global @v, i32 1
18:  bir.store_global @v, offset 4, i32 2
19:  %t3 = bir.load_global i32 @v
21:  %t6 = bir.load_global i32 @v, offset 4
27:prepared.summary @main stable_base=rsp frame_size=0 frame_alignment=1 has_dynamic_stack=no saved_regs=2 calls=0 dynamic_stack_ops=0 variadic_entry=no storage_values=4
35:prepared.func @main
38:prepared.func @main
54:prepared.func @main frame_size=0 frame_alignment=1 has_dynamic_stack=no fixed_slots_use_fp=no
59:--- prepared-store-source-publications ---
60:  store_source function=main block=entry inst=0 source=<none> status=available intent=store_global_publication source_producer=unknown source_load_local=no source_load_global=no source_cast=no source_binary=no source_select=no direct_global_select_chain=no direct_global_root_is_select=no
61:  store_source function=main block=entry inst=1 source=<none> status=available intent=store_global_publication source_producer=unknown source_load_local=no source_load_global=no source_cast=no source_binary=no source_select=no direct_global_select_chain=no direct_global_root_is_select=no
63:  select_chain function=main block=entry value=%t3 root_is_select=no root_inst=2 direct_global_select_chain=yes direct_global_root_is_select=no direct_global_root_inst=2 source_producer=load_global source_producer_block=entry source_producer_inst=2
64:  select_chain function=main block=entry value=%t4 root_is_select=no root_inst=3 direct_global_select_chain=yes direct_global_root_is_select=no direct_global_root_inst=3 source_producer=binary source_producer_block=entry source_producer_inst=3
65:  select_chain function=main block=entry value=%t6 root_is_select=no root_inst=4 direct_global_select_chain=yes direct_global_root_is_select=no direct_global_root_inst=4 source_producer=load_global source_producer_block=entry source_producer_inst=4
66:  select_chain function=main block=entry value=%t7 root_is_select=no root_inst=5 direct_global_select_chain=yes direct_global_root_is_select=no direct_global_root_inst=5 source_producer=binary source_producer_block=entry source_producer_inst=5
69:prepared.func @main
71:prepared.func @main
84:prepared.func @main frame_size=0 frame_alignment=1
85:  access block=entry inst_index=0 base=global_symbol symbol=v offset=0 size=4 align=4 base_plus_offset=yes layout_authority=unknown range_verdict=proven_in_bounds dynamic_array_verdict=unknown
86:  access block=entry inst_index=1 base=global_symbol symbol=v offset=4 size=4 align=4 base_plus_offset=yes layout_authority=unknown range_verdict=proven_in_bounds dynamic_array_verdict=unknown
87:  access block=entry inst_index=2 base=global_symbol result=%t3 symbol=v offset=0 size=4 align=4 base_plus_offset=yes layout_authority=unknown range_verdict=proven_in_bounds dynamic_array_verdict=unknown
88:  access block=entry inst_index=4 base=global_symbol result=%t6 symbol=v offset=4 size=4 align=4 base_plus_offset=yes layout_authority=unknown range_verdict=proven_in_bounds dynamic_array_verdict=unknown

## Emit status

emit_exit=1

## Emit diagnostics

error: riscv prepared module emitter does not support this prepared global storage layout

## Assembly excerpts

no assembly file
