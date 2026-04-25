#include "call.hpp"
#include "../../../llvm/calling_convention.hpp"

namespace c4c::codegen::lir {

namespace llvm_cc = c4c::codegen::llvm_backend;
using namespace stmt_emitter_detail;

std::string StmtEmitter::emit_amd64_va_arg_from_registers(
    FnCtx& ctx, const TypeSpec& res_ts, const std::string& res_ty,
    const llvm_cc::Amd64VarargInfo& layout, const Amd64VaListPtrs& access,
    const std::string& gp_offset, const std::string& fp_offset) {
  const int size_bytes = layout.size_bytes;
  const int align = object_align_bytes(mod_, module_, res_ts);

  std::string gp_offset_i64;
  if (layout.gp_chunks > 0) {
    gp_offset_i64 = fresh_tmp(ctx);
    emit_lir_op(
        ctx, lir::LirCastOp{gp_offset_i64, lir::LirCastKind::SExt, "i32", gp_offset, "i64"});
  }
  std::string fp_offset_i64;
  if (layout.sse_slots > 0) {
    fp_offset_i64 = fresh_tmp(ctx);
    emit_lir_op(
        ctx, lir::LirCastOp{fp_offset_i64, lir::LirCastKind::SExt, "i32", fp_offset, "i64"});
  }

  std::string gp_base_ptr;
  if (layout.gp_chunks > 0) {
    gp_base_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{
                         gp_base_ptr, "i8", access.reg_save_area_ptr, false,
                         {"i64 " + gp_offset_i64}});
    const std::string new_gp = fresh_tmp(ctx);
    emit_lir_op(
        ctx, lir::LirBinOp{new_gp, "add", "i32", gp_offset, std::to_string(layout.gp_chunks * 8)});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), new_gp, access.gp_offset_ptr});
  }

  std::string fp_base_ptr;
  if (layout.sse_slots > 0) {
    fp_base_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{
                         fp_base_ptr, "i8", access.reg_save_area_ptr, false,
                         {"i64 " + fp_offset_i64}});
    const std::string new_fp = fresh_tmp(ctx);
    emit_lir_op(ctx,
                lir::LirBinOp{new_fp, "add", "i32", fp_offset,
                              std::to_string(layout.sse_slots * 16)});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), new_fp, access.fp_offset_ptr});
  }

  const std::string tmp_addr = fresh_tmp(ctx);
  ctx.alloca_insts.push_back(lir::LirAllocaOp{tmp_addr, res_ty, "", align});

  int gp_chunk_index = 0;
  int sse_slot_index = 0;
  std::string active_sse_slot_ptr;
  for (size_t chunk = 0; chunk < layout.classes.size(); ++chunk) {
    if (static_cast<int>(chunk) * 8 >= size_bytes) break;
    const int chunk_offset = static_cast<int>(chunk) * 8;
    const int chunk_size = std::min(8, size_bytes - static_cast<int>(chunk) * 8);
    const auto cls = layout.classes[chunk];
    if (cls == llvm_cc::Amd64ArgClass::Memory || cls == llvm_cc::Amd64ArgClass::None) continue;

    std::string src_ptr;
    if (cls == llvm_cc::Amd64ArgClass::Integer) {
      src_ptr = gp_base_ptr;
      if (gp_chunk_index > 0) {
        src_ptr = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{
                             src_ptr, "i8", gp_base_ptr, false,
                             {"i64 " + std::to_string(gp_chunk_index * 8)}});
      }
      ++gp_chunk_index;
    } else if (cls == llvm_cc::Amd64ArgClass::SSE) {
      src_ptr = fp_base_ptr;
      if (sse_slot_index > 0) {
        src_ptr = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{
                             src_ptr, "i8", fp_base_ptr, false,
                             {"i64 " + std::to_string(sse_slot_index * 16)}});
      }
      active_sse_slot_ptr = src_ptr;
      ++sse_slot_index;
    } else if (cls == llvm_cc::Amd64ArgClass::SSEUp) {
      if (active_sse_slot_ptr.empty()) active_sse_slot_ptr = fp_base_ptr;
      std::string upper_ptr = fresh_tmp(ctx);
      emit_lir_op(
          ctx, lir::LirGepOp{upper_ptr, "i8", active_sse_slot_ptr, false, {"i64 8"}});
      src_ptr = upper_ptr;
    } else {
      continue;
    }

    std::string dst_ptr = tmp_addr;
    if (chunk_offset > 0) {
      dst_ptr = fresh_tmp(ctx);
      emit_lir_op(
          ctx, lir::LirGepOp{dst_ptr, "i8", tmp_addr, false, {"i64 " + std::to_string(chunk_offset)}});
    }
    module_->need_memcpy = true;
    emit_lir_op(ctx, lir::LirMemcpyOp{dst_ptr, src_ptr, std::to_string(chunk_size), false});
  }

  const std::string out = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, tmp_addr});
  return out;
}

}  // namespace c4c::codegen::lir
