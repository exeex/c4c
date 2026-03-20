#include "hir_emitter.hpp"
#include "canonical_symbol.hpp"
#include "../lir/lir_printer.hpp"

namespace c4c::codegen::llvm_backend {

namespace {

// ── FnPtrSig accessors ───────────────────────────────────────────────────
// These helpers extract return type and parameter types from FnPtrSig.
// canonical_sig is always populated during HIR lowering.

/// Extract the return TypeSpec from a FnPtrSig.
TypeSpec sig_return_type(const FnPtrSig& sig) {
  return sig.return_type.spec;
}

/// Extract the i-th parameter TypeSpec from a FnPtrSig via canonical_sig.
TypeSpec sig_param_type(const FnPtrSig& sig, size_t i) {
  if (sig.canonical_sig) {
    const auto* fsig = sema::get_function_sig(*sig.canonical_sig);
    if (fsig && i < fsig->params.size()) {
      return sema::typespec_from_canonical(fsig->params[i]);
    }
  }
  return i < sig.params.size() ? sig.params[i].spec : TypeSpec{};
}

/// Check if the i-th parameter is va_list by value.
bool sig_param_is_va_list_value(const FnPtrSig& sig, size_t i) {
  const TypeSpec ts = sig_param_type(sig, i);
  return ts.base == TB_VA_LIST && ts.ptr_level == 0 && ts.array_rank == 0;
}

/// Return the number of declared parameters.
size_t sig_param_count(const FnPtrSig& sig) {
  if (sig.canonical_sig) {
    const auto* fsig = sema::get_function_sig(*sig.canonical_sig);
    if (fsig) return fsig->params.size();
  }
  return sig.params.size();
}

/// Check if sig is variadic.
bool sig_is_variadic(const FnPtrSig& sig) {
  if (sig.canonical_sig) {
    const auto* fsig = sema::get_function_sig(*sig.canonical_sig);
    if (fsig) return fsig->is_variadic;
  }
  return sig.variadic;
}

/// Check if sig has a void parameter list (i.e. f(void)).
bool sig_has_void_param_list(const FnPtrSig& sig) {
  if (sig_param_count(sig) != 1) return false;
  const TypeSpec ts = sig_param_type(sig, 0);
  return ts.base == TB_VOID && ts.ptr_level == 0 && ts.array_rank == 0;
}

int llvm_struct_field_slot(const HirStructDef& sd, int target_llvm_idx) {
  if (sd.is_union) return 0;
  int last_idx = -1;
  int cur_offset = 0;
  int slot = 0;
  for (const auto& f : sd.fields) {
    if (f.llvm_idx == last_idx) continue;
    last_idx = f.llvm_idx;
    if (f.offset_bytes > cur_offset) ++slot;
    if (f.llvm_idx == target_llvm_idx) return slot;
    cur_offset = f.offset_bytes + std::max(0, f.size_bytes);
    ++slot;
  }
  return 0;
}

int llvm_struct_field_slot_by_name(const HirStructDef& sd, const std::string& field_name) {
  for (const auto& f : sd.fields) {
    if (f.name == field_name) return llvm_struct_field_slot(sd, f.llvm_idx);
  }
  return 0;
}

std::string llvm_global_sym(const std::string& raw) {
  return "@" + quote_llvm_ident(raw);
}

const char* llvm_visibility(Visibility v) {
  switch (v) {
    case Visibility::Hidden: return "hidden ";
    case Visibility::Protected: return "protected ";
    default: return "";
  }
}

}  // namespace

HirEmitter::HirEmitter(const Module& m) : mod_(m){}

std::string HirEmitter::emit(){
    emit_preamble();
    // Deduplicate globals: C tentative definitions can produce multiple entries
    // for the same name. Prefer the entry with an explicit initializer; among
    // equals, prefer later entries (last wins for extern semantics).
    std::unordered_map<std::string, size_t> best_gv; // name -> index in mod_.globals
    for (size_t i = 0; i < mod_.globals.size(); ++i) {
      const GlobalVar& gv = mod_.globals[i];
      auto it = best_gv.find(gv.name);
      if (it == best_gv.end()) {
        best_gv[gv.name] = i;
      } else {
        // Prefer explicitly initialized over tentative (monostate)
        const bool cur_has_init = !std::holds_alternative<std::monostate>(mod_.globals[it->second].init);
        const bool new_has_init = !std::holds_alternative<std::monostate>(gv.init);
        if (new_has_init || !cur_has_init) it->second = i;
      }
    }
    // Emit in original order, skipping non-best duplicates
    for (size_t i = 0; i < mod_.globals.size(); ++i) {
      const GlobalVar& gv = mod_.globals[i];
      if (best_gv.count(gv.name) && best_gv[gv.name] == i) emit_global(gv);
    }
    // Deduplicate functions: prefer definitions (non-empty blocks) over declarations.
    // Skip non-materialized functions — they exist for compile-time analysis only.
    std::unordered_map<std::string, size_t> best_fn; // name -> index
    for (size_t i = 0; i < mod_.functions.size(); ++i) {
      const Function& fn = mod_.functions[i];
      if (!fn.materialized) continue;
      auto it = best_fn.find(fn.name);
      if (it == best_fn.end()) {
        best_fn[fn.name] = i;
      } else {
        const bool cur_is_def = !mod_.functions[it->second].blocks.empty();
        const bool new_is_def = !fn.blocks.empty();
        if (new_is_def && !cur_is_def) it->second = i;
      }
    }
    // Emit in original order, skipping non-best duplicates
    for (size_t i = 0; i < mod_.functions.size(); ++i) {
      const Function& fn = mod_.functions[i];
      if (best_fn.count(fn.name) && best_fn[fn.name] == i) emit_function(fn);
    }

    // ── Finalize module_ state before printing ─────────────────────────────
    // Propagate intrinsic requirement flags.
    if (need_llvm_va_start_)     module_.need_va_start = true;
    if (need_llvm_va_end_)       module_.need_va_end = true;
    if (need_llvm_va_copy_)      module_.need_va_copy = true;
    if (need_llvm_memcpy_)       module_.need_memcpy = true;
    if (need_llvm_stacksave_)    module_.need_stacksave = true;
    if (need_llvm_stackrestore_) module_.need_stackrestore = true;
    if (need_llvm_abs_)          module_.need_abs = true;
    // Populate LirModule extern_decls from extern_call_decls_.
    for (const auto& [name, ret_ty] : extern_call_decls_) {
      if (mod_.fn_index.count(name)) continue;
      lir::LirExternDecl ed;
      ed.name = name;
      ed.return_type_str = ret_ty;
      module_.extern_decls.push_back(std::move(ed));
    }
    // Populate specialization metadata.
    for (const auto& e : spec_entries_) {
      module_.spec_entries.push_back({e.spec_key, e.template_origin, e.mangled_name});
    }

    // ── Render via standalone LIR printer ────────────────────────────────────
    return lir::print_llvm(module_);
  }

const GlobalVar* HirEmitter::select_global_object(const std::string& name) const {
    const GlobalVar* best = nullptr;
    for (const auto& g : mod_.globals) {
      if (g.name != name) continue;
      if (!best) {
        best = &g;
        continue;
      }
      const TypeSpec& best_ts = best->type.spec;
      const TypeSpec& cand_ts = g.type.spec;
      if (cand_ts.array_rank > 0 && best_ts.array_rank <= 0) {
        best = &g;
      } else if (cand_ts.array_rank > 0 && best_ts.array_rank > 0 &&
                 cand_ts.array_size > best_ts.array_size) {
        best = &g;
      } else if (cand_ts.array_rank <= 0 && best_ts.array_rank <= 0 &&
                 g.init.index() != 0 && best->init.index() == 0) {
        best = &g;
      }
    }
    return best;
  }

const GlobalVar* HirEmitter::select_global_object(GlobalId id) const {
    const GlobalVar* gv = mod_.find_global(id);
    if (!gv) return nullptr;
    if (const GlobalVar* best = select_global_object(gv->name)) return best;
    return gv;
  }

void HirEmitter::emit_instr(FnCtx& ctx, const std::string& line){
    ctx.cur_block().insts.push_back(lir::LirRawLine{"  " + line});
    ctx.last_term = false;
  }

void HirEmitter::emit_lbl(FnCtx& ctx, const std::string& lbl){
    lir::LirBlock blk;
    blk.id = lir::LirBlockId{static_cast<uint32_t>(ctx.lir_blocks.size())};
    blk.label = lbl;
    ctx.lir_blocks.push_back(std::move(blk));
    ctx.current_block_idx = ctx.lir_blocks.size() - 1;
    ctx.last_term = false;
  }

void HirEmitter::emit_term(FnCtx& ctx, const std::string& line){
    if (!ctx.last_term) {
      ctx.cur_block().insts.push_back(lir::LirRawLine{"  " + line});
      ctx.last_term = true;
    }
  }

std::string HirEmitter::fresh_tmp(FnCtx& ctx){
    return "%t" + std::to_string(ctx.tmp_idx++);
  }

void HirEmitter::record_extern_call_decl(const std::string& name, const std::string& ret_ty){
    if (name.empty() || mod_.fn_index.count(name)) return;
    auto it = extern_call_decls_.find(name);
    if (it == extern_call_decls_.end()) {
      extern_call_decls_.emplace(name, ret_ty);
      return;
    }
    if (it->second == "void" && ret_ty != "void") it->second = ret_ty;
  }

std::string HirEmitter::fresh_lbl(FnCtx& ctx, const std::string& pfx){
    return pfx + std::to_string(ctx.tmp_idx++);
  }

std::string HirEmitter::block_lbl(BlockId id){
    return "block_" + std::to_string(id.value);
  }

std::string HirEmitter::intern_str(const std::string& raw_bytes){
    auto it = str_pool_.find(raw_bytes);
    if (it != str_pool_.end()) return it->second;
    const std::string name = "@.str" + std::to_string(str_idx_++);
    str_pool_[raw_bytes] = name;
    const size_t len = raw_bytes.size() + 1;
    std::string esc;
    for (unsigned char c : raw_bytes) {
      if (c == '"')      { esc += "\\22"; }
      else if (c == '\\') { esc += "\\5C"; }
      else if (c == '\n') { esc += "\\0A"; }
      else if (c == '\r') { esc += "\\0D"; }
      else if (c == '\t') { esc += "\\09"; }
      else if (c < 32 || c >= 127) {
        char buf[8]; std::snprintf(buf, sizeof(buf), "\\%02X", c); esc += buf;
      } else {
        esc += static_cast<char>(c);
      }
    }
    lir::LirStringConst sc;
    sc.pool_name = name;
    sc.raw_bytes = esc;
    sc.byte_length = static_cast<int>(len);
    module_.string_pool.push_back(std::move(sc));
    return name;
  }

void HirEmitter::emit_preamble(){
    module_.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");
    // Emit struct/union type definitions in declaration order
    for (const auto& tag : mod_.struct_def_order) {
      const auto it = mod_.struct_defs.find(tag);
      if (it == mod_.struct_defs.end()) continue;
      const HirStructDef& sd = it->second;
      const std::string sty = llvm_struct_type_str(tag);
      if (sd.fields.empty()) {
        module_.type_decls.push_back(sty + " = type {}");
        continue;
      }
      if (sd.is_union) {
        const int sz = sd.size_bytes;
        module_.type_decls.push_back(sty + " = type { [" +
                                     std::to_string(sz) + " x i8] }");
      } else {
        std::ostringstream line;
        line << sty << " = type { ";
        bool first = true;
        int last_idx = -1;
        int cur_offset = 0;
        for (const auto& f : sd.fields) {
          if (f.llvm_idx == last_idx) continue;
          last_idx = f.llvm_idx;
          if (f.offset_bytes > cur_offset) {
            if (!first) line << ", ";
            first = false;
            line << "[" << (f.offset_bytes - cur_offset) << " x i8]";
            cur_offset = f.offset_bytes;
          }
          if (!first) line << ", ";
          first = false;
          line << llvm_field_ty(f);
          cur_offset = f.offset_bytes + std::max(0, f.size_bytes);
        }
        if (sd.size_bytes > cur_offset) {
          if (!first) line << ", ";
          line << "[" << (sd.size_bytes - cur_offset) << " x i8]";
        }
        line << " }";
        module_.type_decls.push_back(line.str());
      }
    }
  }

bool HirEmitter::is_char_like(TypeBase b){
    return b == TB_CHAR || b == TB_SCHAR || b == TB_UCHAR;
  }

TypeSpec HirEmitter::drop_one_array_dim(TypeSpec ts){
    if (ts.array_rank <= 0) return ts;
    for (int i = 0; i < ts.array_rank - 1; ++i) ts.array_dims[i] = ts.array_dims[i + 1];
    ts.array_dims[ts.array_rank - 1] = -1;
    ts.array_rank--;
    ts.array_size = (ts.array_rank > 0) ? ts.array_dims[0] : -1;
    return ts;
  }

std::string HirEmitter::bytes_from_string_literal(const StringLiteral& sl){
    std::string bytes = sl.raw;
    if (bytes.size() >= 2 && bytes.front() == '"' && bytes.back() == '"') {
      bytes = bytes.substr(1, bytes.size() - 2);
    } else if (bytes.size() >= 3 && bytes[0] == 'L' && bytes[1] == '"' &&
               bytes.back() == '"') {
      bytes = bytes.substr(2, bytes.size() - 3);
    }
    return decode_c_escaped_bytes(bytes);
  }

std::vector<long long> HirEmitter::decode_wide_string_values(const std::string& raw){
    std::vector<long long> out;
    // Strip L"..." wrapper
    const char* p = raw.c_str();
    while (*p && *p != '"') ++p;
    if (*p == '"') ++p;
    while (*p && *p != '"') {
      if (*p == '\\' && *(p + 1)) {
        ++p;
        switch (*p) {
          case 'n':  out.push_back('\n'); ++p; break;
          case 't':  out.push_back('\t'); ++p; break;
          case 'r':  out.push_back('\r'); ++p; break;
          case '0':  out.push_back(0); ++p; break;
          case '\\': out.push_back('\\'); ++p; break;
          case '\'': out.push_back('\''); ++p; break;
          case '"':  out.push_back('"'); ++p; break;
          case 'a':  out.push_back('\a'); ++p; break;
          case 'x': {
            ++p;
            long long v = 0;
            while (isxdigit((unsigned char)*p)) {
              v = v * 16 + (isdigit((unsigned char)*p) ? *p - '0' : tolower((unsigned char)*p) - 'a' + 10);
              ++p;
            }
            out.push_back(v);
            break;
          }
          default:
            if (*p >= '0' && *p <= '7') {
              long long v = 0;
              for (int k = 0; k < 3 && *p >= '0' && *p <= '7'; ++k, ++p)
                v = v * 8 + (*p - '0');
              out.push_back(v);
            } else {
              out.push_back(static_cast<unsigned char>(*p)); ++p;
            }
            break;
        }
        continue;
      }
      // Decode UTF-8 to Unicode codepoint
      const unsigned char c0 = static_cast<unsigned char>(*p);
      if (c0 < 0x80) {
        out.push_back(c0); ++p;
      } else if ((c0 & 0xE0) == 0xC0 && p[1]) {
        out.push_back(((c0 & 0x1F) << 6) | (static_cast<unsigned char>(p[1]) & 0x3F));
        p += 2;
      } else if ((c0 & 0xF0) == 0xE0 && p[1] && p[2]) {
        out.push_back(((c0 & 0x0F) << 12) | ((static_cast<unsigned char>(p[1]) & 0x3F) << 6) |
                       (static_cast<unsigned char>(p[2]) & 0x3F));
        p += 3;
      } else if ((c0 & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) {
        out.push_back(((c0 & 0x07) << 18) | ((static_cast<unsigned char>(p[1]) & 0x3F) << 12) |
                       ((static_cast<unsigned char>(p[2]) & 0x3F) << 6) |
                       (static_cast<unsigned char>(p[3]) & 0x3F));
        p += 4;
      } else {
        out.push_back(c0); ++p;
      }
    }
    out.push_back(0); // null terminator
    return out;
  }

std::string HirEmitter::escape_llvm_c_bytes(const std::string& raw_bytes){
    std::string esc;
    for (unsigned char c : raw_bytes) {
      if (c == '"')      { esc += "\\22"; }
      else if (c == '\\') { esc += "\\5C"; }
      else if (c == '\n') { esc += "\\0A"; }
      else if (c == '\r') { esc += "\\0D"; }
      else if (c == '\t') { esc += "\\09"; }
      else if (c < 32 || c >= 127) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "\\%02X", c);
        esc += buf;
      } else {
        esc += static_cast<char>(c);
      }
    }
    return esc;
  }

TypeSpec HirEmitter::field_decl_type(const HirStructField& f) const{
    TypeSpec ts = f.elem_type;
    if (f.array_first_dim >= 0) {
      for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
      ts.array_rank = 1;
      ts.array_size = f.array_first_dim;
      ts.array_dims[0] = f.array_first_dim;
    }
    return ts;
  }

std::optional<long long> HirEmitter::try_const_eval_int(ExprId id){
    const Expr& e = get_expr(id);
    return std::visit([&](const auto& p) -> std::optional<long long> {
      using T = std::decay_t<decltype(p)>;
      if constexpr (std::is_same_v<T, IntLiteral>) {
        return p.value;
      } else if constexpr (std::is_same_v<T, CharLiteral>) {
        return static_cast<long long>(p.value);
      } else if constexpr (std::is_same_v<T, CastExpr>) {
        return try_const_eval_int(p.expr);
      } else if constexpr (std::is_same_v<T, UnaryExpr>) {
        auto v = try_const_eval_int(p.operand);
        if (!v) return std::nullopt;
        switch (p.op) {
          case UnaryOp::Plus:   return *v;
          case UnaryOp::Minus:  return -*v;
          case UnaryOp::BitNot: return ~*v;
          case UnaryOp::Not:    return static_cast<long long>(!*v);
          default: return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, BinaryExpr>) {
        auto lv = try_const_eval_int(p.lhs);
        auto rv = try_const_eval_int(p.rhs);
        if (!lv || !rv) return std::nullopt;
        switch (p.op) {
          case BinaryOp::Add:    return *lv + *rv;
          case BinaryOp::Sub:    return *lv - *rv;
          case BinaryOp::Mul:    return *lv * *rv;
          case BinaryOp::Div:    return (*rv != 0) ? *lv / *rv : 0LL;
          case BinaryOp::Mod:    return (*rv != 0) ? *lv % *rv : 0LL;
          case BinaryOp::Shl:    return *lv << *rv;
          case BinaryOp::Shr:    return *lv >> *rv;
          case BinaryOp::BitAnd: return *lv & *rv;
          case BinaryOp::BitOr:  return *lv | *rv;
          case BinaryOp::BitXor: return *lv ^ *rv;
          case BinaryOp::Lt:     return static_cast<long long>(*lv < *rv);
          case BinaryOp::Le:     return static_cast<long long>(*lv <= *rv);
          case BinaryOp::Gt:     return static_cast<long long>(*lv > *rv);
          case BinaryOp::Ge:     return static_cast<long long>(*lv >= *rv);
          case BinaryOp::Eq:     return static_cast<long long>(*lv == *rv);
          case BinaryOp::Ne:     return static_cast<long long>(*lv != *rv);
          case BinaryOp::LAnd:   return static_cast<long long>(*lv && *rv);
          case BinaryOp::LOr:    return static_cast<long long>(*lv || *rv);
          default: return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, SizeofTypeExpr>) {
        return static_cast<long long>(sizeof_ts(mod_, p.type.spec));
      } else if constexpr (std::is_same_v<T, SizeofExpr>) {
        const Expr& op = get_expr(p.expr);
        if (has_concrete_type(op.type.spec))
          return static_cast<long long>(sizeof_ts(mod_, op.type.spec));
        return std::nullopt;
      } else {
        return std::nullopt;
      }
    }, e.payload);
  }

std::optional<double> HirEmitter::try_const_eval_float(ExprId id){
    const Expr& e = get_expr(id);
    return std::visit([&](const auto& p) -> std::optional<double> {
      using T = std::decay_t<decltype(p)>;
      if constexpr (std::is_same_v<T, FloatLiteral>) {
        return p.value;
      } else if constexpr (std::is_same_v<T, IntLiteral>) {
        return static_cast<double>(p.value);
      } else if constexpr (std::is_same_v<T, CharLiteral>) {
        return static_cast<double>(p.value);
      } else if constexpr (std::is_same_v<T, CastExpr>) {
        return try_const_eval_float(p.expr);
      } else if constexpr (std::is_same_v<T, UnaryExpr>) {
        auto v = try_const_eval_float(p.operand);
        if (!v) return std::nullopt;
        switch (p.op) {
          case UnaryOp::Plus:  return *v;
          case UnaryOp::Minus: return -*v;
          default: return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, BinaryExpr>) {
        auto lv = try_const_eval_float(p.lhs);
        auto rv = try_const_eval_float(p.rhs);
        if (!lv || !rv) return std::nullopt;
        switch (p.op) {
          case BinaryOp::Add: return *lv + *rv;
          case BinaryOp::Sub: return *lv - *rv;
          case BinaryOp::Mul: return *lv * *rv;
          case BinaryOp::Div: return *lv / *rv;  // IEEE: div-by-zero → ±Inf
          default: return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, CallExpr>) {
        const BuiltinId builtin_id = p.builtin_id;
        switch (builtin_constant_fp_kind(builtin_id)) {
          case BuiltinConstantFpKind::Infinity:
            if (builtin_id == BuiltinId::HugeValF || builtin_id == BuiltinId::InfF) {
              return static_cast<double>(std::numeric_limits<float>::infinity());
            }
            return std::numeric_limits<double>::infinity();
          case BuiltinConstantFpKind::QuietNaN:
            if (builtin_id == BuiltinId::NanF || builtin_id == BuiltinId::NansF) {
              return static_cast<double>(std::numeric_limits<float>::quiet_NaN());
            }
            return std::numeric_limits<double>::quiet_NaN();
          case BuiltinConstantFpKind::None:
            break;
        }
        return std::nullopt;
      } else {
        return std::nullopt;
      }
    }, e.payload);
  }

std::optional<std::pair<long long, long long>> HirEmitter::try_const_eval_complex_int(ExprId id){
    const Expr& e = get_expr(id);
    return std::visit([&](const auto& p) -> std::optional<std::pair<long long, long long>> {
      using T = std::decay_t<decltype(p)>;
      if constexpr (std::is_same_v<T, IntLiteral>) {
        if (e.type.spec.base == TB_COMPLEX_CHAR || e.type.spec.base == TB_COMPLEX_SCHAR ||
            e.type.spec.base == TB_COMPLEX_UCHAR || e.type.spec.base == TB_COMPLEX_SHORT ||
            e.type.spec.base == TB_COMPLEX_USHORT || e.type.spec.base == TB_COMPLEX_INT ||
            e.type.spec.base == TB_COMPLEX_UINT || e.type.spec.base == TB_COMPLEX_LONG ||
            e.type.spec.base == TB_COMPLEX_ULONG || e.type.spec.base == TB_COMPLEX_LONGLONG ||
            e.type.spec.base == TB_COMPLEX_ULONGLONG) {
          return std::pair<long long, long long>{0, p.value};
        }
        return std::pair<long long, long long>{p.value, 0};
      } else if constexpr (std::is_same_v<T, CharLiteral>) {
        return std::pair<long long, long long>{p.value, 0};
      } else if constexpr (std::is_same_v<T, UnaryExpr>) {
        auto v = try_const_eval_complex_int(p.operand);
        if (!v) return std::nullopt;
        switch (p.op) {
          case UnaryOp::Plus: return v;
          case UnaryOp::Minus: return std::pair<long long, long long>{-v->first, -v->second};
          default: return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, BinaryExpr>) {
        auto lv = try_const_eval_complex_int(p.lhs);
        auto rv = try_const_eval_complex_int(p.rhs);
        if (!lv || !rv) return std::nullopt;
        switch (p.op) {
          case BinaryOp::Add:
            return std::pair<long long, long long>{lv->first + rv->first, lv->second + rv->second};
          case BinaryOp::Sub:
            return std::pair<long long, long long>{lv->first - rv->first, lv->second - rv->second};
          default:
            return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, CastExpr>) {
        return try_const_eval_complex_int(p.expr);
      } else {
        return std::nullopt;
      }
    }, e.payload);
  }

std::optional<std::pair<double, double>> HirEmitter::try_const_eval_complex_fp(ExprId id){
    const Expr& e = get_expr(id);
    return std::visit([&](const auto& p) -> std::optional<std::pair<double, double>> {
      using T = std::decay_t<decltype(p)>;
      if constexpr (std::is_same_v<T, FloatLiteral>) {
        if (is_complex_base(e.type.spec.base)) return std::pair<double, double>{0.0, p.value};
        return std::pair<double, double>{p.value, 0.0};
      } else if constexpr (std::is_same_v<T, IntLiteral>) {
        if (is_complex_base(e.type.spec.base)) return std::pair<double, double>{0.0, static_cast<double>(p.value)};
        return std::pair<double, double>{static_cast<double>(p.value), 0.0};
      } else if constexpr (std::is_same_v<T, CharLiteral>) {
        return std::pair<double, double>{static_cast<double>(p.value), 0.0};
      } else if constexpr (std::is_same_v<T, UnaryExpr>) {
        auto v = try_const_eval_complex_fp(p.operand);
        if (!v) return std::nullopt;
        switch (p.op) {
          case UnaryOp::Plus: return v;
          case UnaryOp::Minus: return std::pair<double, double>{-v->first, -v->second};
          default: return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, BinaryExpr>) {
        auto lv = try_const_eval_complex_fp(p.lhs);
        auto rv = try_const_eval_complex_fp(p.rhs);
        if (!lv || !rv) return std::nullopt;
        switch (p.op) {
          case BinaryOp::Add:
            return std::pair<double, double>{lv->first + rv->first, lv->second + rv->second};
          case BinaryOp::Sub:
            return std::pair<double, double>{lv->first - rv->first, lv->second - rv->second};
          case BinaryOp::Mul:
            return std::pair<double, double>{
                lv->first * rv->first - lv->second * rv->second,
                lv->first * rv->second + lv->second * rv->first};
          case BinaryOp::Div: {
            const double denom = rv->first * rv->first + rv->second * rv->second;
            if (denom == 0.0) return std::nullopt;
            return std::pair<double, double>{
                (lv->first * rv->first + lv->second * rv->second) / denom,
                (lv->second * rv->first - lv->first * rv->second) / denom};
          }
          default:
            return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, CastExpr>) {
        return try_const_eval_complex_fp(p.expr);
      } else {
        return std::nullopt;
      }
    }, e.payload);
  }

std::string HirEmitter::emit_const_int_like(long long value, const TypeSpec& expected_ts){
    if (llvm_ty(expected_ts) == "ptr") {
      if (value == 0) return "null";
      return "inttoptr (i64 " + std::to_string(value) + " to ptr)";
    }
    if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
      return fp_literal(expected_ts.base, static_cast<double>(value));
    return std::to_string(value);
  }

std::string HirEmitter::emit_const_scalar_expr(ExprId id, const TypeSpec& expected_ts){
    const Expr& e = get_expr(id);
    return std::visit([&](const auto& p) -> std::string {
      using T = std::decay_t<decltype(p)>;
      if (is_complex_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0) {
        const TypeSpec elem_ts = complex_component_ts(expected_ts.base);
        if (is_float_base(elem_ts.base)) {
          if (auto cv = try_const_eval_complex_fp(id)) {
            const auto emit_fp = [&](double value) -> std::string {
              return fp_literal(elem_ts.base, value);
            };
            return "{ " + llvm_ty(elem_ts) + " " + emit_fp(cv->first) +
                   ", " + llvm_ty(elem_ts) + " " + emit_fp(cv->second) + " }";
          }
        }
        if (auto cv = try_const_eval_complex_int(id)) {
          return "{ " + llvm_ty(elem_ts) + " " +
                 emit_const_int_like(cv->first, elem_ts) +
                 ", " + llvm_ty(elem_ts) + " " +
                 emit_const_int_like(cv->second, elem_ts) +
                 " }";
        }
        return "zeroinitializer";
      }
      if constexpr (std::is_same_v<T, IntLiteral>) {
        if (llvm_ty(expected_ts) == "ptr") {
          if (p.value == 0) return "null";
          return "inttoptr (i64 " + std::to_string(p.value) + " to ptr)";
        }
        if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
          return fp_literal(expected_ts.base, static_cast<double>(p.value));
        return std::to_string(p.value);
      } else if constexpr (std::is_same_v<T, FloatLiteral>) {
        if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
          return fp_literal(expected_ts.base, p.value);
        return fp_to_hex(p.value);
      } else if constexpr (std::is_same_v<T, CharLiteral>) {
        return std::to_string(p.value);
      } else if constexpr (std::is_same_v<T, StringLiteral>) {
        const std::string bytes = bytes_from_string_literal(p);
        const std::string gname = intern_str(bytes);
        const size_t len = bytes.size() + 1;
        return "getelementptr inbounds ([" + std::to_string(len) + " x i8], ptr " +
               gname + ", i64 0, i64 0)";
      } else if constexpr (std::is_same_v<T, DeclRef>) {
        if (mod_.fn_index.count(p.name) || llvm_ty(expected_ts) == "ptr" ||
            expected_ts.ptr_level > 0 || expected_ts.is_fn_ptr) {
          return llvm_global_sym(p.name);
        }
        return "0";
      } else if constexpr (std::is_same_v<T, UnaryExpr>) {
        if (p.op == UnaryOp::AddrOf) {
          const Expr& op_e = get_expr(p.operand);
          if (const auto* r = std::get_if<DeclRef>(&op_e.payload))
            return llvm_global_sym(r->name);
          if (const auto* s = std::get_if<StringLiteral>(&op_e.payload)) {
            const std::string bytes = bytes_from_string_literal(*s);
            const std::string gname = intern_str(bytes);
            const size_t len = bytes.size() + 1;
            return "getelementptr inbounds ([" + std::to_string(len) + " x i8], ptr " +
                   gname + ", i64 0, i64 0)";
          }
          // &arr[i] or &arr[i][j][k] — global array subscript: emit GEP constant
          // Recursively collect indices from nested IndexExpr chains.
          if (const auto* idx_e = std::get_if<IndexExpr>(&op_e.payload)) {
            std::vector<long long> indices;
            const Expr* cur_e = &op_e;
            while (const auto* ie = std::get_if<IndexExpr>(&cur_e->payload)) {
              indices.push_back(try_const_eval_int(ie->index).value_or(0));
              cur_e = &get_expr(ie->base);
            }
            std::reverse(indices.begin(), indices.end());
            if (const auto* dr = std::get_if<DeclRef>(&cur_e->payload)) {
              auto git = mod_.global_index.find(dr->name);
              if (git != mod_.global_index.end()) {
                const GlobalVar* gv = select_global_object(git->second);
                if (gv && gv->type.spec.array_rank > 0) {
                  const TypeSpec& resolved = gv->type.spec;
                  const std::string aty = llvm_alloca_ty(resolved);
                  std::string gep = "getelementptr inbounds (" + aty + ", ptr @" + dr->name + ", i64 0";
                  for (auto idx : indices) gep += ", i64 " + std::to_string(idx);
                  gep += ")";
                  return gep;
                }
              }
            }
            // &"string"[i] — address of character in string literal
            if (const auto* sl = std::get_if<StringLiteral>(&cur_e->payload)) {
              const std::string bytes = bytes_from_string_literal(*sl);
              const std::string gname = intern_str(bytes);
              const size_t len = bytes.size() + 1;
              const std::string aty = "[" + std::to_string(len) + " x i8]";
              std::string gep = "getelementptr inbounds (" + aty + ", ptr " + gname + ", i64 0";
              for (auto idx : indices) gep += ", i64 " + std::to_string(idx);
              gep += ")";
              return gep;
            }
          }
          // &struct_var.field or &(arr+N)->field — global struct member: emit GEP constant
          if (const auto* mem_e = std::get_if<MemberExpr>(&op_e.payload)) {
            auto find_struct_def = [&](const std::string& tag) -> const HirStructDef* {
              auto sit = mod_.struct_defs.find(tag);
              return (sit != mod_.struct_defs.end()) ? &sit->second : nullptr;
            };
            if (!mem_e->is_arrow) {
              // Walk nested MemberExpr chain to collect field path and find root DeclRef.
              // Handles &v.d.b (arbitrary nesting depth).
              {
                std::vector<std::string> field_path;
                const Expr* walk = &op_e;
                while (const auto* me = std::get_if<MemberExpr>(&walk->payload)) {
                  if (me->is_arrow) break;
                  field_path.push_back(me->field);
                  walk = &get_expr(me->base);
                }
                std::reverse(field_path.begin(), field_path.end());
                if (field_path.size() >= 2) {
                  if (const auto* dr = std::get_if<DeclRef>(&walk->payload)) {
                    auto git = mod_.global_index.find(dr->name);
                    if (git != mod_.global_index.end()) {
                      const GlobalVar* gv = mod_.find_global(git->second);
                      if (gv && gv->type.spec.tag && gv->type.spec.tag[0]) {
                        // Compute cumulative byte offset through all member accesses.
                        long long total_offset = 0;
                        std::string cur_tag(gv->type.spec.tag);
                        bool ok = true;
                        for (const auto& fname : field_path) {
                          const auto* sd = find_struct_def(cur_tag);
                          if (!sd) { ok = false; break; }
                          bool found = false;
                          for (const auto& f : sd->fields) {
                            if (f.name == fname) {
                              total_offset += f.offset_bytes;
                              // Determine the tag for next level (if this field is a struct/union).
                              TypeSpec ft = f.elem_type;
                              ft.inner_rank = -1;
                              if (ft.tag && ft.tag[0] && ft.ptr_level == 0 &&
                                  (ft.base == TB_STRUCT || ft.base == TB_UNION)) {
                                cur_tag = ft.tag;
                              }
                              found = true;
                              break;
                            }
                          }
                          if (!found) { ok = false; break; }
                        }
                        if (ok) {
                          if (total_offset == 0) return "@" + dr->name;
                          return "getelementptr (i8, ptr @" + dr->name +
                                 ", i64 " + std::to_string(total_offset) + ")";
                        }
                      }
                    }
                  }
                }
              }
              const Expr& base_e = get_expr(mem_e->base);
              // &struct_var.field
              if (const auto* dr = std::get_if<DeclRef>(&base_e.payload)) {
                auto git = mod_.global_index.find(dr->name);
                if (git != mod_.global_index.end()) {
                  const GlobalVar* gv = mod_.find_global(git->second);
                  if (gv && gv->type.spec.tag && gv->type.spec.tag[0]) {
                    const std::string tag(gv->type.spec.tag);
                    if (const auto* sd = find_struct_def(tag)) {
                      const int fi = llvm_struct_field_slot_by_name(*sd, mem_e->field);
                      return "getelementptr inbounds (" + llvm_struct_type_str(tag) +
                             ", ptr " + llvm_global_sym(dr->name) +
                             ", i32 0, i32 " + std::to_string(fi) + ")";
                    }
                  }
                }
              }
              // &arr[i].field — array of structs with field access
              if (const auto* idx_e = std::get_if<IndexExpr>(&base_e.payload)) {
                std::vector<long long> indices;
                const Expr* cur_e = &base_e;
                while (const auto* ie = std::get_if<IndexExpr>(&cur_e->payload)) {
                  indices.push_back(try_const_eval_int(ie->index).value_or(0));
                  cur_e = &get_expr(ie->base);
                }
                std::reverse(indices.begin(), indices.end());
                if (const auto* dr = std::get_if<DeclRef>(&cur_e->payload)) {
                  auto git = mod_.global_index.find(dr->name);
                  if (git != mod_.global_index.end()) {
                    const GlobalVar* gv = select_global_object(git->second);
                    if (gv && gv->type.spec.array_rank > 0) {
                      // Find the struct tag from the element type
                      TypeSpec elem_ts = gv->type.spec;
                      for (int i = 0; i < (int)indices.size() && elem_ts.array_rank > 0; ++i) {
                        elem_ts.array_rank--;
                        if (elem_ts.array_rank > 0) {
                          for (int j = 0; j < 7; ++j) elem_ts.array_dims[j] = elem_ts.array_dims[j+1];
                          elem_ts.array_dims[7] = -1;
                          elem_ts.array_size = elem_ts.array_dims[0];
                        }
                      }
                      if (elem_ts.tag && elem_ts.tag[0]) {
                        const std::string tag(elem_ts.tag);
                        if (const auto* sd = find_struct_def(tag)) {
                          const int fi = llvm_struct_field_slot_by_name(*sd, mem_e->field);
                          const std::string aty = llvm_alloca_ty(gv->type.spec);
                          std::string gep = "getelementptr inbounds (" + aty + ", ptr @" + dr->name + ", i64 0";
                          for (auto idx : indices) gep += ", i64 " + std::to_string(idx);
                          gep += ", i32 " + std::to_string(fi) + ")";
                          return gep;
                        }
                      }
                    }
                  }
                }
              }
            } else {
              // &(ptr+N)->field  or &(arr[i])->field
              // Arrow member: base is a pointer expression. Common pattern:
              // (Upgrade_items + 1)->uaattrid  → GEP on Upgrade_items array
              const Expr& base_e = get_expr(mem_e->base);
              // arr->field  where arr is global array of structs (no offset, index 0)
              if (const auto* dr = std::get_if<DeclRef>(&base_e.payload)) {
                auto git = mod_.global_index.find(dr->name);
                if (git != mod_.global_index.end()) {
                  const GlobalVar* gv = select_global_object(git->second);
                  if (gv && gv->type.spec.array_rank > 0 && gv->type.spec.tag && gv->type.spec.tag[0]) {
                    const std::string tag(gv->type.spec.tag);
                    if (const auto* sd = find_struct_def(tag)) {
                      const int fi = llvm_struct_field_slot_by_name(*sd, mem_e->field);
                      const std::string aty = llvm_alloca_ty(gv->type.spec);
                      return "getelementptr inbounds (" + aty + ", ptr @" + dr->name +
                             ", i64 0, i64 0, i32 " + std::to_string(fi) + ")";
                    }
                  }
                }
              }
              // (arr + N)->field  where arr is global array of structs
              if (const auto* bin_e = std::get_if<BinaryExpr>(&base_e.payload)) {
                if (bin_e->op == BinaryOp::Add) {
                  const Expr& lhs_e = get_expr(bin_e->lhs);
                  const Expr& rhs_e = get_expr(bin_e->rhs);
                  const DeclRef* dr2 = std::get_if<DeclRef>(&lhs_e.payload);
                  auto rhs_val = try_const_eval_int(bin_e->rhs);
                  if (!dr2) {
                    dr2 = std::get_if<DeclRef>(&rhs_e.payload);
                    rhs_val = try_const_eval_int(bin_e->lhs);
                  }
                  if (dr2 && rhs_val) {
                    auto git = mod_.global_index.find(dr2->name);
                    if (git != mod_.global_index.end()) {
                      const GlobalVar* gv = select_global_object(git->second);
                      if (gv && gv->type.spec.array_rank > 0 && gv->type.spec.tag && gv->type.spec.tag[0]) {
                        const std::string tag(gv->type.spec.tag);
                        if (const auto* sd = find_struct_def(tag)) {
                          const int fi = llvm_struct_field_slot_by_name(*sd, mem_e->field);
                          const std::string aty = llvm_alloca_ty(gv->type.spec);
                          return "getelementptr inbounds (" + aty + ", ptr @" + dr2->name +
                                 ", i64 0, i64 " + std::to_string(*rhs_val) +
                                 ", i32 " + std::to_string(fi) + ")";
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
        if (p.op == UnaryOp::Plus) {
          return emit_const_scalar_expr(p.operand, expected_ts);
        }
        if (p.op == UnaryOp::Minus) {
          const Expr& op_e = get_expr(p.operand);
          if (const auto* i = std::get_if<IntLiteral>(&op_e.payload)) {
            // When target is float, emit as float constant, not integer literal.
            if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
              return fp_literal(expected_ts.base, static_cast<double>(-i->value));
            return std::to_string(-i->value);
          }
          if (const auto* f = std::get_if<FloatLiteral>(&op_e.payload)) {
            if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
              return fp_literal(expected_ts.base, -f->value);
            return fp_to_hex(-f->value);
          }
        }
        // For float targets, try full float constant eval (handles -0.0, etc.)
        if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0) {
          auto fval = try_const_eval_float(id);
          if (fval) return fp_literal(expected_ts.base, *fval);
        }
        return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
      } else if constexpr (std::is_same_v<T, CastExpr>) {
        // Apply the cast's target type to properly truncate/extend the value.
        const TypeSpec& cast_ts = p.to_type.spec;
        if (is_any_int(cast_ts.base) && cast_ts.ptr_level == 0 &&
            is_any_int(expected_ts.base) && expected_ts.ptr_level == 0) {
          auto val = try_const_eval_int(p.expr);
          if (val) {
            // Truncate to the cast target width, then zero/sign-extend.
            long long v = *val;
            const int bits = int_bits(cast_ts.base);
            if (bits < 64) {
              unsigned long long mask = (1ULL << bits) - 1;
              unsigned long long uv = static_cast<unsigned long long>(v) & mask;
              if (is_signed_int(cast_ts.base) && (uv >> (bits - 1))) {
                // Sign-extend
                v = static_cast<long long>(uv | ~mask);
              } else {
                v = static_cast<long long>(uv);
              }
            }
            if (is_float_base(expected_ts.base))
              return fp_literal(expected_ts.base, static_cast<double>(v));
            return std::to_string(v);
          }
        }
        return emit_const_scalar_expr(p.expr, expected_ts);
      } else if constexpr (std::is_same_v<T, BinaryExpr>) {
        // Try float eval first when target is a float type
        if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0) {
          auto fval = try_const_eval_float(id);
          if (fval) return fp_literal(expected_ts.base, *fval);
        }
        auto val = try_const_eval_int(id);
        if (val) {
          if (llvm_ty(expected_ts) == "ptr") {
            if (*val == 0) return "null";
            return "inttoptr (i64 " + std::to_string(*val) + " to ptr)";
          }
          return std::to_string(*val);
        }
        // Pointer arithmetic: ptr + int or int + ptr → emit GEP constant
        if (llvm_ty(expected_ts) == "ptr" && p.op == BinaryOp::Add) {
          const Expr& lhs_e = get_expr(p.lhs);
          const Expr& rhs_e = get_expr(p.rhs);
          // string + N
          if (const auto* sl = std::get_if<StringLiteral>(&lhs_e.payload)) {
            auto offset = try_const_eval_int(p.rhs);
            if (offset) {
              const std::string bytes = bytes_from_string_literal(*sl);
              const std::string gname = intern_str(bytes);
              const size_t len = bytes.size() + 1;
              return "getelementptr inbounds ([" + std::to_string(len) + " x i8], ptr " +
                     gname + ", i64 0, i64 " + std::to_string(*offset) + ")";
            }
          }
          // global_array + N  or  N + global_array
          const DeclRef* dr = std::get_if<DeclRef>(&lhs_e.payload);
          auto rhs_val = try_const_eval_int(p.rhs);
          if (!dr) {
            dr = std::get_if<DeclRef>(&rhs_e.payload);
            rhs_val = try_const_eval_int(p.lhs);
          }
          if (dr && rhs_val) {
            auto git = mod_.global_index.find(dr->name);
            if (git != mod_.global_index.end()) {
              const GlobalVar* gv = select_global_object(git->second);
              if (gv) {
                if (gv->type.spec.array_rank > 0) {
                  const std::string aty = llvm_alloca_ty(gv->type.spec);
                  return "getelementptr inbounds (" + aty + ", ptr @" + dr->name +
                         ", i64 0, i64 " + std::to_string(*rhs_val) + ")";
                }
                // Non-array global: byte-level GEP
                return "getelementptr inbounds (i8, ptr @" + dr->name +
                       ", i64 " + std::to_string(*rhs_val) + ")";
              }
            }
          }
        }
        // Label address difference: &&lab1 - &&lab0 → constant expr with blockaddress
        if (p.op == BinaryOp::Sub || p.op == BinaryOp::Add) {
          auto emit_label_ce = [&](auto& self, ExprId eid) -> std::optional<std::string> {
            const Expr& ex = get_expr(eid);
            return std::visit([&](const auto& q) -> std::optional<std::string> {
              using U = std::decay_t<decltype(q)>;
              if constexpr (std::is_same_v<U, LabelAddrExpr>) {
                return "ptrtoint (ptr blockaddress(@" + q.fn_name +
                       ", %ulbl_" + q.label_name + ") to i64)";
              } else if constexpr (std::is_same_v<U, CastExpr>) {
                return self(self, q.expr);
              } else if constexpr (std::is_same_v<U, BinaryExpr>) {
                auto lstr = self(self, q.lhs);
                auto rstr = self(self, q.rhs);
                if (!lstr || !rstr) return std::nullopt;
                const char* opname = (q.op == BinaryOp::Sub) ? "sub" : "add";
                return std::string(opname) + " (i64 " + *lstr + ", i64 " + *rstr + ")";
              } else if constexpr (std::is_same_v<U, IntLiteral>) {
                return std::to_string(q.value);
              } else {
                return std::nullopt;
              }
            }, ex.payload);
          };
          auto lstr = emit_label_ce(emit_label_ce, p.lhs);
          auto rstr = emit_label_ce(emit_label_ce, p.rhs);
          if (lstr && rstr) {
            const char* opname = (p.op == BinaryOp::Sub) ? "sub" : "add";
            std::string ce = std::string(opname) + " (i64 " + *lstr + ", i64 " + *rstr + ")";
            // Truncate to target type if narrower than i64
            const std::string tgt_ty = llvm_ty(expected_ts);
            if (tgt_ty != "i64" && tgt_ty != "ptr") {
              ce = "trunc (i64 " + ce + " to " + tgt_ty + ")";
            }
            return ce;
          }
        }
        return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
      } else if constexpr (std::is_same_v<T, LabelAddrExpr>) {
        return "blockaddress(@" + p.fn_name + ", %ulbl_" + p.label_name + ")";
      } else if constexpr (std::is_same_v<T, CallExpr>) {
        const BuiltinId builtin_id = p.builtin_id;
        switch (builtin_constant_fp_kind(builtin_id)) {
          case BuiltinConstantFpKind::Infinity:
            if (builtin_id == BuiltinId::HugeValF || builtin_id == BuiltinId::InfF) {
              if (expected_ts.base == TB_FLOAT && expected_ts.ptr_level == 0)
                return fp_to_float_literal(std::numeric_limits<float>::infinity());
              return fp_to_hex(static_cast<double>(std::numeric_limits<float>::infinity()));
            }
            return fp_literal(expected_ts.base, std::numeric_limits<double>::infinity());
          case BuiltinConstantFpKind::QuietNaN:
            if (builtin_id == BuiltinId::NanF || builtin_id == BuiltinId::NansF) {
              if (expected_ts.base == TB_FLOAT && expected_ts.ptr_level == 0)
                return fp_to_float_literal(std::numeric_limits<float>::quiet_NaN());
              return fp_to_hex(static_cast<double>(std::numeric_limits<float>::quiet_NaN()));
            }
            return fp_literal(expected_ts.base, std::numeric_limits<double>::quiet_NaN());
          case BuiltinConstantFpKind::None:
            break;
        }
        return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
      } else if constexpr (std::is_same_v<T, SizeofTypeExpr>) {
        return std::to_string(sizeof_ts(mod_, p.type.spec));
      } else if constexpr (std::is_same_v<T, SizeofExpr>) {
        auto val = try_const_eval_int(id);
        if (val) return std::to_string(*val);
        return "0";
      } else {
        // Try integer constant evaluation as a last resort
        auto val = try_const_eval_int(id);
        if (val) return std::to_string(*val);
        return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
      }
    }, e.payload);
  }

void HirEmitter::emit_global(const GlobalVar& gv){
    // Global object types arrive from HIR with ordinary array bounds already resolved.
    const TypeSpec& ts = gv.type.spec;
    auto push_global = [&](const std::string& line) {
      lir::LirGlobal lg;
      lg.name = gv.name;
      lg.type = ts;
      lg.is_internal = gv.linkage.is_static;
      lg.is_const = gv.is_const;
      lg.raw_line = line;
      module_.globals.push_back(std::move(lg));
    };
    if (!gv.linkage.is_extern &&
        ts.ptr_level == 0 &&
        ts.array_rank == 0 &&
        ts.tag && ts.tag[0] &&
        ts.base == TB_STRUCT) {
      const auto it = mod_.struct_defs.find(ts.tag);
      if (it != mod_.struct_defs.end()) {
        const HirStructDef& sd = it->second;
        if (!sd.is_union && !sd.fields.empty() && sd.fields.back().is_flexible_array) {
          std::vector<TypeSpec> field_types;
          const auto field_vals = emit_const_struct_fields(ts, sd, gv.init, &field_types);
          const TypeSpec& last_ts = field_types.back();
          if (last_ts.array_rank > 0 && last_ts.array_size > 0) {
            std::string lk = gv.linkage.is_static ? "internal " : "";
            if (gv.linkage.is_weak && !gv.linkage.is_static) lk = "weak ";
            lk += llvm_visibility(gv.linkage.visibility);
            const std::string qual = (gv.is_const && ts.ptr_level == 0) ? "constant " : "global ";
            std::string literal_ty = "{ ";
            std::string literal_init = "{ ";
            for (size_t i = 0; i < sd.fields.size(); ++i) {
              if (i) {
                literal_ty += ", ";
                literal_init += ", ";
              }
              literal_ty += llvm_alloca_ty(field_types[i]);
              literal_init += llvm_alloca_ty(field_types[i]) + " " + field_vals[i];
            }
            literal_ty += " }";
            literal_init += " }";
            push_global(llvm_global_sym(gv.name) + " = " + lk + qual +
                        literal_ty + " " + literal_init);
            return;
          }
        }
      }
    }
    const std::string ty = llvm_alloca_ty(ts);
    if (gv.linkage.is_extern) {
      std::string ext = gv.linkage.is_weak ? "extern_weak " : "external ";
      ext += llvm_visibility(gv.linkage.visibility);
      push_global(llvm_global_sym(gv.name) + " = " + ext + "global " + ty);
      return;
    }
    std::string lk = gv.linkage.is_static ? "internal " : "";
    if (gv.linkage.is_weak && !gv.linkage.is_static) lk = "weak ";
    lk += llvm_visibility(gv.linkage.visibility);
    const std::string qual = (gv.is_const && ts.ptr_level == 0) ? "constant " : "global ";
    const std::string init = emit_const_init(ts, gv.init);
    push_global(llvm_global_sym(gv.name) + " = " + lk + qual + ty + " " + init);
  }

const Expr& HirEmitter::get_expr(ExprId id) const{
    for (const auto& e : mod_.expr_pool)
      if (e.id.value == id.value) return e;
    throw std::runtime_error("HirEmitter: expr not found id=" + std::to_string(id.value));
  }

std::string HirEmitter::coerce(FnCtx& ctx, const std::string& val,
                     const TypeSpec& from_ts, const TypeSpec& to_ts){
    const std::string ft = llvm_ty(from_ts);
    const std::string tt = llvm_ty(to_ts);
    if (tt == "ptr" && val == "0") return "null";
    if (ft == tt) return val;
    if (ft == "ptr" && tt == "ptr") return val;

    // Reinterpret between same-sized vector and integer scalars.
    if (((is_vector_value(from_ts) && is_any_int(to_ts.base) && to_ts.ptr_level == 0 &&
          to_ts.array_rank == 0) ||
         (is_any_int(from_ts.base) && from_ts.ptr_level == 0 && from_ts.array_rank == 0 &&
          is_vector_value(to_ts))) &&
        sizeof_ts(mod_, from_ts) == sizeof_ts(mod_, to_ts)) {
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = bitcast " + ft + " " + val + " to " + tt);
      return tmp;
    }

    if (is_complex_base(from_ts.base) && is_complex_base(to_ts.base)) {
      const TypeSpec from_elem_ts = complex_component_ts(from_ts.base);
      const TypeSpec to_elem_ts = complex_component_ts(to_ts.base);
      const std::string real0 = fresh_tmp(ctx);
      emit_instr(ctx, real0 + " = extractvalue " + ft + " " + val + ", 0");
      const std::string imag0 = fresh_tmp(ctx);
      emit_instr(ctx, imag0 + " = extractvalue " + ft + " " + val + ", 1");
      const std::string real1 = coerce(ctx, real0, from_elem_ts, to_elem_ts);
      const std::string imag1 = coerce(ctx, imag0, from_elem_ts, to_elem_ts);
      const std::string with_real = fresh_tmp(ctx);
      emit_instr(ctx, with_real + " = insertvalue " + tt + " undef, " + llvm_ty(to_elem_ts) +
                                 " " + real1 + ", 0");
      const std::string out = fresh_tmp(ctx);
      emit_instr(ctx, out + " = insertvalue " + tt + " " + with_real + ", " +
                            llvm_ty(to_elem_ts) + " " + imag1 + ", 1");
      return out;
    }

    if (!is_complex_base(from_ts.base) && is_complex_base(to_ts.base) &&
        from_ts.ptr_level == 0 && from_ts.array_rank == 0) {
      const TypeSpec to_elem_ts = complex_component_ts(to_ts.base);
      const std::string real = coerce(ctx, val, from_ts, to_elem_ts);
      const std::string elem_ty = llvm_ty(to_elem_ts);
      const std::string zero =
          is_float_base(to_elem_ts.base) ? fp_literal(to_elem_ts.base, 0.0) : "0";
      const std::string with_real = fresh_tmp(ctx);
      emit_instr(ctx, with_real + " = insertvalue " + tt + " undef, " + elem_ty +
                                 " " + real + ", 0");
      const std::string out = fresh_tmp(ctx);
      emit_instr(ctx, out + " = insertvalue " + tt + " " + with_real + ", " +
                            elem_ty + " " + zero + ", 1");
      return out;
    }

    // i1 → wider int
    if (ft == "i1" && tt != "ptr" && tt != "void") {
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = zext i1 " + val + " to " + tt);
      return tmp;
    }

    // int → int
    if (from_ts.ptr_level == 0 && from_ts.array_rank == 0 &&
        to_ts.ptr_level == 0 && to_ts.array_rank == 0 &&
        is_any_int(from_ts.base) && is_any_int(to_ts.base)) {
      const int fb = int_bits(from_ts.base), tb = int_bits(to_ts.base);
      if (fb == tb) return val;
      const std::string tmp = fresh_tmp(ctx);
      if (tb > fb) {
        const std::string ext = is_signed_int(from_ts.base) ? "sext" : "zext";
        emit_instr(ctx, tmp + " = " + ext + " " + ft + " " + val + " to " + tt);
      } else {
        emit_instr(ctx, tmp + " = trunc " + ft + " " + val + " to " + tt);
      }
      return tmp;
    }

    // float → float
    if (is_float_base(from_ts.base) && is_float_base(to_ts.base)) {
      const int fb = (from_ts.base == TB_FLOAT) ? 32 : (from_ts.base == TB_LONGDOUBLE ? 128 : 64);
      const int tb = (to_ts.base == TB_FLOAT) ? 32 : (to_ts.base == TB_LONGDOUBLE ? 128 : 64);
      if (fb == tb) return val;
      const std::string tmp = fresh_tmp(ctx);
      const std::string op = (tb > fb) ? "fpext" : "fptrunc";
      emit_instr(ctx, tmp + " = " + op + " " + ft + " " + val + " to " + tt);
      return tmp;
    }

    // int → float (only for non-pointer scalar float types)
    if (is_any_int(from_ts.base) && from_ts.ptr_level == 0 &&
        is_float_base(to_ts.base) && to_ts.ptr_level == 0 && to_ts.array_rank == 0) {
      const std::string tmp = fresh_tmp(ctx);
      const std::string op = is_signed_int(from_ts.base) ? "sitofp" : "uitofp";
      emit_instr(ctx, tmp + " = " + op + " " + ft + " " + val + " to " + tt);
      return tmp;
    }

    // float → int (only for non-pointer scalar types)
    if (is_float_base(from_ts.base) && from_ts.ptr_level == 0 && from_ts.array_rank == 0 &&
        is_any_int(to_ts.base) && to_ts.ptr_level == 0) {
      const std::string tmp = fresh_tmp(ctx);
      const std::string op = is_signed_int(to_ts.base) ? "fptosi" : "fptoui";
      emit_instr(ctx, tmp + " = " + op + " " + ft + " " + val + " to " + tt);
      return tmp;
    }

    // Reference dereference: ptr (reference) → value type by loading
    if (ft == "ptr" && (from_ts.is_lvalue_ref || from_ts.is_rvalue_ref) &&
        to_ts.ptr_level == 0 && !to_ts.is_lvalue_ref && !to_ts.is_rvalue_ref &&
        !to_ts.is_fn_ptr) {
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = load " + tt + ", ptr " + val);
      return tmp;
    }

    // ptr ↔ int
    if (ft == "ptr" && is_any_int(to_ts.base)) {
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = ptrtoint ptr " + val + " to " + tt);
      return tmp;
    }
    if (is_any_int(from_ts.base) && tt == "ptr") {
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = inttoptr " + ft + " " + val + " to ptr");
      return tmp;
    }

    // Same size numeric: bitcast
    return val;
  }

std::string HirEmitter::to_bool(FnCtx& ctx, const std::string& val, const TypeSpec& ts){
    const std::string ty = llvm_ty(ts);
    if (ty == "i1") return val;
    const std::string tmp = fresh_tmp(ctx);
    // Check ptr FIRST: a TypeSpec with ptr_level>0 always uses icmp even if the
    // base is a float type (e.g. function pointer typed as float-return + ptr_level=1).
    if (ty == "ptr") {
      emit_instr(ctx, tmp + " = icmp ne ptr " + val + ", null");
    } else if (is_float_base(ts.base) && ts.ptr_level == 0 && ts.array_rank == 0) {
      emit_instr(ctx, tmp + " = fcmp une " + ty + " " + val + ", " + fp_literal(ts.base, 0.0));
    } else {
      emit_instr(ctx, tmp + " = icmp ne " + ty + " " + val + ", 0");
    }
    return tmp;
  }

bool HirEmitter::find_field_chain(const std::string& tag, const std::string& field_name,
                        std::vector<FieldStep>& chain, TypeSpec& out_field_ts){
    const auto it = mod_.struct_defs.find(tag);
    if (it == mod_.struct_defs.end()) return false;
    const HirStructDef& sd = it->second;

    // Direct lookup
    for (const auto& f : sd.fields) {
      if (!f.is_anon_member && f.name == field_name) {
        FieldStep step{tag, llvm_struct_field_slot(sd, f.llvm_idx), sd.is_union};
        if (f.bit_width >= 0) {
          step.bit_width = f.bit_width;
          step.bit_offset = f.bit_offset;
          step.storage_unit_bits = f.storage_unit_bits;
          step.bf_is_signed = f.is_bf_signed;
        }
        chain.push_back(std::move(step));
        out_field_ts = field_decl_type(f);  // includes array dims if array field
        return true;
      }
    }
    // Recursive: search anonymous embedded members
    for (const auto& f : sd.fields) {
      if (!f.is_anon_member) continue;
      if (!f.elem_type.tag || !f.elem_type.tag[0]) continue;
      std::vector<FieldStep> sub_chain;
      TypeSpec sub_ts{};
      if (find_field_chain(f.elem_type.tag, field_name, sub_chain, sub_ts)) {
        // prepend: current struct's step to reach the anon member
        chain.push_back({tag, llvm_struct_field_slot(sd, f.llvm_idx), sd.is_union});
        for (const auto& s : sub_chain) chain.push_back(s);
        out_field_ts = sub_ts;
        return true;
      }
    }
    return false;
  }

std::string HirEmitter::emit_member_gep(FnCtx& ctx, const std::string& base_ptr,
                               const std::string& tag, const std::string& field_name,
                               TypeSpec& out_field_ts,
                               BitfieldAccess* out_bf){
    std::vector<FieldStep> chain;
    if (!find_field_chain(tag, field_name, chain, out_field_ts)) {
      throw std::runtime_error(
          "HirEmitter: field '" + field_name + "' not found in struct/union '" + tag + "'");
    }
    std::string cur_ptr = base_ptr;
    for (const auto& step : chain) {
      const std::string sty = llvm_struct_type_str(step.tag);
      if (step.is_union) {
        // Union: GEP to field 0 (byte array); with opaque ptrs same addr
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = getelementptr " + sty +
                           ", ptr " + cur_ptr + ", i32 0, i32 0");
        cur_ptr = tmp;
      } else {
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = getelementptr " + sty +
                           ", ptr " + cur_ptr + ", i32 0, i32 " +
                           std::to_string(step.llvm_idx));
        cur_ptr = tmp;
      }
    }
    // Propagate bitfield info from the last (innermost) step
    if (out_bf && !chain.empty()) {
      const auto& last = chain.back();
      out_bf->bit_width = last.bit_width;
      out_bf->bit_offset = last.bit_offset;
      out_bf->storage_unit_bits = last.storage_unit_bits;
      out_bf->is_signed = last.bf_is_signed;
    }
    return cur_ptr;
  }

int HirEmitter::bitfield_promoted_bits(const BitfieldAccess& bf){
    if (bf.bit_width <= 31) return 32;  // fits in int
    if (bf.bit_width == 32) return 32;  // fits in int (signed) or uint (unsigned)
    return bf.storage_unit_bits;        // >32 bits: use storage type (i64)
  }

TypeBase HirEmitter::bitfield_promoted_base(int bit_width, bool is_signed, int storage_unit_bits){
    if (bit_width < 32) return TB_INT;
    if (bit_width == 32) return is_signed ? TB_INT : TB_UINT;
    if (storage_unit_bits > 32) return is_signed ? TB_LONGLONG : TB_ULONGLONG;
    return is_signed ? TB_INT : TB_UINT;
  }

TypeSpec HirEmitter::bitfield_promoted_ts(const BitfieldAccess& bf){
    TypeSpec ts{};
    ts.base = bitfield_promoted_base(bf.bit_width, bf.is_signed, bf.storage_unit_bits);
    return ts;
  }

std::string HirEmitter::emit_bitfield_load(FnCtx& ctx, const std::string& unit_ptr,
                                  const BitfieldAccess& bf){
    const std::string result = fresh_tmp(ctx);
    lir::LirBitfieldExtract op;
    op.result = result;
    op.unit_ptr = unit_ptr;
    op.bit_width = bf.bit_width;
    op.bit_offset = bf.bit_offset;
    op.storage_unit_bits = bf.storage_unit_bits;
    op.is_signed = bf.is_signed;
    op.promoted_bits = bitfield_promoted_bits(bf);
    emit_lir_op(ctx, op);
    return result;
  }

void HirEmitter::emit_bitfield_store(FnCtx& ctx, const std::string& unit_ptr,
                            const BitfieldAccess& bf,
                            const std::string& new_val, const TypeSpec& val_ts){
    // Coerce new_val to storage unit type
    TypeSpec unit_ts{};
    switch (bf.storage_unit_bits) {
      case 8:  unit_ts.base = TB_UCHAR; break;
      case 16: unit_ts.base = TB_USHORT; break;
      case 32: unit_ts.base = TB_UINT; break;
      case 64: unit_ts.base = TB_ULONGLONG; break;
      default: unit_ts.base = TB_UINT; break;
    }
    std::string val_coerced = coerce(ctx, new_val, val_ts, unit_ts);
    const std::string scratch = fresh_tmp(ctx);
    lir::LirBitfieldInsert op;
    op.unit_ptr = unit_ptr;
    op.new_val = val_coerced;
    op.scratch = scratch;
    op.bit_width = bf.bit_width;
    op.bit_offset = bf.bit_offset;
    op.storage_unit_bits = bf.storage_unit_bits;
    emit_lir_op(ctx, op);
  }

std::string HirEmitter::emit_lval(FnCtx& ctx, ExprId id, TypeSpec& pointee_ts){
    const Expr& e = get_expr(id);
    return emit_lval_dispatch(ctx, e, pointee_ts);
  }

std::string HirEmitter::emit_va_list_obj_ptr(FnCtx& ctx, ExprId id, TypeSpec& ts){
    const Expr& e = get_expr(id);
    ts = resolve_expr_type(ctx, id);
    if (const auto* r = std::get_if<DeclRef>(&e.payload)) {
      if (r->param_index && ctx.fn && *r->param_index < ctx.fn->params.size()) {
        const TypeSpec& pts = ctx.fn->params[*r->param_index].type.spec;
        if (pts.base == TB_VA_LIST && pts.ptr_level == 0 && pts.array_rank == 0) {
          const auto spill_it = ctx.param_slots.find(*r->param_index + 0x80000000u);
          if (spill_it != ctx.param_slots.end()) {
            const std::string tmp = fresh_tmp(ctx);
            emit_instr(ctx, tmp + " = load ptr, ptr " + spill_it->second);
            return tmp;
          }
          const auto it = ctx.param_slots.find(*r->param_index);
          if (it != ctx.param_slots.end()) return it->second;
        }
      }
    }
    return emit_lval(ctx, id, ts);
  }

std::string HirEmitter::emit_lval_dispatch(FnCtx& ctx, const Expr& e, TypeSpec& pts){
    if (const auto* r = std::get_if<DeclRef>(&e.payload)) {
      if (r->local) {
        pts = ctx.local_types.at(r->local->value);
        return ctx.local_slots.at(r->local->value);
      }
      // Parameter used as lvalue (e.g. p++): spill to a stack slot on first use
      if (r->param_index && ctx.fn && *r->param_index < ctx.fn->params.size()) {
        const auto& param = ctx.fn->params[*r->param_index];
        pts = param.type.spec;
        const std::string pname = "%p." + sanitize_llvm_ident(param.name);
        // Check if we already have a spill slot
        auto it = ctx.param_slots.find(*r->param_index + 0x80000000u);
        if (it != ctx.param_slots.end()) {
          return it->second;
        }
        // Create a new alloca for this parameter
        const std::string slot = "%lv.param." + sanitize_llvm_ident(param.name);
        ctx.alloca_insts.push_back(lir::LirRawLine{"  " + slot + " = alloca " + llvm_alloca_ty(pts)});
        ctx.alloca_insts.push_back(lir::LirRawLine{"  store " + llvm_ty(pts) + " " + pname + ", ptr " + slot});
        ctx.param_slots[*r->param_index + 0x80000000u] = slot;
        return slot;
      }
      if (r->global) {
        size_t gv_idx = r->global->value;
        const auto& gv0 = mod_.globals[gv_idx];
        if (const GlobalVar* best = select_global_object(gv0.name)) gv_idx = best->id.value;
        const auto& gv = mod_.globals[gv_idx];
        pts = gv.type.spec;
        return llvm_global_sym(gv.name);
      }
      // Unresolved: assume external global
      pts = e.type.spec;
      return llvm_global_sym(r->name);
    }
    if (const auto* u = std::get_if<UnaryExpr>(&e.payload)) {
      if (u->op == UnaryOp::Deref) {
        TypeSpec ptr_ts{};
        const std::string ptr = emit_rval_id(ctx, u->operand, ptr_ts);
        pts = ptr_ts;
        if (pts.ptr_level > 0) pts.ptr_level--;
        return ptr;
      }
      if (u->op == UnaryOp::RealPart || u->op == UnaryOp::ImagPart) {
        TypeSpec complex_ts{};
        const std::string complex_ptr = emit_lval(ctx, u->operand, complex_ts);
        if (!is_complex_base(complex_ts.base)) {
          throw std::runtime_error("HirEmitter: real/imag lvalue on non-complex expr");
        }
        pts = complex_component_ts(complex_ts.base);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = getelementptr " + llvm_alloca_ty(complex_ts) +
                            ", ptr " + complex_ptr + ", i32 0, i32 " +
                            std::to_string(u->op == UnaryOp::ImagPart ? 1 : 0));
        return tmp;
      }
    }
    if (const auto* idx = std::get_if<IndexExpr>(&e.payload)) {
      TypeSpec base_ts{};
      std::string base;
      if (const TypeSpec resolved_base_ts = resolve_expr_type(ctx, idx->base);
          is_vector_value(resolved_base_ts)) {
        TypeSpec obj_ts{};
        base = emit_lval(ctx, idx->base, obj_ts);
        base_ts = obj_ts;
      } else {
        base = emit_rval_id(ctx, idx->base, base_ts);
      }
      TypeSpec ix_ts{};
      const std::string ix = emit_rval_id(ctx, idx->index, ix_ts);
      // Widen index to i64
      TypeSpec i64_ts{}; i64_ts.base = TB_LONGLONG;
      const std::string ix64 = coerce(ctx, ix, ix_ts, i64_ts);
      // Element type: strip one array or pointer level with proper dim shifting.
      pts = base_ts;
      if (is_vector_value(pts)) {
        pts.is_vector = false;
        pts.vector_lanes = 0;
        pts.vector_bytes = 0;
      } else if (pts.ptr_level > 0 && pts.is_ptr_to_array) {
        // Pointer-to-array: subscript consumes the pointer layer first.
        pts.ptr_level--;
      } else if (pts.array_rank > 0) {
        pts = drop_one_array_dim(pts);
      } else if (pts.ptr_level > 0) {
        pts.ptr_level--;
      }
      const std::string tmp = fresh_tmp(ctx);
      // Use alloca type (non-decayed) for array elements so GEP advances by the
      // correct stride ([N x T] instead of ptr).
      const std::string elem_ty = (pts.array_rank > 0)
                                      ? llvm_alloca_ty(pts)
                                      : llvm_ty(pts);
      const std::string safe_elem_ty = (elem_ty == "void") ? "i8" : elem_ty;
      emit_instr(ctx, tmp + " = getelementptr " + safe_elem_ty +
                          ", ptr " + base + ", i64 " + ix64);
      return tmp;
    }
    if (const auto* m = std::get_if<MemberExpr>(&e.payload)) {
      return emit_member_lval(ctx, *m, pts);
    }
    // Reference-type cast preserves addressability (xvalue semantics).
    // e.g. static_cast<T&&>(x) → same address as x.
    if (const auto* c = std::get_if<CastExpr>(&e.payload)) {
      if (c->to_type.spec.is_rvalue_ref || c->to_type.spec.is_lvalue_ref) {
        return emit_lval(ctx, c->expr, pts);
      }
    }
    throw std::runtime_error("HirEmitter: cannot take lval of expr");
  }

std::string HirEmitter::emit_member_lval(FnCtx& ctx, const MemberExpr& m, TypeSpec& out_pts,
                                BitfieldAccess* out_bf){
    // Get base pointer
    const Expr& base_e = get_expr(m.base);
    TypeSpec base_ts = base_e.type.spec;
    if (base_ts.base == TB_VOID && base_ts.ptr_level == 0)
      base_ts = resolve_expr_type(ctx, m.base);

    std::string base_ptr;
    if (m.is_arrow) {
      // expr->field: load the pointer from the base expression
      TypeSpec ptr_ts{};
      base_ptr = emit_rval_id(ctx, m.base, ptr_ts);
      base_ts = ptr_ts;
      // Strip one pointer level for the struct type
      if (base_ts.ptr_level > 0) base_ts.ptr_level--;
    } else {
      // expr.field: prefer direct lvalue; for struct rvalues materialize a temp.
      TypeSpec dummy{};
      try {
        base_ptr = emit_lval_dispatch(ctx, base_e, dummy);
      } catch (const std::runtime_error&) {
        if (base_ts.base != TB_STRUCT && base_ts.base != TB_UNION) {
          throw;
        }
        TypeSpec rval_ts{};
        const std::string rval = emit_rval_id(ctx, m.base, rval_ts);
        if (rval_ts.base != TB_VOID || rval_ts.ptr_level > 0 || rval_ts.array_rank > 0) {
          base_ts = rval_ts;
        }
        const std::string slot = fresh_tmp(ctx) + ".agg";
        ctx.alloca_insts.push_back(lir::LirRawLine{"  " + slot + " = alloca " + llvm_alloca_ty(base_ts)});
        emit_instr(ctx, "store " + llvm_ty(base_ts) + " " + rval + ", ptr " + slot);
        base_ptr = slot;
      }
    }

    const char* tag = base_ts.tag;
    if (!tag || !tag[0]) {
      throw std::runtime_error(
          "HirEmitter: MemberExpr base has no struct tag (field='" + m.field + "')");
    }
    return emit_member_gep(ctx, base_ptr, tag, m.field, out_pts, out_bf);
  }

TypeSpec HirEmitter::resolve_expr_type(FnCtx& ctx, ExprId id){
    const Expr& e = get_expr(id);
    // Use stored type if not void
    const TypeSpec& ts = e.type.spec;
    if (ts.base != TB_VOID || ts.ptr_level > 0 || ts.array_rank > 0 ||
        is_vector_value(ts))
      return ts;

    return std::visit([&](const auto& p) -> TypeSpec {
      return resolve_payload_type(ctx, p);
    }, e.payload);
  }

TypeSpec HirEmitter::resolve_expr_type(FnCtx& ctx, const Expr& e){
    if (has_concrete_type(e.type.spec)) return e.type.spec;
    return std::visit([&](const auto& p) -> TypeSpec {
      return resolve_payload_type(ctx, p);
    }, e.payload);
  }

const FnPtrSig* HirEmitter::resolve_callee_fn_ptr_sig(FnCtx& ctx, const Expr& callee_e){
    if (const auto* u = std::get_if<UnaryExpr>(&callee_e.payload)) {
      if (u->op == UnaryOp::Deref) {
        const Expr& inner_e = get_expr(u->operand);
        return resolve_callee_fn_ptr_sig(ctx, inner_e);
      }
    }
    if (const auto* dr = std::get_if<DeclRef>(&callee_e.payload)) {
      if (dr->local) {
        const auto it = ctx.local_fn_ptr_sigs.find(dr->local->value);
        if (it != ctx.local_fn_ptr_sigs.end()) return &it->second;
      }
      if (dr->param_index) {
        const auto it = ctx.param_fn_ptr_sigs.find(*dr->param_index);
        if (it != ctx.param_fn_ptr_sigs.end()) return &it->second;
      }
      if (dr->global) {
        const auto it = ctx.global_fn_ptr_sigs.find(dr->global->value);
        if (it != ctx.global_fn_ptr_sigs.end()) return &it->second;
      }
    }
    // Phase C: MemberExpr — look up the struct field's fn_ptr_sig.
    if (const auto* m = std::get_if<MemberExpr>(&callee_e.payload)) {
      TypeSpec base_ts = resolve_expr_type(ctx, m->base);
      if (m->is_arrow && base_ts.ptr_level > 0) base_ts.ptr_level--;
      if (base_ts.tag && base_ts.tag[0]) {
        const auto sit = mod_.struct_defs.find(base_ts.tag);
        if (sit != mod_.struct_defs.end()) {
          for (const auto& f : sit->second.fields) {
            if (f.name == m->field && f.fn_ptr_sig) {
              return &*f.fn_ptr_sig;
            }
          }
        }
      }
    }
    // Phase C: TernaryExpr — recurse on the then branch.
    if (const auto* t = std::get_if<TernaryExpr>(&callee_e.payload)) {
      return resolve_callee_fn_ptr_sig(ctx, get_expr(t->then_expr));
    }
    // Phase C slice 2: CastExpr — use the cast target's fn_ptr_sig.
    if (const auto* c = std::get_if<CastExpr>(&callee_e.payload)) {
      if (c->fn_ptr_sig) return &*c->fn_ptr_sig;
    }
    // Phase C slice 2: IndexExpr — recurse on base (array/ptr of fn_ptrs).
    if (const auto* idx = std::get_if<IndexExpr>(&callee_e.payload)) {
      return resolve_callee_fn_ptr_sig(ctx, get_expr(idx->base));
    }
    // Phase C slice 2: CallExpr — look up called function's ret_fn_ptr_sig.
    if (const auto* call = std::get_if<CallExpr>(&callee_e.payload)) {
      const Expr& inner_callee = get_expr(call->callee);
      if (const auto* dr = std::get_if<DeclRef>(&inner_callee.payload)) {
        const auto fit = mod_.fn_index.find(dr->name);
        if (fit != mod_.fn_index.end() && fit->second.value < mod_.functions.size()) {
          const Function& target = mod_.functions[fit->second.value];
          if (target.ret_fn_ptr_sig) return &*target.ret_fn_ptr_sig;
        }
      }
    }
    return nullptr;
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx& ctx, const DeclRef& r){
    if (r.local) {
      const auto it = ctx.local_types.find(r.local->value);
      if (it != ctx.local_types.end()) return it->second;
    }
    if (r.param_index && ctx.fn && *r.param_index < ctx.fn->params.size())
      return ctx.fn->params[*r.param_index].type.spec;
    if (r.global) {
      if (const GlobalVar* gv = select_global_object(*r.global)) return gv->type.spec;
    }
    return {};
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx& ctx, const BinaryExpr& b){
    switch (b.op) {
      case BinaryOp::Lt:
      case BinaryOp::Le:
      case BinaryOp::Gt:
      case BinaryOp::Ge:
      case BinaryOp::Eq:
      case BinaryOp::Ne:
      case BinaryOp::LAnd:
      case BinaryOp::LOr: {
        TypeSpec ts{};
        ts.base = TB_INT;
        return ts;
      }
      case BinaryOp::Comma: {
        // Comma operator: result type/value is from RHS
        const Expr& rhs_e = get_expr(b.rhs);
        return std::visit([&](const auto& p) -> TypeSpec {
          return resolve_payload_type(ctx, p);
        }, rhs_e.payload);
      }
      default:
        break;
    }
    const TypeSpec lts = resolve_expr_type(ctx, b.lhs);
    const TypeSpec rts = resolve_expr_type(ctx, b.rhs);

    if (b.op == BinaryOp::Sub && llvm_ty(lts) == "ptr" && llvm_ty(rts) == "ptr") {
      TypeSpec ts{};
      ts.base = TB_LONGLONG;
      return ts;
    }
    if ((b.op == BinaryOp::Add || b.op == BinaryOp::Sub) &&
        llvm_ty(lts) == "ptr" &&
        rts.ptr_level == 0 && rts.array_rank == 0 && is_any_int(rts.base)) {
      return lts;
    }
    if (b.op == BinaryOp::Add &&
        llvm_ty(rts) == "ptr" &&
        lts.ptr_level == 0 && lts.array_rank == 0 && is_any_int(lts.base)) {
      return rts;
    }
    if (is_complex_base(lts.base) || is_complex_base(rts.base)) {
      if (!is_complex_base(lts.base)) return rts;
      if (!is_complex_base(rts.base)) return lts;
      return sizeof_ts(mod_, lts) >= sizeof_ts(mod_, rts) ? lts : rts;
    }
    if (is_vector_value(lts)) return lts;
    if (is_vector_value(rts)) return rts;

    // For arithmetic/bitwise the result type is the UAC common type.
    if (lts.base != TB_VOID && rts.base != TB_VOID &&
        lts.ptr_level == 0 && rts.ptr_level == 0 &&
        lts.array_rank == 0 && rts.array_rank == 0 &&
        is_any_int(lts.base) && is_any_int(rts.base)) {
      const bool is_shift = (b.op == BinaryOp::Shl || b.op == BinaryOp::Shr);
      TypeSpec ts{};
      ts.base = is_shift ? integer_promote(lts.base) : usual_arith_conv(lts.base, rts.base);
      return ts;
    }
    if (lts.base != TB_VOID) return lts;
    return resolve_expr_type(ctx, b.rhs);
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx& ctx, const UnaryExpr& u){
    TypeSpec ts = resolve_expr_type(ctx, u.operand);
    switch (u.op) {
      case UnaryOp::AddrOf:
        if (ts.array_rank > 0 && !is_vector_value(ts)) {
          // &array produces a pointer-to-array; preserve array dims so that
          // pointer arithmetic uses the correct stride and deref produces
          // array decay instead of a scalar load.
          ts.is_ptr_to_array = true;
        }
        ts.ptr_level += 1;
        return ts;
      case UnaryOp::Deref:
        if (ts.ptr_level > 0) {
          ts.ptr_level -= 1;
          // After consuming the pointer level, the result is an array value
          // (not a pointer-to-array), so clear the flag.
          if (ts.ptr_level == 0) ts.is_ptr_to_array = false;
        } else if (ts.array_rank > 0) {
          ts.array_rank -= 1;
        }
        return ts;
      case UnaryOp::RealPart:
      case UnaryOp::ImagPart:
        if (is_complex_base(ts.base)) return complex_component_ts(ts.base);
        return ts;
      case UnaryOp::Not: {
        TypeSpec out{};
        out.base = TB_INT;
        return out;
      }
      case UnaryOp::BitNot:
      case UnaryOp::Plus:
      case UnaryOp::Minus:
        if (ts.ptr_level == 0 && !is_vector_value(ts))
          ts.base = integer_promote(ts.base);
        return ts;
      default:
        return ts;
    }
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx& ctx, const AssignExpr& a){
    return resolve_expr_type(ctx, a.lhs);
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx& ctx, const CastExpr& c){
    return c.to_type.spec;
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx& ctx, const CallExpr& c){
    const BuiltinId builtin_id = c.builtin_id;
    if (builtin_id != BuiltinId::Unknown) {
      if (builtin_id == BuiltinId::Expect && !c.args.empty())
        return resolve_expr_type(ctx, c.args[0]);
      switch (builtin_id) {
        case BuiltinId::Unreachable:
        case BuiltinId::VaStart:
        case BuiltinId::VaEnd:
        case BuiltinId::VaCopy:
        case BuiltinId::Prefetch:
          return {};
        case BuiltinId::Bswap16:
        case BuiltinId::Bswap32:
        case BuiltinId::Bswap64:
          if (!c.args.empty()) return resolve_expr_type(ctx, c.args[0]);
          break;
        default:
          break;
      }
      if (auto builtin_ts = type_spec_from_builtin_result_kind(
              builtin_result_kind(builtin_id))) {
        return *builtin_ts;
      }
    }
    const Expr& callee_e = get_expr(c.callee);
    // Direct function call: look up return type from the function definition.
    if (const auto* r = std::get_if<DeclRef>(&callee_e.payload)) {
      const auto fit = mod_.fn_index.find(r->name);
      if (fit != mod_.fn_index.end() && fit->second.value < mod_.functions.size()) {
        TypeSpec ret = mod_.functions[fit->second.value].return_type.spec;
        // Reference return: the LLVM call actually returns ptr, so bump ptr_level
        // to keep the type annotation consistent with the LLVM value.
        if ((ret.is_lvalue_ref || ret.is_rvalue_ref) && ret.ptr_level == 0) ret.ptr_level++;
        return ret;
      }
    }
    // Indirect call through fn-ptr: use canonical fn_ptr_sig for return type.
    if (const FnPtrSig* sig = resolve_callee_fn_ptr_sig(ctx, callee_e)) {
      return sig_return_type(*sig);
    }
    // Check known libc return types before falling back to implicit int
    if (const auto* r = std::get_if<DeclRef>(&callee_e.payload)) {
      if (auto kts = type_spec_from_builtin_result_kind(
              known_call_result_kind(r->name.c_str()))) {
        return *kts;
      }
    }
    // Unknown external call: C implicit declaration defaults to int return
    TypeSpec implicit_int{}; implicit_int.base = TB_INT;
    return implicit_int;
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx&, const IntLiteral&){
    TypeSpec ts{}; ts.base = TB_INT; return ts;
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx&, const FloatLiteral&){
    TypeSpec ts{}; ts.base = TB_DOUBLE; return ts;
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx&, const CharLiteral&){
    TypeSpec ts{}; ts.base = TB_CHAR; return ts;
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx&, const StringLiteral& sl){
    TypeSpec ts{};
    ts.base = sl.is_wide ? TB_INT : TB_CHAR;
    ts.ptr_level = 1;
    return ts;
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx& ctx, const MemberExpr& m){
    // Look up the field type in struct_defs
    const Expr& base_e = get_expr(m.base);
    TypeSpec base_ts = base_e.type.spec;
    if (base_ts.base == TB_VOID && base_ts.ptr_level == 0)
      base_ts = resolve_expr_type(ctx, m.base);
    if (m.is_arrow && base_ts.ptr_level > 0) base_ts.ptr_level--;
    if (!base_ts.tag || !base_ts.tag[0]) return {};
    std::vector<FieldStep> chain;
    TypeSpec field_ts{};
    find_field_chain(std::string(base_ts.tag), m.field, chain, field_ts);
    // Bitfield values are promoted per C integer promotion rules
    if (!chain.empty() && chain.back().bit_width >= 0) {
      TypeSpec bf_ts{};
      bf_ts.base = bitfield_promoted_base(
          chain.back().bit_width, chain.back().bf_is_signed, chain.back().storage_unit_bits);
      return bf_ts;
    }
    return field_ts;
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx& ctx, const IndexExpr& idx){
    TypeSpec base_ts = resolve_expr_type(ctx, idx.base);
    if (is_vector_value(base_ts)) {
      base_ts.is_vector = false;
      base_ts.vector_lanes = 0;
      base_ts.vector_bytes = 0;
      return base_ts;
    }
    if (base_ts.array_rank > 0) return drop_one_array_dim(base_ts);
    if (base_ts.ptr_level > 0) { base_ts.ptr_level--; return base_ts; }
    return base_ts;
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx& ctx, const TernaryExpr& t){
    TypeSpec ts = resolve_expr_type(ctx, t.then_expr);
    if (ts.base != TB_VOID || ts.ptr_level > 0 || ts.array_rank > 0 ||
        is_vector_value(ts))
      return ts;
    return resolve_expr_type(ctx, t.else_expr);
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx&, const VaArgExpr&){
    return {};
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx&, const SizeofExpr&){
    TypeSpec ts{};
    ts.base = TB_ULONGLONG;
    return ts;
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx&, const SizeofTypeExpr&){
    TypeSpec ts{};
    ts.base = TB_ULONGLONG;
    return ts;
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx&, const LabelAddrExpr&){
    TypeSpec ts{};
    ts.base = TB_VOID;
    ts.ptr_level = 1;
    return ts;
  }

TypeSpec HirEmitter::resolve_payload_type(FnCtx&, const PendingConstevalExpr&){
    return {};
  }

template <typename T>
  TypeSpec HirEmitter::resolve_payload_type(FnCtx&, const T&){ return {}; }

std::string HirEmitter::emit_rval_id(FnCtx& ctx, ExprId id, TypeSpec& out_ts){
    const Expr& e = get_expr(id);
    out_ts = e.type.spec;
    if (const auto* b = std::get_if<BinaryExpr>(&e.payload)) {
      // Prefer semantic reconstruction for binary expressions; parser-provided
      // AST types are often too coarse for pointer arithmetic.
      out_ts = resolve_payload_type(ctx, *b);
    } else if (const auto* c = std::get_if<CallExpr>(&e.payload)) {
      // Builtin calls such as __builtin_copysignl/__builtin_fabsl can refine
      // the return type beyond what survived from the parser AST.
      out_ts = resolve_payload_type(ctx, *c);
    }
    // AST does not annotate types on all nodes; resolve recursively.
    if (out_ts.base == TB_VOID && out_ts.ptr_level == 0 && out_ts.array_rank == 0) {
      out_ts = resolve_expr_type(ctx, id);
    }
    return emit_rval_expr(ctx, e);
  }

std::string HirEmitter::emit_rval_expr(FnCtx& ctx, const Expr& e){
    return std::visit([&](const auto& p) -> std::string {
      return emit_rval_payload(ctx, p, e);
    }, e.payload);
  }

template <typename T>
  std::string HirEmitter::emit_rval_payload(FnCtx&, const T&, const Expr&){
    throw std::runtime_error(
        std::string("HirEmitter: unimplemented expr: ") + typeid(T).name());
  }

std::string HirEmitter::emit_rval_payload(FnCtx&, const IntLiteral& x, const Expr& e){
    if (is_complex_base(e.type.spec.base)) {
      const TypeSpec elem_ts = complex_component_ts(e.type.spec.base);
      return "{ " + llvm_ty(elem_ts) + " " + emit_const_int_like(0, elem_ts) + ", " +
             llvm_ty(elem_ts) + " " + emit_const_int_like(x.value, elem_ts) + " }";
    }
    return std::to_string(x.value);
  }

std::string HirEmitter::emit_rval_payload(FnCtx&, const FloatLiteral& x, const Expr& e){
    if (is_complex_base(e.type.spec.base)) {
      const TypeSpec elem_ts = complex_component_ts(e.type.spec.base);
      const std::string imag_v = fp_literal(elem_ts.base, x.value);
      return "{ " + llvm_ty(elem_ts) + " " + emit_const_int_like(0, elem_ts) + ", " +
             llvm_ty(elem_ts) + " " + imag_v + " }";
    }
    return is_float_base(e.type.spec.base) ? fp_literal(e.type.spec.base, x.value)
                                           : fp_to_hex(x.value);
  }

std::string HirEmitter::emit_rval_payload(FnCtx&, const CharLiteral& x, const Expr&){
    return std::to_string(x.value);
  }

std::string HirEmitter::emit_complex_binary_arith(FnCtx& ctx,
                                        BinaryOp op,
                                        const std::string& lv,
                                        const TypeSpec& lts,
                                        const std::string& rv,
                                        const TypeSpec& rts,
                                        const TypeSpec& res_spec){
    TypeSpec complex_ts = lts;
    if (!is_complex_base(complex_ts.base) ||
        (is_complex_base(rts.base) && sizeof_ts(mod_, rts) > sizeof_ts(mod_, complex_ts))) {
      complex_ts = rts;
    }
    const TypeSpec elem_ts = complex_component_ts(complex_ts.base);
    auto lift_scalar_to_complex = [&](const std::string& scalar,
                                      const TypeSpec& scalar_ts) -> std::string {
      const std::string real_v = coerce(ctx, scalar, scalar_ts, elem_ts);
      const std::string imag_v = emit_const_int_like(0, elem_ts);
      const std::string with_real = fresh_tmp(ctx);
      emit_instr(ctx, with_real + " = insertvalue " + llvm_ty(complex_ts) + " undef, " +
                             llvm_ty(elem_ts) + " " + real_v + ", 0");
      const std::string out = fresh_tmp(ctx);
      emit_instr(ctx, out + " = insertvalue " + llvm_ty(complex_ts) + " " + with_real + ", " +
                          llvm_ty(elem_ts) + " " + imag_v + ", 1");
      return out;
    };

    std::string complex_lv = lv;
    std::string complex_rv = rv;
    TypeSpec complex_lts = lts;
    TypeSpec complex_rts = rts;
    if (!is_complex_base(complex_lts.base)) {
      complex_lv = lift_scalar_to_complex(complex_lv, complex_lts);
      complex_lts = complex_ts;
    } else if (llvm_ty(complex_lts) != llvm_ty(complex_ts)) {
      complex_lv = coerce(ctx, complex_lv, complex_lts, complex_ts);
      complex_lts = complex_ts;
    }
    if (!is_complex_base(complex_rts.base)) {
      complex_rv = lift_scalar_to_complex(complex_rv, complex_rts);
      complex_rts = complex_ts;
    } else if (llvm_ty(complex_rts) != llvm_ty(complex_ts)) {
      complex_rv = coerce(ctx, complex_rv, complex_rts, complex_ts);
      complex_rts = complex_ts;
    }

    const std::string lreal = fresh_tmp(ctx);
    emit_instr(ctx, lreal + " = extractvalue " + llvm_ty(complex_ts) + " " + complex_lv + ", 0");
    const std::string limag = fresh_tmp(ctx);
    emit_instr(ctx, limag + " = extractvalue " + llvm_ty(complex_ts) + " " + complex_lv + ", 1");
    const std::string rreal = fresh_tmp(ctx);
    emit_instr(ctx, rreal + " = extractvalue " + llvm_ty(complex_ts) + " " + complex_rv + ", 0");
    const std::string rimag = fresh_tmp(ctx);
    emit_instr(ctx, rimag + " = extractvalue " + llvm_ty(complex_ts) + " " + complex_rv + ", 1");

    const std::string elem_ty = llvm_ty(elem_ts);
    const char* add_instr = is_float_base(elem_ts.base) ? "fadd" : "add";
    const char* sub_instr = is_float_base(elem_ts.base) ? "fsub" : "sub";
    const char* mul_instr = is_float_base(elem_ts.base) ? "fmul" : "mul";
    const char* div_instr = is_float_base(elem_ts.base)
                                ? "fdiv"
                                : (is_signed_int(elem_ts.base) ? "sdiv" : "udiv");

    std::string out_real;
    std::string out_imag;
    if (op == BinaryOp::Add || op == BinaryOp::Sub) {
      out_real = fresh_tmp(ctx);
      emit_instr(ctx, out_real + " = " + std::string(op == BinaryOp::Add ? add_instr : sub_instr) +
                          " " + elem_ty + " " + lreal + ", " + rreal);
      out_imag = fresh_tmp(ctx);
      emit_instr(ctx, out_imag + " = " + std::string(op == BinaryOp::Add ? add_instr : sub_instr) +
                          " " + elem_ty + " " + limag + ", " + rimag);
    } else if (op == BinaryOp::Mul) {
      const std::string ac = fresh_tmp(ctx);
      const std::string bd = fresh_tmp(ctx);
      const std::string ad = fresh_tmp(ctx);
      const std::string bc = fresh_tmp(ctx);
      emit_instr(ctx, ac + " = " + std::string(mul_instr) + " " + elem_ty + " " + lreal + ", " + rreal);
      emit_instr(ctx, bd + " = " + std::string(mul_instr) + " " + elem_ty + " " + limag + ", " + rimag);
      emit_instr(ctx, ad + " = " + std::string(mul_instr) + " " + elem_ty + " " + lreal + ", " + rimag);
      emit_instr(ctx, bc + " = " + std::string(mul_instr) + " " + elem_ty + " " + limag + ", " + rreal);
      out_real = fresh_tmp(ctx);
      emit_instr(ctx, out_real + " = " + std::string(sub_instr) + " " + elem_ty + " " + ac + ", " + bd);
      out_imag = fresh_tmp(ctx);
      emit_instr(ctx, out_imag + " = " + std::string(add_instr) + " " + elem_ty + " " + ad + ", " + bc);
    } else {
      const std::string ac = fresh_tmp(ctx);
      const std::string bd = fresh_tmp(ctx);
      const std::string bc = fresh_tmp(ctx);
      const std::string ad = fresh_tmp(ctx);
      const std::string cc = fresh_tmp(ctx);
      const std::string dd = fresh_tmp(ctx);
      const std::string denom = fresh_tmp(ctx);
      emit_instr(ctx, ac + " = " + std::string(mul_instr) + " " + elem_ty + " " + lreal + ", " + rreal);
      emit_instr(ctx, bd + " = " + std::string(mul_instr) + " " + elem_ty + " " + limag + ", " + rimag);
      emit_instr(ctx, bc + " = " + std::string(mul_instr) + " " + elem_ty + " " + limag + ", " + rreal);
      emit_instr(ctx, ad + " = " + std::string(mul_instr) + " " + elem_ty + " " + lreal + ", " + rimag);
      emit_instr(ctx, cc + " = " + std::string(mul_instr) + " " + elem_ty + " " + rreal + ", " + rreal);
      emit_instr(ctx, dd + " = " + std::string(mul_instr) + " " + elem_ty + " " + rimag + ", " + rimag);
      emit_instr(ctx, denom + " = " + std::string(add_instr) + " " + elem_ty + " " + cc + ", " + dd);
      const std::string real_num = fresh_tmp(ctx);
      const std::string imag_num = fresh_tmp(ctx);
      emit_instr(ctx, real_num + " = " + std::string(add_instr) + " " + elem_ty + " " + ac + ", " + bd);
      emit_instr(ctx, imag_num + " = " + std::string(sub_instr) + " " + elem_ty + " " + bc + ", " + ad);
      out_real = fresh_tmp(ctx);
      out_imag = fresh_tmp(ctx);
      emit_instr(ctx, out_real + " = " + std::string(div_instr) + " " + elem_ty + " " + real_num + ", " + denom);
      emit_instr(ctx, out_imag + " = " + std::string(div_instr) + " " + elem_ty + " " + imag_num + ", " + denom);
    }

    const std::string with_real = fresh_tmp(ctx);
    emit_instr(ctx, with_real + " = insertvalue " + llvm_ty(complex_ts) + " undef, " +
                               elem_ty + " " + out_real + ", 0");
    const std::string out = fresh_tmp(ctx);
    emit_instr(ctx, out + " = insertvalue " + llvm_ty(complex_ts) + " " + with_real + ", " +
                          elem_ty + " " + out_imag + ", 1");
    return llvm_ty(complex_ts) == llvm_ty(res_spec) ? out : coerce(ctx, out, complex_ts, res_spec);
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const StringLiteral& sl, const Expr& /*e*/){
    if (sl.is_wide) {
      // Wide string: emit as global array of i32 (wchar_t) values
      const auto vals = decode_wide_string_values(sl.raw);
      const std::string gname = "@.str" + std::to_string(str_idx_++);
      std::string init = "[";
      for (size_t i = 0; i < vals.size(); ++i) {
        if (i) init += ", ";
        init += "i32 " + std::to_string(vals[i]);
      }
      init += "]";
      {
        lir::LirStringConst sc;
        sc.pool_name = gname;
        sc.raw_bytes = "[" + std::to_string(vals.size()) + " x i32] " + init;
        sc.byte_length = -1;  // sentinel: raw_bytes is pre-formatted type+init
        module_.string_pool.push_back(std::move(sc));
      }
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = getelementptr [" + std::to_string(vals.size()) +
                          " x i32], ptr " + gname + ", i64 0, i64 0");
      return tmp;
    }
    // Strip enclosing quotes from raw, use as bytes
    std::string bytes = sl.raw;
    if (bytes.size() >= 2 && bytes.front() == '"' && bytes.back() == '"') {
      bytes = bytes.substr(1, bytes.size() - 2);
    }
    bytes = decode_c_escaped_bytes(bytes);
    const std::string gname = intern_str(bytes);
    const size_t len = bytes.size() + 1;
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = getelementptr [" + std::to_string(len) +
                        " x i8], ptr " + gname + ", i64 0, i64 0");
    return tmp;
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const DeclRef& r, const Expr& e){
    // Param: SSA value (check before function refs — params shadow function names)
    if (r.param_index && ctx.fn && *r.param_index < ctx.fn->params.size()) {
      // If we already have a spill slot for this param (due to lval use like p++),
      // load from it to get the current (possibly modified) value.
      const auto spill_it = ctx.param_slots.find(*r.param_index + 0x80000000u);
      if (spill_it != ctx.param_slots.end()) {
        const TypeSpec& pts = ctx.fn->params[*r.param_index].type.spec;
        const std::string ty = llvm_ty(pts);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = load " + ty + ", ptr " + spill_it->second);
        return tmp;
      }
      // Otherwise use the SSA param value directly.
      const auto it = ctx.param_slots.find(*r.param_index);
      if (it != ctx.param_slots.end()) return it->second;
    }

    // Local: load
    if (r.local) {
      const auto it = ctx.local_slots.find(r.local->value);
      if (it == ctx.local_slots.end())
        throw std::runtime_error("HirEmitter: local slot not found: " + r.name);
      const TypeSpec ts = ctx.local_types.at(r.local->value);
      const auto vit = ctx.local_is_vla.find(r.local->value);
      if (vit != ctx.local_is_vla.end() && vit->second) {
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = load ptr, ptr " + it->second);
        return tmp;
      }
      if (ts.base == TB_VA_LIST && ts.ptr_level == 0 && ts.array_rank == 0) {
        // Treat va_list as array-like in expressions: it decays to a pointer to
        // the va_list object, so callers like vprintf(fmt, ap) receive ptr.
        return it->second;
      }
      if (ts.array_rank > 0 && !ts.is_ptr_to_array) {
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = getelementptr " + llvm_alloca_ty(ts) +
                            ", ptr " + it->second + ", i64 0, i64 0");
        return tmp;
      }
      const std::string ty = llvm_ty(ts);
      if (ty == "void") return "0";
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = load " + ty + ", ptr " + it->second);
      return tmp;
    }

    // Global: load
    if (r.global) {
      size_t gv_idx = r.global->value;
      const auto& gv0 = mod_.globals[gv_idx];
      if (const GlobalVar* best = select_global_object(gv0.name)) gv_idx = best->id.value;
      const auto& gv = mod_.globals[gv_idx];
      if (gv.type.spec.array_rank > 0 && !gv.type.spec.is_ptr_to_array) {
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = getelementptr " + llvm_alloca_ty(gv.type.spec) +
                            ", ptr " + llvm_global_sym(gv.name) + ", i64 0, i64 0");
        return tmp;
      }
      const std::string ty = llvm_ty(gv.type.spec);
      if (ty == "void") return "0";
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = load " + ty + ", ptr " + llvm_global_sym(gv.name));
      return tmp;
    }

    // Function reference: return as ptr value (after checking locals/params)
    if (mod_.fn_index.count(r.name)) return llvm_global_sym(r.name);

    if ((r.name == "__func__" || r.name == "__FUNCTION__" || r.name == "__PRETTY_FUNCTION__") &&
        ctx.fn) {
      const std::string gname = intern_str(ctx.fn->name);
      const size_t len = ctx.fn->name.size() + 1;
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = getelementptr [" + std::to_string(len) +
                          " x i8], ptr " + gname + ", i64 0, i64 0");
      return tmp;
    }

    // Unresolved: load from external global
    const TypeSpec ets = resolve_expr_type(ctx, e);
    if (!has_concrete_type(ets)) return "0";
    const std::string ty = llvm_ty(ets);
    if (ty == "void") return "0";
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = load " + ty + ", ptr @" + r.name);
    return tmp;
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const UnaryExpr& u, const Expr& e){
    // Short-circuit ops that use emit_lval (not emit_rval_id) to avoid
    // double-evaluating the operand and duplicating side effects (e.g. cnt++).
    if (u.op == UnaryOp::AddrOf) {
      TypeSpec pts{};
      return emit_lval(ctx, u.operand, pts);
    }
    if (u.op == UnaryOp::PreInc || u.op == UnaryOp::PreDec ||
        u.op == UnaryOp::PostInc || u.op == UnaryOp::PostDec) {
      // Fall through to the switch below but skip the rval evaluation
      // that would duplicate side effects.
    } else {
      // Only evaluate operand as rval for ops that actually need it.
    }

    TypeSpec et = resolve_expr_type(ctx, e);
    TypeSpec op_ts{};
    // For pre/post inc/dec, the operand is an lvalue — avoid evaluating as rval
    // which would duplicate any side effects in the operand expression.
    const bool skip_rval = (u.op == UnaryOp::PreInc || u.op == UnaryOp::PreDec ||
                            u.op == UnaryOp::PostInc || u.op == UnaryOp::PostDec);
    const std::string val = skip_rval ? "" : emit_rval_id(ctx, u.operand, op_ts);
    if (skip_rval) op_ts = resolve_expr_type(ctx, u.operand);
    if (!has_concrete_type(et)) et = op_ts;
    const std::string ty = llvm_ty(et);
    const std::string op_ty = llvm_ty(op_ts);

    switch (u.op) {
      case UnaryOp::Plus: {
        // Integer promotion for small types
        if (op_ty != ty && !is_vector_value(op_ts) && !is_float_base(op_ts.base)) {
          const std::string ext = fresh_tmp(ctx);
          const bool is_signed = is_signed_int(op_ts.base);
          emit_instr(ctx, ext + " = " + (is_signed ? "sext" : "zext") +
                              " " + op_ty + " " + val + " to " + ty);
          return ext;
        }
        return val;
      }

      case UnaryOp::Minus: {
        // Integer promotion for small types
        std::string promoted_val = val;
        std::string promoted_ty = op_ty;
        if (op_ty != ty && !is_vector_value(op_ts) && !is_float_base(op_ts.base)) {
          const std::string ext = fresh_tmp(ctx);
          const bool is_signed = is_signed_int(op_ts.base);
          emit_instr(ctx, ext + " = " + (is_signed ? "sext" : "zext") +
                              " " + op_ty + " " + val + " to " + ty);
          promoted_val = ext;
          promoted_ty = ty;
        }
        const std::string tmp = fresh_tmp(ctx);
        if (is_vector_value(op_ts)) {
          emit_instr(ctx, tmp + " = sub " + ty + " zeroinitializer, " + val);
        } else if (is_float_base(op_ts.base)) {
          emit_instr(ctx, tmp + " = fneg " + ty + " " + val);
        } else {
          emit_instr(ctx, tmp + " = sub " + promoted_ty + " 0, " + promoted_val);
        }
        return tmp;
      }

      case UnaryOp::Not: {
        const std::string cmp = to_bool(ctx, val, op_ts);
        // Invert i1
        const std::string inv = fresh_tmp(ctx);
        emit_instr(ctx, inv + " = xor i1 " + cmp + ", true");
        if (ty == "i1") return inv;
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + inv + " to " + ty);
        return tmp;
      }

      case UnaryOp::BitNot: {
        if (is_complex_base(op_ts.base)) {
          const TypeSpec elem_ts = complex_component_ts(op_ts.base);
          const std::string real_v = fresh_tmp(ctx);
          emit_instr(ctx, real_v + " = extractvalue " + op_ty + " " + val + ", 0");
          const std::string imag_v0 = fresh_tmp(ctx);
          emit_instr(ctx, imag_v0 + " = extractvalue " + op_ty + " " + val + ", 1");
          const std::string imag_v = fresh_tmp(ctx);
          if (is_float_base(elem_ts.base)) {
            emit_instr(ctx, imag_v + " = fneg " + llvm_ty(elem_ts) + " " + imag_v0);
          } else {
            emit_instr(ctx, imag_v + " = sub " + llvm_ty(elem_ts) + " 0, " + imag_v0);
          }
          const std::string with_real = fresh_tmp(ctx);
          emit_instr(ctx, with_real + " = insertvalue " + op_ty + " undef, " +
                                 llvm_ty(elem_ts) + " " + real_v + ", 0");
          const std::string out = fresh_tmp(ctx);
          emit_instr(ctx, out + " = insertvalue " + op_ty + " " + with_real + ", " +
                                llvm_ty(elem_ts) + " " + imag_v + ", 1");
          return out;
        }
        const std::string tmp = fresh_tmp(ctx);
        if (is_vector_value(op_ts)) {
          TypeSpec elem_ts = op_ts;
          elem_ts.is_vector = false;
          elem_ts.vector_lanes = 0;
          elem_ts.vector_bytes = 0;
          std::string mask = "<";
          for (long long i = 0; i < op_ts.vector_lanes; ++i) {
            if (i) mask += ", ";
            mask += llvm_ty(elem_ts) + " -1";
          }
          mask += ">";
          emit_instr(ctx, tmp + " = xor " + ty + " " + val + ", " + mask);
        } else {
          // Integer promotion: extend small types (i8, i16) to i32 before ~
          std::string promoted_val = val;
          std::string promoted_ty = op_ty;
          if (op_ty != ty && !is_vector_value(op_ts)) {
            const std::string ext = fresh_tmp(ctx);
            const bool is_signed = is_signed_int(op_ts.base);
            emit_instr(ctx, ext + " = " + (is_signed ? "sext" : "zext") +
                                " " + op_ty + " " + val + " to " + ty);
            promoted_val = ext;
            promoted_ty = ty;
          }
          emit_instr(ctx, tmp + " = xor " + promoted_ty + " " + promoted_val + ", -1");
        }
        return tmp;
      }

      case UnaryOp::AddrOf: {
        // Handled above before emit_rval_id to avoid side-effect duplication.
        __builtin_unreachable();
      }

      case UnaryOp::Deref: {
        TypeSpec load_ts = op_ts;
        if (load_ts.ptr_level > 0) {
          load_ts.ptr_level -= 1;
        } else if (load_ts.array_rank > 0) {
          // Decayed array: the value is already a pointer to the first element.
          // Dereference it by loading the element type.
          load_ts.array_rank--;
          load_ts.array_size = -1;
        }
        // Dereferencing a pointer-to-array yields an array lvalue. In rvalue
        // position it decays back to the same base address; loading a ptr here
        // would incorrectly treat the first elements as an address.
        if (load_ts.array_rank > 0) return val;
        // In C, dereferencing a function pointer (*fp) yields the function,
        // which immediately decays back to a function pointer — identity op.
        if (load_ts.is_fn_ptr && load_ts.ptr_level == 0) return val;
        const std::string load_ty = llvm_ty(load_ts);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = load " + load_ty + ", ptr " + val);
        return tmp;
      }

      case UnaryOp::PreInc:
      case UnaryOp::PreDec: {
        // Check for bitfield operand
        const Expr& operand_e = get_expr(u.operand);
        if (const auto* m = std::get_if<MemberExpr>(&operand_e.payload)) {
          TypeSpec lhs_ts{};
          BitfieldAccess bf;
          const std::string bf_ptr = emit_member_lval(ctx, *m, lhs_ts, &bf);
          if (bf.is_bitfield()) {
            const int pbits = bitfield_promoted_bits(bf);
            const std::string pty = "i" + std::to_string(pbits);
            const std::string loaded = emit_bitfield_load(ctx, bf_ptr, bf);
            const std::string delta = (u.op == UnaryOp::PreInc) ? "1" : "-1";
            const std::string result = fresh_tmp(ctx);
            emit_instr(ctx, result + " = add " + pty + " " + loaded + ", " + delta);
            TypeSpec promoted_ts = bitfield_promoted_ts(bf);
            emit_bitfield_store(ctx, bf_ptr, bf, result, promoted_ts);
            // Re-read to get value masked to bitfield width (C semantics)
            return emit_bitfield_load(ctx, bf_ptr, bf);
          }
        }
        TypeSpec pts{};
        const std::string ptr = emit_lval(ctx, u.operand, pts);
        const std::string pty = llvm_ty(pts);
        const std::string loaded = fresh_tmp(ctx);
        emit_instr(ctx, loaded + " = load " + pty + ", ptr " + ptr);
        const std::string result = fresh_tmp(ctx);
        if (pty == "ptr") {
          const std::string delta = (u.op == UnaryOp::PreInc) ? "1" : "-1";
          TypeSpec elem_ts = pts;
          if (elem_ts.ptr_level > 0) elem_ts.ptr_level -= 1;
          else elem_ts.base = TB_CHAR;
          std::string gep_ety1;
          if (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0 && elem_ts.array_rank == 0)
            gep_ety1 = "i8";
          else if (elem_ts.array_rank > 0 && elem_ts.ptr_level == 0)
            gep_ety1 = llvm_alloca_ty(elem_ts);
          else
            gep_ety1 = llvm_ty(elem_ts);
          emit_instr(ctx, result + " = getelementptr " + gep_ety1 +
                              ", ptr " + loaded + ", i64 " + delta);
        } else if (is_float_base(pts.base)) {
          const std::string delta = (u.op == UnaryOp::PreInc) ? "1.0" : "-1.0";
          emit_instr(ctx, result + " = fadd " + pty + " " + loaded + ", " + delta);
        } else {
          const std::string delta = (u.op == UnaryOp::PreInc) ? "1" : "-1";
          emit_instr(ctx, result + " = add " + pty + " " + loaded + ", " + delta);
        }
        emit_instr(ctx, "store " + pty + " " + result + ", ptr " + ptr);
        return result;
      }

      case UnaryOp::PostInc:
      case UnaryOp::PostDec: {
        // Check for bitfield operand
        const Expr& post_operand_e = get_expr(u.operand);
        if (const auto* m = std::get_if<MemberExpr>(&post_operand_e.payload)) {
          TypeSpec lhs_ts{};
          BitfieldAccess bf;
          const std::string bf_ptr = emit_member_lval(ctx, *m, lhs_ts, &bf);
          if (bf.is_bitfield()) {
            const int pbits = bitfield_promoted_bits(bf);
            const std::string pty = "i" + std::to_string(pbits);
            const std::string old_val = emit_bitfield_load(ctx, bf_ptr, bf);
            const std::string delta = (u.op == UnaryOp::PostInc) ? "1" : "-1";
            const std::string new_val = fresh_tmp(ctx);
            emit_instr(ctx, new_val + " = add " + pty + " " + old_val + ", " + delta);
            TypeSpec promoted_ts = bitfield_promoted_ts(bf);
            emit_bitfield_store(ctx, bf_ptr, bf, new_val, promoted_ts);
            return old_val;  // post: return old value
          }
        }
        TypeSpec pts{};
        const std::string ptr = emit_lval(ctx, u.operand, pts);
        const std::string pty = llvm_ty(pts);
        const std::string loaded = fresh_tmp(ctx);
        emit_instr(ctx, loaded + " = load " + pty + ", ptr " + ptr);
        const std::string result = fresh_tmp(ctx);
        if (pty == "ptr") {
          const std::string delta = (u.op == UnaryOp::PostInc) ? "1" : "-1";
          TypeSpec elem_ts = pts;
          if (elem_ts.ptr_level > 0) elem_ts.ptr_level -= 1;
          else elem_ts.base = TB_CHAR;
          std::string gep_ety2;
          if (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0 && elem_ts.array_rank == 0)
            gep_ety2 = "i8";
          else if (elem_ts.array_rank > 0 && elem_ts.ptr_level == 0)
            gep_ety2 = llvm_alloca_ty(elem_ts);
          else
            gep_ety2 = llvm_ty(elem_ts);
          emit_instr(ctx, result + " = getelementptr " + gep_ety2 +
                              ", ptr " + loaded + ", i64 " + delta);
        } else if (is_float_base(pts.base)) {
          const std::string delta = (u.op == UnaryOp::PostInc) ? "1.0" : "-1.0";
          emit_instr(ctx, result + " = fadd " + pty + " " + loaded + ", " + delta);
        } else {
          const std::string delta = (u.op == UnaryOp::PostInc) ? "1" : "-1";
          emit_instr(ctx, result + " = add " + pty + " " + loaded + ", " + delta);
        }
        emit_instr(ctx, "store " + pty + " " + result + ", ptr " + ptr);
        return loaded;  // post: return old value
      }

      case UnaryOp::RealPart:
      case UnaryOp::ImagPart: {
        try {
          TypeSpec complex_ts{};
          const std::string complex_ptr = emit_lval(ctx, u.operand, complex_ts);
          if (!is_complex_base(complex_ts.base)) throw std::runtime_error("non-complex");
          const TypeSpec part_ts = complex_component_ts(complex_ts.base);
          const std::string part_ptr = fresh_tmp(ctx);
          emit_instr(ctx, part_ptr + " = getelementptr " + llvm_alloca_ty(complex_ts) +
                                 ", ptr " + complex_ptr + ", i32 0, i32 " +
                                 std::to_string(u.op == UnaryOp::ImagPart ? 1 : 0));
          const std::string tmp = fresh_tmp(ctx);
          emit_instr(ctx, tmp + " = load " + llvm_ty(part_ts) + ", ptr " + part_ptr);
          return tmp;
        } catch (const std::runtime_error&) {
        }
        if (!is_complex_base(op_ts.base)) return "0";
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = extractvalue " + llvm_ty(op_ts) + " " + val + ", " +
                            std::to_string(u.op == UnaryOp::ImagPart ? 1 : 0));
        return tmp;
      }
    }
    return "0";
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const BinaryExpr& b, const Expr& e){
    // Comma: eval both, return rhs
    if (b.op == BinaryOp::Comma) {
      TypeSpec lts{};
      emit_rval_id(ctx, b.lhs, lts);
      TypeSpec rts{};
      return emit_rval_id(ctx, b.rhs, rts);
    }
    // Short-circuit logical
    if (b.op == BinaryOp::LAnd || b.op == BinaryOp::LOr) {
      return emit_logical(ctx, b, e);
    }

    TypeSpec lts{};
    std::string lv = emit_rval_id(ctx, b.lhs, lts);
    TypeSpec rts{};
    std::string rv = emit_rval_id(ctx, b.rhs, rts);
    TypeSpec res_spec = (e.type.spec.base != TB_VOID || e.type.spec.ptr_level > 0)
                            ? e.type.spec
                            : lts;

    if ((b.op == BinaryOp::Add || b.op == BinaryOp::Sub) &&
        llvm_ty(lts) == "ptr" && llvm_ty(rts) != "ptr") {
      TypeSpec i64_ts{};
      i64_ts.base = TB_LONGLONG;
      std::string idx = coerce(ctx, rv, rts, i64_ts);
      if (b.op == BinaryOp::Sub) {
        const std::string neg = fresh_tmp(ctx);
        emit_instr(ctx, neg + " = sub i64 0, " + idx);
        idx = neg;
      }
      TypeSpec elem_ts = lts;
      if (elem_ts.ptr_level > 0) elem_ts.ptr_level -= 1;
      else if (elem_ts.array_rank > 0 && !elem_ts.is_ptr_to_array) elem_ts = drop_one_array_dim(elem_ts);
      else {
        elem_ts = {};
        elem_ts.base = TB_CHAR;
      }
      std::string gep_elem_ty;
      if (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0 && elem_ts.array_rank == 0)
        gep_elem_ty = "i8";
      else if (elem_ts.array_rank > 0 && elem_ts.ptr_level == 0)
        gep_elem_ty = llvm_alloca_ty(elem_ts);
      else
        gep_elem_ty = llvm_ty(elem_ts);
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = getelementptr " + gep_elem_ty +
                          ", ptr " + lv + ", i64 " + idx);
      return tmp;
    }

    if ((b.op == BinaryOp::Eq || b.op == BinaryOp::Ne) &&
        (is_complex_base(lts.base) || is_complex_base(rts.base))) {
      TypeSpec cmp_lts = lts;
      TypeSpec cmp_rts = rts;
      std::string cmp_lv = lv;
      std::string cmp_rv = rv;
      const TypeSpec elem_ts = is_complex_base(cmp_lts.base)
                                   ? complex_component_ts(cmp_lts.base)
                                   : complex_component_ts(cmp_rts.base);
      const TypeSpec complex_ts = is_complex_base(cmp_lts.base) ? cmp_lts : cmp_rts;
      auto lift_scalar_to_complex = [&](const std::string& scalar,
                                        const TypeSpec& scalar_ts,
                                        bool scalar_is_lhs) -> std::string {
        std::string real_v = coerce(ctx, scalar, scalar_ts, elem_ts);
        const std::string imag_v = emit_const_int_like(0, elem_ts);
        const std::string with_real = fresh_tmp(ctx);
        emit_instr(ctx, with_real + " = insertvalue " + llvm_ty(complex_ts) + " undef, " +
                                   llvm_ty(elem_ts) + " " + real_v + ", 0");
        const std::string out = fresh_tmp(ctx);
        emit_instr(ctx, out + " = insertvalue " + llvm_ty(complex_ts) + " " + with_real + ", " +
                            llvm_ty(elem_ts) + " " + imag_v + ", 1");
        (void)scalar_is_lhs;
        return out;
      };
      if (!is_complex_base(cmp_lts.base)) {
        cmp_lv = lift_scalar_to_complex(cmp_lv, cmp_lts, true);
        cmp_lts = complex_ts;
      } else if (llvm_ty(cmp_lts) != llvm_ty(complex_ts)) {
        cmp_lv = coerce(ctx, cmp_lv, cmp_lts, complex_ts);
        cmp_lts = complex_ts;
      }
      if (!is_complex_base(cmp_rts.base)) {
        cmp_rv = lift_scalar_to_complex(cmp_rv, cmp_rts, false);
        cmp_rts = complex_ts;
      } else if (llvm_ty(cmp_rts) != llvm_ty(complex_ts)) {
        cmp_rv = coerce(ctx, cmp_rv, cmp_rts, complex_ts);
        cmp_rts = complex_ts;
      }
      const std::string lreal = fresh_tmp(ctx);
      emit_instr(ctx, lreal + " = extractvalue " + llvm_ty(cmp_lts) + " " + cmp_lv + ", 0");
      const std::string limag = fresh_tmp(ctx);
      emit_instr(ctx, limag + " = extractvalue " + llvm_ty(cmp_lts) + " " + cmp_lv + ", 1");
      const std::string rreal0 = fresh_tmp(ctx);
      emit_instr(ctx, rreal0 + " = extractvalue " + llvm_ty(cmp_rts) + " " + cmp_rv + ", 0");
      const std::string rimag0 = fresh_tmp(ctx);
      emit_instr(ctx, rimag0 + " = extractvalue " + llvm_ty(cmp_rts) + " " + cmp_rv + ", 1");
      const std::string rreal = coerce(ctx, rreal0, elem_ts, elem_ts);
      const std::string rimag = coerce(ctx, rimag0, elem_ts, elem_ts);
      const std::string creal = fresh_tmp(ctx);
      const std::string cimag = fresh_tmp(ctx);
      if (is_float_base(elem_ts.base)) {
        emit_instr(ctx, creal + " = fcmp oeq " + llvm_ty(elem_ts) + " " + lreal + ", " + rreal);
        emit_instr(ctx, cimag + " = fcmp oeq " + llvm_ty(elem_ts) + " " + limag + ", " + rimag);
      } else {
        emit_instr(ctx, creal + " = icmp eq " + llvm_ty(elem_ts) + " " + lreal + ", " + rreal);
        emit_instr(ctx, cimag + " = icmp eq " + llvm_ty(elem_ts) + " " + limag + ", " + rimag);
      }
      const std::string both = fresh_tmp(ctx);
      emit_instr(ctx, both + " = and i1 " + creal + ", " + cimag);
      const std::string out = fresh_tmp(ctx);
      if (b.op == BinaryOp::Eq) {
        emit_instr(ctx, out + " = zext i1 " + both + " to i32");
      } else {
        const std::string inv = fresh_tmp(ctx);
        emit_instr(ctx, inv + " = xor i1 " + both + ", true");
        emit_instr(ctx, out + " = zext i1 " + inv + " to i32");
      }
      return out;
    }

    if ((b.op == BinaryOp::Add || b.op == BinaryOp::Sub ||
         b.op == BinaryOp::Mul || b.op == BinaryOp::Div) &&
        (is_complex_base(lts.base) || is_complex_base(rts.base))) {
      return emit_complex_binary_arith(ctx, b.op, lv, lts, rv, rts, res_spec);
    }

    if (is_vector_value(res_spec)) {
      // Scalar-to-vector splatting: when one operand is scalar and the other
      // is a vector, splat the scalar to match the vector type before the op.
      auto emit_splat_vec = [&](const std::string& scalar, const TypeSpec& scalar_ts,
                            const TypeSpec& vec_ts) -> std::string {
        TypeSpec elem_ts = vec_ts;
        elem_ts.is_vector = false;
        elem_ts.vector_lanes = 0;
        elem_ts.vector_bytes = 0;
        std::string coerced = coerce(ctx, scalar, scalar_ts, elem_ts);
        const std::string elem_ty = llvm_ty(elem_ts);
        const std::string vec_ty_s = llvm_vector_ty(vec_ts);
        const int lanes = vec_ts.vector_lanes;
        const std::string ins = fresh_tmp(ctx);
        emit_instr(ctx, ins + " = insertelement " + vec_ty_s + " poison, " +
                            elem_ty + " " + coerced + ", i64 0");
        const std::string shuf = fresh_tmp(ctx);
        emit_instr(ctx, shuf + " = shufflevector " + vec_ty_s + " " + ins +
                            ", " + vec_ty_s + " poison, <" + std::to_string(lanes) +
                            " x i32> zeroinitializer");
        return shuf;
      };
      if (is_vector_value(lts) && !is_vector_value(rts) && rts.ptr_level == 0) {
        rv = emit_splat_vec(rv, rts, lts);
        rts = lts;
      } else if (is_vector_value(rts) && !is_vector_value(lts) && lts.ptr_level == 0) {
        lv = emit_splat_vec(lv, lts, rts);
        lts = rts;
      }
      const std::string tmp = fresh_tmp(ctx);
      const std::string vec_ty = llvm_ty(res_spec);
      const bool vec_float = is_float_base(res_spec.base);
      switch (b.op) {
        case BinaryOp::Add:
          emit_instr(ctx, tmp + " = " + (vec_float ? "fadd " : "add ") + vec_ty + " " + lv + ", " + rv);
          return tmp;
        case BinaryOp::Sub:
          emit_instr(ctx, tmp + " = " + (vec_float ? "fsub " : "sub ") + vec_ty + " " + lv + ", " + rv);
          return tmp;
        case BinaryOp::Mul:
          emit_instr(ctx, tmp + " = " + (vec_float ? "fmul " : "mul ") + vec_ty + " " + lv + ", " + rv);
          return tmp;
        case BinaryOp::Div:
          if (vec_float)
            emit_instr(ctx, tmp + " = fdiv " + vec_ty + " " + lv + ", " + rv);
          else
            emit_instr(ctx, tmp + " = " +
                                std::string(is_signed_int(res_spec.base) ? "sdiv " : "udiv ") +
                                vec_ty + " " + lv + ", " + rv);
          return tmp;
        case BinaryOp::Mod:
          if (vec_float)
            emit_instr(ctx, tmp + " = frem " + vec_ty + " " + lv + ", " + rv);
          else
            emit_instr(ctx, tmp + " = " +
                                std::string(is_signed_int(res_spec.base) ? "srem " : "urem ") +
                                vec_ty + " " + lv + ", " + rv);
          return tmp;
        case BinaryOp::BitAnd:
          emit_instr(ctx, tmp + " = and " + vec_ty + " " + lv + ", " + rv);
          return tmp;
        case BinaryOp::BitOr:
          emit_instr(ctx, tmp + " = or " + vec_ty + " " + lv + ", " + rv);
          return tmp;
        case BinaryOp::BitXor:
          emit_instr(ctx, tmp + " = xor " + vec_ty + " " + lv + ", " + rv);
          return tmp;
        default:
          break;
      }
    }

    // Apply C usual arithmetic conversions (UAC) for integer operands.
    // This promotes both operands to at least int, then to the common type.
    const bool l_is_int = is_any_int(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0 &&
                          !is_vector_value(lts);
    const bool r_is_int = is_any_int(rts.base) && rts.ptr_level == 0 && rts.array_rank == 0 &&
                          !is_vector_value(rts);
    const bool l_is_flt = is_float_base(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0 &&
                          !is_vector_value(lts);
    const bool r_is_flt = is_float_base(rts.base) && rts.ptr_level == 0 && rts.array_rank == 0 &&
                          !is_vector_value(rts);
    if (l_is_int && r_is_int) {
      // For shift operations, the result type is the promoted LHS only;
      // the RHS (shift amount) doesn't participate in usual arithmetic conversions.
      const bool is_shift = (b.op == BinaryOp::Shl || b.op == BinaryOp::Shr);
      if (is_shift) {
        TypeBase lp = integer_promote(lts.base);
        TypeSpec lp_ts{}; lp_ts.base = lp;
        if (lts.base != lp) lv = coerce(ctx, lv, lts, lp_ts);
        lts = lp_ts;
        // Coerce RHS to match LHS width for LLVM (shift needs same-width operands)
        if (llvm_ty(rts) != llvm_ty(lts)) rv = coerce(ctx, rv, rts, lts);
        rts = lts;
      } else {
        TypeBase common = usual_arith_conv(lts.base, rts.base);
        TypeSpec common_ts{};
        common_ts.base = common;
        if (lts.base != common) lv = coerce(ctx, lv, lts, common_ts);
        lts = common_ts;
        if (rts.base != common) rv = coerce(ctx, rv, rts, common_ts);
        rts = common_ts;
      }
    } else if (l_is_int && r_is_flt) {
      lv = coerce(ctx, lv, lts, rts);
      lts = rts;
    } else if (l_is_flt && r_is_int) {
      rv = coerce(ctx, rv, rts, lts);
      rts = lts;
    }

    // After UAC, the result type should be at least as wide as the promoted
    // operand type.  The stored res_spec may reflect the pre-promotion type
    // from sema, so override it with the promoted lts for int arithmetic.
    if (l_is_int && r_is_int && is_any_int(res_spec.base) &&
        res_spec.ptr_level == 0 && res_spec.array_rank == 0) {
      res_spec = lts;
    }

    // Scalar-to-vector splatting: when one operand is scalar and the other
    // is a vector, splat the scalar to match the vector type.
    auto emit_splat = [&](const std::string& scalar, const TypeSpec& scalar_ts,
                          const TypeSpec& vec_ts) -> std::string {
      // First coerce the scalar to the vector element type
      TypeSpec elem_ts = vec_ts;
      elem_ts.is_vector = false;
      elem_ts.vector_lanes = 0;
      elem_ts.vector_bytes = 0;
      std::string coerced = coerce(ctx, scalar, scalar_ts, elem_ts);
      const std::string elem_ty = llvm_ty(elem_ts);
      const std::string vec_ty = llvm_vector_ty(vec_ts);
      const int lanes = vec_ts.vector_lanes;
      // insertelement + shufflevector to broadcast
      const std::string ins = fresh_tmp(ctx);
      emit_instr(ctx, ins + " = insertelement " + vec_ty + " poison, " +
                          elem_ty + " " + coerced + ", i64 0");
      const std::string shuf = fresh_tmp(ctx);
      emit_instr(ctx, shuf + " = shufflevector " + vec_ty + " " + ins +
                          ", " + vec_ty + " poison, <" + std::to_string(lanes) +
                          " x i32> zeroinitializer");
      return shuf;
    };
    if (is_vector_value(lts) && !is_vector_value(rts) && rts.ptr_level == 0) {
      rv = emit_splat(rv, rts, lts);
      rts = lts;
      res_spec = lts;  // update result type to vector
    } else if (is_vector_value(rts) && !is_vector_value(lts) && lts.ptr_level == 0) {
      lv = emit_splat(lv, lts, rts);
      lts = rts;
      res_spec = rts;  // update result type to vector
    }

    // If result type is unannotated (void), use the operand type
    const std::string res_ty = llvm_ty(res_spec);
    const std::string op_ty  = llvm_ty(lts);
    // lf/ls must be false when the LLVM type is ptr (even if base is double/float).
    const bool lf  = is_float_base(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0;
    const bool ls  = is_signed_int(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0;
    const bool arith_ls = ls;

    // Pointer arithmetic: ptr +/- int and int + ptr.
    if (b.op == BinaryOp::Add || b.op == BinaryOp::Sub) {
      auto emit_ptr_gep = [&](const std::string& base_ptr, const TypeSpec& base_ts,
                              const std::string& idx_val, const TypeSpec& idx_ts,
                              bool negate_idx) -> std::string {
        TypeSpec i64_ts{};
        i64_ts.base = TB_LONGLONG;
        std::string idx = coerce(ctx, idx_val, idx_ts, i64_ts);
        if (negate_idx) {
          const std::string neg = fresh_tmp(ctx);
          emit_instr(ctx, neg + " = sub i64 0, " + idx);
          idx = neg;
        }
        TypeSpec elem_ts = base_ts;
        if (elem_ts.ptr_level > 0) {
          // T* +/- n: stride by sizeof(T).  For pointer-to-array keep the
          // array object type after consuming one pointer layer.
          elem_ts.ptr_level -= 1;
        } else if (elem_ts.array_rank > 0 && !elem_ts.is_ptr_to_array) {
          // Array expression in arithmetic decays to pointer-to-first-element.
          elem_ts = drop_one_array_dim(elem_ts);
        } else {
          elem_ts = {};
          elem_ts.base = TB_CHAR;
        }
        // GCC extension: void* arithmetic uses byte stride (same as char*).
        std::string gep_elem_ty;
        if (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0 && elem_ts.array_rank == 0)
          gep_elem_ty = "i8";
        else if (elem_ts.array_rank > 0 && elem_ts.ptr_level == 0)
          gep_elem_ty = llvm_alloca_ty(elem_ts);
        else
          gep_elem_ty = llvm_ty(elem_ts);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = getelementptr " + gep_elem_ty +
                            ", ptr " + base_ptr + ", i64 " + idx);
        return tmp;
      };

      const bool rhs_is_nonptr_scalar =
          llvm_ty(rts) != "ptr" && !is_float_base(rts.base) && rts.array_rank == 0;
      const bool lhs_is_nonptr_scalar =
          llvm_ty(lts) != "ptr" && !is_float_base(lts.base) && lts.array_rank == 0;
      if (op_ty == "ptr" && rhs_is_nonptr_scalar) {
        return emit_ptr_gep(lv, lts, rv, rts, b.op == BinaryOp::Sub);
      }
      if (llvm_ty(rts) == "ptr" && lhs_is_nonptr_scalar && b.op == BinaryOp::Add) {
        return emit_ptr_gep(rv, rts, lv, lts, false);
      }
      if (b.op == BinaryOp::Sub && op_ty == "ptr" && llvm_ty(rts) == "ptr") {
        const std::string lhs_i = fresh_tmp(ctx);
        const std::string rhs_i = fresh_tmp(ctx);
        emit_instr(ctx, lhs_i + " = ptrtoint ptr " + lv + " to i64");
        emit_instr(ctx, rhs_i + " = ptrtoint ptr " + rv + " to i64");
        const std::string byte_diff = fresh_tmp(ctx);
        emit_instr(ctx, byte_diff + " = sub i64 " + lhs_i + ", " + rhs_i);
        TypeSpec elem_ts = lts;
        if (elem_ts.ptr_level > 0) elem_ts.ptr_level -= 1;
        // GCC extension: void* arithmetic is byte-granular (sizeof(void) == 1)
        const int elem_sz = (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0)
                                ? 1 : std::max(1, sizeof_ts(mod_, elem_ts));
        std::string diff = byte_diff;
        if (elem_sz != 1) {
          const std::string scaled = fresh_tmp(ctx);
          emit_instr(ctx, scaled + " = sdiv i64 " + byte_diff + ", " + std::to_string(elem_sz));
          diff = scaled;
        }
        TypeSpec i64_ts{};
        i64_ts.base = TB_LONGLONG;
        if (res_spec.ptr_level > 0 || res_spec.array_rank > 0 || res_spec.base == TB_VOID) {
          return diff;
        }
        return coerce(ctx, diff, i64_ts, res_spec);
      }
    }

    rv = coerce(ctx, rv, rts, lts);

    // Arithmetic
    static const struct { BinaryOp op; const char* i_s; const char* i_u; const char* f; } arith[] = {
      { BinaryOp::Add, "add",  "add",  "fadd" },
      { BinaryOp::Sub, "sub",  "sub",  "fsub" },
      { BinaryOp::Mul, "mul",  "mul",  "fmul" },
      { BinaryOp::Div, "sdiv", "udiv", "fdiv" },
      { BinaryOp::Mod, "srem", "urem", nullptr },
      { BinaryOp::Shl, "shl",  "shl",  nullptr },
      { BinaryOp::Shr, "ashr", "lshr", nullptr },
      { BinaryOp::BitAnd, "and", "and", nullptr },
      { BinaryOp::BitOr,  "or",  "or",  nullptr },
      { BinaryOp::BitXor, "xor", "xor", nullptr },
    };
    for (const auto& row : arith) {
      if (row.op == b.op) {
        if ((row.op == BinaryOp::Add || row.op == BinaryOp::Sub) && op_ty == "ptr") {
          auto emit_ptr_fallback = [&](const std::string& base_ptr, const TypeSpec& base_ts,
                                       const std::string& idx_val, const TypeSpec& idx_ts,
                                       bool negate_idx) -> std::string {
            TypeSpec i64_ts{};
            i64_ts.base = TB_LONGLONG;
            std::string idx = coerce(ctx, idx_val, idx_ts, i64_ts);
            if (negate_idx) {
              const std::string neg = fresh_tmp(ctx);
              emit_instr(ctx, neg + " = sub i64 0, " + idx);
              idx = neg;
            }
            TypeSpec elem_ts = base_ts;
            if (elem_ts.ptr_level > 0) elem_ts.ptr_level -= 1;
            else if (elem_ts.array_rank > 0 && !elem_ts.is_ptr_to_array) elem_ts = drop_one_array_dim(elem_ts);
            else {
              elem_ts = {};
              elem_ts.base = TB_CHAR;
            }
            std::string gep_elem_ty;
            if (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0 && elem_ts.array_rank == 0)
              gep_elem_ty = "i8";
            else if (elem_ts.array_rank > 0 && elem_ts.ptr_level == 0)
              gep_elem_ty = llvm_alloca_ty(elem_ts);
            else
              gep_elem_ty = llvm_ty(elem_ts);
            const std::string tmp = fresh_tmp(ctx);
            emit_instr(ctx, tmp + " = getelementptr " + gep_elem_ty +
                                ", ptr " + base_ptr + ", i64 " + idx);
            return tmp;
          };
          return emit_ptr_fallback(lv, lts, rv, rts, row.op == BinaryOp::Sub);
        }
        const char* instr = nullptr;
        if (lf) {
          instr = row.f;
        } else if (row.op == BinaryOp::Shr) {
          instr = ls ? row.i_s : row.i_u;
        } else {
          instr = arith_ls ? row.i_s : row.i_u;
        }
        if (!instr) break;
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = " + instr + " " + op_ty + " " + lv + ", " + rv);
        return coerce(ctx, tmp, lts, res_spec);
      }
    }

    // Comparison
    static const struct { BinaryOp op; const char* is; const char* iu; const char* f; } cmp[] = {
      { BinaryOp::Lt, "slt", "ult", "olt" },
      { BinaryOp::Le, "sle", "ule", "ole" },
      { BinaryOp::Gt, "sgt", "ugt", "ogt" },
      { BinaryOp::Ge, "sge", "uge", "oge" },
      { BinaryOp::Eq, "eq",  "eq",  "oeq" },
      { BinaryOp::Ne, "ne",  "ne",  "une" },
    };
    for (const auto& row : cmp) {
      if (row.op == b.op) {
        // C comparisons always yield int. Always zext i1 → i32 so the returned
        // value matches the TB_INT TypeSpec reported by resolve_payload_type.
        const std::string cmp_tmp = fresh_tmp(ctx);
        if (lf) {
          emit_instr(ctx, cmp_tmp + " = fcmp " + row.f + " " + op_ty + " " + lv + ", " + rv);
        } else {
          // After UAC, both operands are at the common type.
          // Use the common type's signedness for the comparison predicate.
          const char* pred = ls ? row.is : row.iu;
          emit_instr(ctx, cmp_tmp + " = icmp " + pred + " " + op_ty + " " + lv + ", " + rv);
        }
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + cmp_tmp + " to i32");
        return tmp;
      }
    }

    throw std::runtime_error("HirEmitter: unhandled binary op");
  }

std::string HirEmitter::emit_logical(FnCtx& ctx, const BinaryExpr& b, const Expr& e){
    TypeSpec res_spec = resolve_expr_type(ctx, e);
    if (!has_concrete_type(res_spec)) res_spec.base = TB_INT;
    const std::string res_ty = llvm_ty(res_spec);
    TypeSpec lts{};
    const std::string lv = emit_rval_id(ctx, b.lhs, lts);
    const std::string lc = to_bool(ctx, lv, lts);

    const std::string rhs_lbl  = fresh_lbl(ctx, "logic.rhs.");
    const std::string skip_lbl = fresh_lbl(ctx, "logic.skip.");
    const std::string rhs_end_lbl = fresh_lbl(ctx, "logic.rhs.end.");
    const std::string end_lbl  = fresh_lbl(ctx, "logic.end.");

    if (b.op == BinaryOp::LAnd) {
      emit_term(ctx, "br i1 " + lc + ", label %" + rhs_lbl + ", label %" + skip_lbl);
    } else {
      emit_term(ctx, "br i1 " + lc + ", label %" + skip_lbl + ", label %" + rhs_lbl);
    }

    emit_lbl(ctx, rhs_lbl);
    TypeSpec rts{};
    const std::string rv = emit_rval_id(ctx, b.rhs, rts);
    const std::string rc = to_bool(ctx, rv, rts);
    std::string rhs_val;
    if (res_ty == "i1") {
      rhs_val = rc;
    } else if (is_float_base(res_spec.base)) {
      const std::string as_i32 = fresh_tmp(ctx);
      emit_instr(ctx, as_i32 + " = zext i1 " + rc + " to i32");
      rhs_val = fresh_tmp(ctx);
      emit_instr(ctx, rhs_val + " = sitofp i32 " + as_i32 + " to " + res_ty);
    } else {
      rhs_val = fresh_tmp(ctx);
      emit_instr(ctx, rhs_val + " = zext i1 " + rc + " to " + res_ty);
    }
    emit_term(ctx, "br label %" + rhs_end_lbl);

    emit_lbl(ctx, rhs_end_lbl);
    emit_term(ctx, "br label %" + end_lbl);

    emit_lbl(ctx, skip_lbl);
    std::string skip_val;
    if (res_ty == "i1") {
      skip_val = (b.op == BinaryOp::LAnd) ? "false" : "true";
    } else if (is_float_base(res_spec.base)) {
      skip_val = (b.op == BinaryOp::LAnd) ? "0.0" : "1.0";
    } else {
      skip_val = (b.op == BinaryOp::LAnd) ? "0" : "1";
    }
    emit_term(ctx, "br label %" + end_lbl);

    emit_lbl(ctx, end_lbl);
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = phi " + res_ty +
                        " [ " + rhs_val + ", %" + rhs_end_lbl + " ]," +
                        " [ " + skip_val + ", %" + skip_lbl + " ]");
    return tmp;
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const AssignExpr& a, const Expr& /*e*/){
    TypeSpec rhs_ts{};
    std::string rhs = emit_rval_id(ctx, a.rhs, rhs_ts);

    TypeSpec lhs_ts{};
    BitfieldAccess bf;
    // Check if LHS is a member expression (potential bitfield)
    const Expr& lhs_e = get_expr(a.lhs);
    std::string lhs_ptr;
    if (const auto* m = std::get_if<MemberExpr>(&lhs_e.payload)) {
      lhs_ptr = emit_member_lval(ctx, *m, lhs_ts, &bf);
    } else {
      lhs_ptr = emit_lval(ctx, a.lhs, lhs_ts);
    }
    const std::string lty = llvm_ty(lhs_ts);

    // Bitfield simple assignment
    if (bf.is_bitfield() && a.op == AssignOp::Set) {
      emit_bitfield_store(ctx, lhs_ptr, bf, rhs, rhs_ts);
      // In C, (bf = val) yields the value of bf after assignment,
      // which is truncated to the bitfield width.
      return emit_bitfield_load(ctx, lhs_ptr, bf);
    }
    // Bitfield compound assignment
    if (bf.is_bitfield() && a.op != AssignOp::Set) {
      const int pbits = bitfield_promoted_bits(bf);
      const std::string promoted_ty = "i" + std::to_string(pbits);
      TypeSpec promoted_ts = bitfield_promoted_ts(bf);
      const std::string loaded = emit_bitfield_load(ctx, lhs_ptr, bf);
      std::string lhs_op = loaded;
      std::string rhs_op = coerce(ctx, rhs, rhs_ts, promoted_ts);
      const bool ls = is_signed_int(promoted_ts.base);
      const char* instr = nullptr;
      static const struct { AssignOp op; const char* is; const char* iu; } tbl[] = {
        {AssignOp::Add, "add", "add"}, {AssignOp::Sub, "sub", "sub"},
        {AssignOp::Mul, "mul", "mul"}, {AssignOp::Div, "sdiv", "udiv"},
        {AssignOp::Mod, "srem", "urem"}, {AssignOp::Shl, "shl", "shl"},
        {AssignOp::Shr, "ashr", "lshr"}, {AssignOp::BitAnd, "and", "and"},
        {AssignOp::BitOr, "or", "or"}, {AssignOp::BitXor, "xor", "xor"},
      };
      for (const auto& r : tbl) {
        if (r.op == a.op) { instr = ls ? r.is : r.iu; break; }
      }
      if (!instr) throw std::runtime_error("HirEmitter: bitfield compound assign: unknown op");
      const std::string result = fresh_tmp(ctx);
      emit_instr(ctx, result + " = " + instr + " " + promoted_ty + " " + lhs_op + ", " + rhs_op);
      emit_bitfield_store(ctx, lhs_ptr, bf, result, promoted_ts);
      return result;
    }

    if (a.op != AssignOp::Set) {
      const std::string loaded = fresh_tmp(ctx);
      emit_instr(ctx, loaded + " = load " + lty + ", ptr " + lhs_ptr);

      if ((a.op == AssignOp::Add || a.op == AssignOp::Sub) && lty == "ptr") {
        TypeSpec i64_ts{};
        i64_ts.base = TB_LONGLONG;
        std::string delta = coerce(ctx, rhs, rhs_ts, i64_ts);
        if (a.op == AssignOp::Sub) {
          const std::string neg = fresh_tmp(ctx);
          emit_instr(ctx, neg + " = sub i64 0, " + delta);
          delta = neg;
        }
        TypeSpec elem_ts = lhs_ts;
        if (elem_ts.ptr_level > 0) {
          elem_ts.ptr_level -= 1;
        } else {
          elem_ts.base = TB_CHAR;
        }
        const std::string gep_ety3 =
            (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0 && elem_ts.array_rank == 0)
                ? "i8" : llvm_ty(elem_ts);
        const std::string result = fresh_tmp(ctx);
        emit_instr(ctx, result + " = getelementptr " + gep_ety3 +
                            ", ptr " + loaded + ", i64 " + delta);
        emit_instr(ctx, "store ptr " + result + ", ptr " + lhs_ptr);
        return result;
      }

      static const struct { AssignOp op; BinaryOp bop; } compound_map[] = {
        { AssignOp::Add, BinaryOp::Add }, { AssignOp::Sub, BinaryOp::Sub },
        { AssignOp::Mul, BinaryOp::Mul }, { AssignOp::Div, BinaryOp::Div },
        { AssignOp::Mod, BinaryOp::Mod }, { AssignOp::Shl, BinaryOp::Shl },
        { AssignOp::Shr, BinaryOp::Shr }, { AssignOp::BitAnd, BinaryOp::BitAnd },
        { AssignOp::BitOr, BinaryOp::BitOr }, { AssignOp::BitXor, BinaryOp::BitXor },
      };
      const char* instr = nullptr;
      TypeSpec op_ts = lhs_ts;
      for (const auto& row : compound_map) {
        if (row.op != a.op) continue;
        if ((row.bop == BinaryOp::Add || row.bop == BinaryOp::Sub ||
             row.bop == BinaryOp::Mul || row.bop == BinaryOp::Div) &&
            (is_complex_base(lhs_ts.base) || is_complex_base(rhs_ts.base))) {
          const std::string result =
              emit_complex_binary_arith(ctx, row.bop, loaded, lhs_ts, rhs, rhs_ts, lhs_ts);
          emit_instr(ctx, "store " + lty + " " + result + ", ptr " + lhs_ptr);
          return result;
        }
        if (is_vector_value(lhs_ts)) {
          op_ts = lhs_ts;
        } else
        // Compute the operation type via C usual arithmetic conversions.
        if (lhs_ts.ptr_level == 0 && lhs_ts.array_rank == 0 &&
            rhs_ts.ptr_level == 0 && rhs_ts.array_rank == 0) {
          if (is_float_base(lhs_ts.base) || is_float_base(rhs_ts.base)) {
            op_ts.base = (lhs_ts.base == TB_DOUBLE || rhs_ts.base == TB_DOUBLE ||
                          lhs_ts.base == TB_LONGDOUBLE || rhs_ts.base == TB_LONGDOUBLE)
                             ? TB_DOUBLE
                             : TB_FLOAT;
          } else if (is_any_int(lhs_ts.base) && is_any_int(rhs_ts.base)) {
            const bool is_shift = (row.bop == BinaryOp::Shl || row.bop == BinaryOp::Shr);
            op_ts.base = is_shift ? integer_promote(lhs_ts.base)
                                  : usual_arith_conv(lhs_ts.base, rhs_ts.base);
          }
        }
        rhs = coerce(ctx, rhs, rhs_ts, op_ts);
        const bool lf = is_float_base(op_ts.base);
        const bool ls = is_signed_int(op_ts.base);
        // Re-use binary arith table by emitting inline
        static const struct { BinaryOp op; const char* is; const char* iu; const char* f; } tbl[] = {
          { BinaryOp::Add, "add",  "add",  "fadd" }, { BinaryOp::Sub, "sub", "sub", "fsub" },
          { BinaryOp::Mul, "mul",  "mul",  "fmul" }, { BinaryOp::Div, "sdiv","udiv","fdiv"  },
          { BinaryOp::Mod, "srem", "urem", nullptr }, { BinaryOp::Shl, "shl","shl", nullptr  },
          { BinaryOp::Shr, "ashr","lshr", nullptr  }, { BinaryOp::BitAnd,"and","and",nullptr  },
          { BinaryOp::BitOr, "or","or",   nullptr  }, { BinaryOp::BitXor,"xor","xor",nullptr },
        };
        for (const auto& r : tbl) {
          if (r.op == row.bop) {
            if (lf) instr = r.f;
            else if (r.op == BinaryOp::Shr) instr = is_signed_int(lhs_ts.base) ? r.is : r.iu;
            else instr = ls ? r.is : r.iu;
            break;
          }
        }
        break;
      }
      if (!instr) throw std::runtime_error("HirEmitter: compound assign: unknown op");

      const std::string op_ty = llvm_ty(op_ts);
      std::string lhs_op = loaded;
      if (op_ty != lty) lhs_op = coerce(ctx, loaded, lhs_ts, op_ts);
      const std::string result = fresh_tmp(ctx);
      emit_instr(ctx, result + " = " + instr + " " + op_ty + " " + lhs_op + ", " + rhs);
      std::string store_v = result;
      if (op_ty != lty) store_v = coerce(ctx, result, op_ts, lhs_ts);
      emit_instr(ctx, "store " + lty + " " + store_v + ", ptr " + lhs_ptr);
      return store_v;
    }

    rhs = coerce(ctx, rhs, rhs_ts, lhs_ts);
    const bool is_agg = (lhs_ts.base == TB_STRUCT || lhs_ts.base == TB_UNION) &&
                        lhs_ts.ptr_level == 0 && lhs_ts.array_rank == 0;
    if (is_agg && (rhs == "0" || rhs.empty())) rhs = "zeroinitializer";
    emit_instr(ctx, "store " + lty + " " + rhs + ", ptr " + lhs_ptr);
    return rhs;
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const CastExpr& c, const Expr& /*e*/){
    TypeSpec from_ts{};
    const std::string val = emit_rval_id(ctx, c.expr, from_ts);
    return coerce(ctx, val, from_ts, c.to_type.spec);
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const CallExpr& call, const Expr& e){
    TypeSpec callee_ts{};
    std::string callee_val;
    const Expr& callee_e = get_expr(call.callee);
    bool unresolved_external_callee = false;
    std::string unresolved_external_name;
    const BuiltinId builtin_id = call.builtin_id;
    const BuiltinInfo* builtin = builtin_by_id(builtin_id);
    if (builtin && builtin->category == BuiltinCategory::AliasCall &&
        !builtin->canonical_name.empty()) {
      callee_val = llvm_global_sym(std::string(builtin->canonical_name));
      unresolved_external_callee = true;
      unresolved_external_name = std::string(builtin->canonical_name);
    } else if (const auto* r = std::get_if<DeclRef>(&callee_e.payload);
        r && !r->local && !r->param_index && !r->global) {
      // Treat unresolved decl refs in call position as external functions.
      callee_val = llvm_global_sym(r->name);
      callee_ts = resolve_payload_type(ctx, *r);
      unresolved_external_callee = true;
      unresolved_external_name = r->name;
    } else {
      callee_val = emit_rval_id(ctx, call.callee, callee_ts);
    }
    TypeSpec ret_spec = resolve_payload_type(ctx, call);
    if (ret_spec.base == TB_VOID && ret_spec.ptr_level == 0 && ret_spec.array_rank == 0) {
      ret_spec = e.type.spec;
    }
    const std::string ret_ty = llvm_ret_ty(ret_spec);
    const bool builtin_special =
        builtin && builtin->lowering != BuiltinLoweringKind::AliasCall;
    if (unresolved_external_callee && !builtin_special) {
      record_extern_call_decl(unresolved_external_name, ret_ty);
    }

    // Look up function signature for argument type coercion
    std::string fn_name;
    if (builtin_id != BuiltinId::Unknown) {
      fn_name = std::string(builtin_name_from_id(builtin_id));
    } else if (const auto* r = std::get_if<DeclRef>(&callee_e.payload)) {
      fn_name = r->name;
    }

    if (builtin_id == BuiltinId::Unknown && has_builtin_prefix(fn_name)) {
      throw std::runtime_error("HirEmitter: unsupported builtin call: " + fn_name);
    }

    // Handle GCC/Clang builtins
    if (builtin_special) {
      if (builtin_id == BuiltinId::Memcpy && call.args.size() >= 3) {
        TypeSpec dst_ts{};
        TypeSpec src_ts{};
        TypeSpec size_ts{};
        const std::string dst = emit_rval_id(ctx, call.args[0], dst_ts);
        const std::string src = emit_rval_id(ctx, call.args[1], src_ts);
        std::string size = emit_rval_id(ctx, call.args[2], size_ts);
        TypeSpec i64_ts{};
        i64_ts.base = TB_ULONGLONG;
        size = coerce(ctx, size, size_ts, i64_ts);
        need_llvm_memcpy_ = true;
        emit_lir_op(ctx, lir::LirMemcpyOp{dst, src, size, false});
        return dst;
      }
      if (builtin_id == BuiltinId::VaStart && call.args.size() >= 1) {
        TypeSpec ap_ts{};
        const std::string ap_ptr = emit_va_list_obj_ptr(ctx, call.args[0], ap_ts);
        need_llvm_va_start_ = true;
        emit_lir_op(ctx, lir::LirVaStartOp{ap_ptr});
        return "";
      }
      if (builtin_id == BuiltinId::VaEnd && call.args.size() >= 1) {
        TypeSpec ap_ts{};
        const std::string ap_ptr = emit_va_list_obj_ptr(ctx, call.args[0], ap_ts);
        need_llvm_va_end_ = true;
        emit_lir_op(ctx, lir::LirVaEndOp{ap_ptr});
        return "";
      }
      if (builtin_id == BuiltinId::VaCopy && call.args.size() >= 2) {
        TypeSpec dst_ts{};
        TypeSpec src_ts{};
        const std::string dst_ptr = emit_va_list_obj_ptr(ctx, call.args[0], dst_ts);
        const std::string src_ptr = emit_va_list_obj_ptr(ctx, call.args[1], src_ts);
        need_llvm_va_copy_ = true;
        emit_lir_op(ctx, lir::LirVaCopyOp{dst_ptr, src_ptr});
        return "";
      }
      if (builtin_id == BuiltinId::Alloca && call.args.size() == 1) {
        TypeSpec size_ts{};
        std::string size = emit_rval_id(ctx, call.args[0], size_ts);
        TypeSpec i64_ts{}; i64_ts.base = TB_ULONGLONG;
        size = coerce(ctx, size, size_ts, i64_ts);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = alloca i8, i64 " + size + ", align 16");
        return tmp;
      }
      if (builtin_id == BuiltinId::ConstantP) {
        // At -O0, __builtin_constant_p always returns 0
        // Still need to emit args for side effects
        for (auto& a : call.args) {
          TypeSpec dummy{};
          emit_rval_id(ctx, a, dummy);
        }
        return "0";
      }
      if (builtin_id == BuiltinId::ClassifyType && call.args.size() == 1) {
        // GCC __builtin_classify_type: return integer type class
        // We need the type of the argument, not its value
        TypeSpec arg_ts{};
        emit_rval_id(ctx, call.args[0], arg_ts);
        // GCC type classes:
        // 0=void, 1=integer, 2=char, 3=enum(=integer), 5=pointer,
        // 8=real, 9=complex, 12=record, 13=union, 14=array, 15=string(=pointer)
        int type_class = 5; // default: pointer (for unknown)
        if (arg_ts.ptr_level > 0 || arg_ts.is_fn_ptr) {
          type_class = 5; // pointer
        } else if (is_complex_base(arg_ts.base)) {
          type_class = 9; // complex
        } else if (is_float_base(arg_ts.base)) {
          type_class = 8; // real
        } else if (arg_ts.base == TB_VOID) {
          type_class = 0; // void
        } else if (arg_ts.base == TB_BOOL || arg_ts.base == TB_CHAR ||
                   arg_ts.base == TB_SCHAR || arg_ts.base == TB_UCHAR) {
          type_class = 1; // integer (char is integer class in GCC)
        } else if (is_any_int(arg_ts.base)) {
          type_class = 1; // integer
        } else if (arg_ts.base == TB_STRUCT) {
          type_class = 12; // record
        } else if (arg_ts.base == TB_UNION) {
          type_class = 13; // union
        }
        return std::to_string(type_class);
      }
      if ((builtin_id == BuiltinId::AddOverflow ||
           builtin_id == BuiltinId::SubOverflow ||
           builtin_id == BuiltinId::MulOverflow) && call.args.size() == 3) {
        TypeSpec a_ts{}, b_ts{}, p_ts{};
        std::string a = emit_rval_id(ctx, call.args[0], a_ts);
        std::string b = emit_rval_id(ctx, call.args[1], b_ts);
        const std::string result_ptr = emit_rval_id(ctx, call.args[2], p_ts);
        // Determine the result type from the pointer target type
        TypeSpec res_ts = p_ts;
        res_ts.ptr_level--;
        const std::string res_ty = llvm_ty(res_ts);
        // Coerce operands to result type
        a = coerce(ctx, a, a_ts, res_ts);
        b = coerce(ctx, b, b_ts, res_ts);
        // Choose signed or unsigned intrinsic based on result type
        const bool is_signed = is_signed_int(res_ts.base);
        std::string op;
        if (builtin_id == BuiltinId::AddOverflow) op = is_signed ? "sadd" : "uadd";
        else if (builtin_id == BuiltinId::SubOverflow) op = is_signed ? "ssub" : "usub";
        else op = is_signed ? "smul" : "umul";
        const std::string intrinsic = "@llvm." + op + ".with.overflow." + res_ty;
        const std::string pair = fresh_tmp(ctx);
        emit_instr(ctx, pair + " = call { " + res_ty + ", i1 } " + intrinsic +
                        "(" + res_ty + " " + a + ", " + res_ty + " " + b + ")");
        const std::string val = fresh_tmp(ctx);
        emit_instr(ctx, val + " = extractvalue { " + res_ty + ", i1 } " + pair + ", 0");
        const std::string ovf = fresh_tmp(ctx);
        emit_instr(ctx, ovf + " = extractvalue { " + res_ty + ", i1 } " + pair + ", 1");
        emit_instr(ctx, "store " + res_ty + " " + val + ", ptr " + result_ptr);
        const std::string ret = fresh_tmp(ctx);
        emit_instr(ctx, ret + " = zext i1 " + ovf + " to i32");
        return ret;
      }
      if (builtin_id == BuiltinId::Bswap16 || builtin_id == BuiltinId::Bswap32 ||
          builtin_id == BuiltinId::Bswap64) {
        if (call.args.size() == 1) {
          TypeSpec arg_ts{};
          const std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
          const std::string aty = llvm_ty(arg_ts);
          const std::string intrinsic = "@llvm.bswap." + aty;
          const std::string tmp = fresh_tmp(ctx);
          emit_instr(ctx, tmp + " = call " + aty + " " + intrinsic + "(" + aty + " " + arg + ")");
          return tmp;
        }
      }
      if (builtin_id == BuiltinId::Expect && call.args.size() >= 1) {
        TypeSpec arg_ts{};
        std::string result = emit_rval_id(ctx, call.args[0], arg_ts);
        // Evaluate remaining args for side effects (e.g., z++)
        for (size_t i = 1; i < call.args.size(); ++i) {
          TypeSpec dummy{};
          emit_rval_id(ctx, call.args[i], dummy);
        }
        return result;
      }
      if (builtin_id == BuiltinId::Unreachable && call.args.empty()) {
        emit_term(ctx, "unreachable");
        return "";
      }
      if (builtin_id == BuiltinId::Prefetch) {
        // __builtin_prefetch is a no-op hint; evaluate args for side effects only
        TypeSpec dummy{};
        for (size_t i = 0; i < call.args.size(); ++i) {
          emit_rval_id(ctx, call.args[i], dummy);
        }
        return "";
      }
      switch (builtin_constant_fp_kind(builtin_id)) {
        case BuiltinConstantFpKind::Infinity:
          if (builtin_id == BuiltinId::HugeValF || builtin_id == BuiltinId::InfF) {
            return fp_to_float_literal(std::numeric_limits<float>::infinity());
          }
          if (auto base = fp_base_from_builtin_result_kind(
                  builtin_result_kind(builtin_id))) {
            return fp_literal(*base, std::numeric_limits<double>::infinity());
          }
          break;
        case BuiltinConstantFpKind::QuietNaN:
          if (builtin_id == BuiltinId::NanF || builtin_id == BuiltinId::NansF) {
            return fp_to_float_literal(std::numeric_limits<float>::quiet_NaN());
          }
          if (auto base = fp_base_from_builtin_result_kind(
                  builtin_result_kind(builtin_id))) {
            return fp_literal(*base, std::numeric_limits<double>::quiet_NaN());
          }
          break;
        case BuiltinConstantFpKind::None:
          break;
      }
      if ((builtin_id == BuiltinId::SignBit ||
           builtin_id == BuiltinId::SignBitF ||
           builtin_id == BuiltinId::SignBitL) && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        // Promote float to double for __builtin_signbit (generic version)
        if (arg_ts.base == TB_FLOAT && builtin_id == BuiltinId::SignBit) {
          const std::string ext = fresh_tmp(ctx);
          emit_instr(ctx, ext + " = fpext float " + arg + " to double");
          arg = ext;
          arg_ts.base = TB_DOUBLE;
        }
        // Bitcast to integer of same width, then check sign bit
        const std::string fp_ty = llvm_ty(arg_ts);
        std::string int_ty;
        if (arg_ts.base == TB_FLOAT) int_ty = "i32";
        else if (arg_ts.base == TB_LONGDOUBLE) int_ty = "i128";
        else int_ty = "i64"; // double or promoted
        const std::string bc = fresh_tmp(ctx);
        emit_instr(ctx, bc + " = bitcast " + fp_ty + " " + arg + " to " + int_ty);
        const std::string shifted = fresh_tmp(ctx);
        int shift_bits = (int_ty == "i32") ? 31 : (int_ty == "i128") ? 127 : 63;
        emit_instr(ctx, shifted + " = lshr " + int_ty + " " + bc + ", " + std::to_string(shift_bits));
        const std::string trunc = fresh_tmp(ctx);
        emit_instr(ctx, trunc + " = trunc " + int_ty + " " + shifted + " to i32");
        return trunc;
      }
      if (const char* pred = builtin_fp_compare_predicate(builtin_id);
          pred && call.args.size() == 2) {
        TypeSpec at{}, bt{};
        std::string a = emit_rval_id(ctx, call.args[0], at);
        std::string b = emit_rval_id(ctx, call.args[1], bt);
        auto promote_to_double = [&](std::string& v, TypeSpec& ts) {
          if (ts.base == TB_FLOAT && ts.ptr_level == 0 && ts.array_rank == 0) {
            const std::string p = fresh_tmp(ctx);
            emit_instr(ctx, p + " = fpext float " + v + " to double");
            v = p; ts.base = TB_DOUBLE;
          }
        };
        promote_to_double(a, at);
        promote_to_double(b, bt);
        const std::string fty = llvm_ty(at);
        const std::string cmp = fresh_tmp(ctx);
        emit_instr(ctx, cmp + " = fcmp " + std::string(pred) + " " + fty + " " + a + ", " + b);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + cmp + " to i32");
        return tmp;
      }
      if (builtin_id == BuiltinId::IsUnordered && call.args.size() == 2) {
        TypeSpec at{}, bt{};
        std::string a = emit_rval_id(ctx, call.args[0], at);
        std::string b = emit_rval_id(ctx, call.args[1], bt);
        auto promote_to_double = [&](std::string& v, TypeSpec& ts) {
          if (ts.base == TB_FLOAT && ts.ptr_level == 0 && ts.array_rank == 0) {
            const std::string p = fresh_tmp(ctx);
            emit_instr(ctx, p + " = fpext float " + v + " to double");
            v = p; ts.base = TB_DOUBLE;
          }
        };
        promote_to_double(a, at);
        promote_to_double(b, bt);
        const std::string fty = llvm_ty(at);
        const std::string cmp = fresh_tmp(ctx);
        emit_instr(ctx, cmp + " = fcmp uno " + fty + " " + a + ", " + b);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + cmp + " to i32");
        return tmp;
      }
      if (builtin_is_isnan(builtin_id) && call.args.size() == 1) {
        TypeSpec at{};
        std::string a = emit_rval_id(ctx, call.args[0], at);
        if (at.base == TB_FLOAT && at.ptr_level == 0 && at.array_rank == 0) {
          const std::string p = fresh_tmp(ctx);
          emit_instr(ctx, p + " = fpext float " + a + " to double");
          a = p; at.base = TB_DOUBLE;
        }
        const std::string fty = llvm_ty(at);
        const std::string cmp = fresh_tmp(ctx);
        emit_instr(ctx, cmp + " = fcmp uno " + fty + " " + a + ", " + a);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + cmp + " to i32");
        return tmp;
      }
      if (builtin_is_isinf(builtin_id) && call.args.size() == 1) {
        TypeSpec at{};
        std::string a = emit_rval_id(ctx, call.args[0], at);
        if (at.base == TB_FLOAT && at.ptr_level == 0 && at.array_rank == 0) {
          const std::string p = fresh_tmp(ctx);
          emit_instr(ctx, p + " = fpext float " + a + " to double");
          a = p; at.base = TB_DOUBLE;
        }
        const std::string fty = llvm_ty(at);
        const std::string inf_val = fp_literal(at.base, std::numeric_limits<double>::infinity());
        // Check if a == +inf or a == -inf by comparing abs(a) to +inf
        const std::string abs_tmp = fresh_tmp(ctx);
        emit_instr(ctx, abs_tmp + " = call " + fty + " @llvm.fabs." + fty + "(" + fty + " " + a + ")");
        const std::string cmp = fresh_tmp(ctx);
        emit_instr(ctx, cmp + " = fcmp oeq " + fty + " " + abs_tmp + ", " + inf_val);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + cmp + " to i32");
        return tmp;
      }
      if (builtin_is_isfinite(builtin_id) && call.args.size() == 1) {
        TypeSpec at{};
        std::string a = emit_rval_id(ctx, call.args[0], at);
        if (at.base == TB_FLOAT && at.ptr_level == 0 && at.array_rank == 0) {
          const std::string p = fresh_tmp(ctx);
          emit_instr(ctx, p + " = fpext float " + a + " to double");
          a = p; at.base = TB_DOUBLE;
        }
        const std::string fty = llvm_ty(at);
        const std::string inf_val = fp_literal(at.base, std::numeric_limits<double>::infinity());
        const std::string abs_tmp = fresh_tmp(ctx);
        emit_instr(ctx, abs_tmp + " = call " + fty + " @llvm.fabs." + fty + "(" + fty + " " + a + ")");
        const std::string cmp = fresh_tmp(ctx);
        emit_instr(ctx, cmp + " = fcmp olt " + fty + " " + abs_tmp + ", " + inf_val);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + cmp + " to i32");
        return tmp;
      }
      if ((builtin_id == BuiltinId::Copysign || builtin_id == BuiltinId::CopysignL ||
           builtin_id == BuiltinId::CopysignF) && call.args.size() == 2) {
        TypeSpec at{}, bt{};
        std::string a = emit_rval_id(ctx, call.args[0], at);
        std::string b = emit_rval_id(ctx, call.args[1], bt);
        const bool is_float = (builtin_id == BuiltinId::CopysignF);
        const bool is_long_double = (builtin_id == BuiltinId::CopysignL);
        if (is_float) {
          auto to_float = [&](std::string& v, TypeSpec& ts) {
            if (ts.base == TB_DOUBLE && ts.ptr_level == 0 && ts.array_rank == 0) {
              const std::string p = fresh_tmp(ctx); emit_instr(ctx, p + " = fptrunc double " + v + " to float");
              v = p; ts.base = TB_FLOAT;
            }
          };
          to_float(a, at); to_float(b, bt);
          const std::string tmp = fresh_tmp(ctx);
          emit_instr(ctx, tmp + " = call float @llvm.copysign.f32(float " + a + ", float " + b + ")");
          return tmp;
        } else if (is_long_double) {
          auto to_fp128 = [&](std::string& v, TypeSpec& ts) {
            if (ts.base == TB_FLOAT && ts.ptr_level == 0 && ts.array_rank == 0) {
              const std::string p = fresh_tmp(ctx);
              emit_instr(ctx, p + " = fpext float " + v + " to fp128");
              v = p; ts.base = TB_LONGDOUBLE;
            } else if (ts.base == TB_DOUBLE && ts.ptr_level == 0 && ts.array_rank == 0) {
              const std::string p = fresh_tmp(ctx);
              emit_instr(ctx, p + " = fpext double " + v + " to fp128");
              v = p; ts.base = TB_LONGDOUBLE;
            }
          };
          to_fp128(a, at); to_fp128(b, bt);
          const std::string tmp = fresh_tmp(ctx);
          emit_instr(ctx, tmp + " = call fp128 @llvm.copysign.f128(fp128 " + a + ", fp128 " + b + ")");
          return tmp;
        } else {
          // Ensure both are double
          auto to_double = [&](std::string& v, TypeSpec& ts) {
            if (ts.base == TB_FLOAT && ts.ptr_level == 0 && ts.array_rank == 0) {
              const std::string p = fresh_tmp(ctx); emit_instr(ctx, p + " = fpext float " + v + " to double");
              v = p; ts.base = TB_DOUBLE;
            }
          };
          to_double(a, at); to_double(b, bt);
          const std::string tmp = fresh_tmp(ctx);
          emit_instr(ctx, tmp + " = call double @llvm.copysign.f64(double " + a + ", double " + b + ")");
          return tmp;
        }
      }
      if ((builtin_id == BuiltinId::Fabs || builtin_id == BuiltinId::FabsL ||
           builtin_id == BuiltinId::FabsF) && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        if (builtin_id == BuiltinId::Fabs) {
          if (arg_ts.base == TB_FLOAT && arg_ts.ptr_level == 0 && arg_ts.array_rank == 0) {
            const std::string promoted = fresh_tmp(ctx);
            emit_instr(ctx, promoted + " = fpext float " + arg + " to double");
            arg = promoted;
            arg_ts.base = TB_DOUBLE;
          }
        } else if (builtin_id == BuiltinId::FabsL) {
          if (arg_ts.base == TB_FLOAT && arg_ts.ptr_level == 0 && arg_ts.array_rank == 0) {
            const std::string promoted = fresh_tmp(ctx);
            emit_instr(ctx, promoted + " = fpext float " + arg + " to fp128");
            arg = promoted;
            arg_ts.base = TB_LONGDOUBLE;
          } else if (arg_ts.base == TB_DOUBLE && arg_ts.ptr_level == 0 && arg_ts.array_rank == 0) {
            const std::string promoted = fresh_tmp(ctx);
            emit_instr(ctx, promoted + " = fpext double " + arg + " to fp128");
            arg = promoted;
            arg_ts.base = TB_LONGDOUBLE;
          }
        } else {
          // fabsf: ensure float
          if (arg_ts.base == TB_DOUBLE && arg_ts.ptr_level == 0 && arg_ts.array_rank == 0) {
            const std::string trunc = fresh_tmp(ctx);
            emit_instr(ctx, trunc + " = fptrunc double " + arg + " to float");
            arg = trunc;
            arg_ts.base = TB_FLOAT;
          }
        }
        const std::string fty = llvm_ty(arg_ts);
        const std::string intrinsic = "@llvm.fabs." + fty;
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = call " + fty + " " + intrinsic + "(" + fty + " " + arg + ")");
        return tmp;
      }
      if (builtin_is_ffs(builtin_id) && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        const bool is_ll = builtin_uses_i64_width(builtin_id);
        const std::string ity = is_ll ? "i64" : "i32";
        TypeSpec target_ts{}; target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
        arg = coerce(ctx, arg, arg_ts, target_ts);
        const std::string cttz = fresh_tmp(ctx);
        emit_instr(ctx, cttz + " = call " + ity + " @llvm.cttz." + ity +
                             "(" + ity + " " + arg + ", i1 false)");
        const std::string plus1 = fresh_tmp(ctx);
        emit_instr(ctx, plus1 + " = add " + ity + " " + cttz + ", 1");
        const std::string is_zero = fresh_tmp(ctx);
        emit_instr(ctx, is_zero + " = icmp eq " + ity + " " + arg + ", 0");
        const std::string sel = fresh_tmp(ctx);
        emit_instr(ctx, sel + " = select i1 " + is_zero + ", " + ity + " 0, " + ity + " " + plus1);
        // Always return i32
        if (is_ll) {
          const std::string trunc = fresh_tmp(ctx);
          emit_instr(ctx, trunc + " = trunc i64 " + sel + " to i32");
          return trunc;
        }
        return sel;
      }
      if (builtin_is_ctz(builtin_id) && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        const bool is_ll = builtin_uses_i64_width(builtin_id);
        const std::string ity = is_ll ? "i64" : "i32";
        TypeSpec target_ts{}; target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
        arg = coerce(ctx, arg, arg_ts, target_ts);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = call " + ity + " @llvm.cttz." + ity +
                             "(" + ity + " " + arg + ", i1 true)");
        if (is_ll) {
          const std::string trunc = fresh_tmp(ctx);
          emit_instr(ctx, trunc + " = trunc i64 " + tmp + " to i32");
          return trunc;
        }
        return tmp;
      }
      if (builtin_is_clz(builtin_id) && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        const bool is_ll = builtin_uses_i64_width(builtin_id);
        const std::string ity = is_ll ? "i64" : "i32";
        TypeSpec target_ts{}; target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
        arg = coerce(ctx, arg, arg_ts, target_ts);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = call " + ity + " @llvm.ctlz." + ity +
                             "(" + ity + " " + arg + ", i1 true)");
        if (is_ll) {
          const std::string trunc = fresh_tmp(ctx);
          emit_instr(ctx, trunc + " = trunc i64 " + tmp + " to i32");
          return trunc;
        }
        return tmp;
      }
      if (builtin_is_popcount(builtin_id) && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        const bool is_ll = builtin_uses_i64_width(builtin_id);
        const std::string ity = is_ll ? "i64" : "i32";
        TypeSpec target_ts{}; target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
        arg = coerce(ctx, arg, arg_ts, target_ts);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = call " + ity + " @llvm.ctpop." + ity +
                             "(" + ity + " " + arg + ")");
        if (is_ll) {
          const std::string trunc = fresh_tmp(ctx);
          emit_instr(ctx, trunc + " = trunc i64 " + tmp + " to i32");
          return trunc;
        }
        return tmp;
      }
      if (builtin_is_parity(builtin_id) && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        const bool is_ll = builtin_uses_i64_width(builtin_id);
        const std::string ity = is_ll ? "i64" : "i32";
        TypeSpec target_ts{}; target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
        arg = coerce(ctx, arg, arg_ts, target_ts);
        const std::string pop = fresh_tmp(ctx);
        emit_instr(ctx, pop + " = call " + ity + " @llvm.ctpop." + ity +
                            "(" + ity + " " + arg + ")");
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = and " + ity + " " + pop + ", 1");
        if (is_ll) {
          const std::string trunc = fresh_tmp(ctx);
          emit_instr(ctx, trunc + " = trunc i64 " + tmp + " to i32");
          return trunc;
        }
        return tmp;
      }
      if (builtin_is_clrsb(builtin_id) && call.args.size() == 1) {
        // clrsb(x) = clz(x ^ (x >> (bitwidth-1))) - 1
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        const bool is_ll = builtin_uses_i64_width(builtin_id);
        const std::string ity = is_ll ? "i64" : "i32";
        const int bitwidth = is_ll ? 64 : 32;
        TypeSpec target_ts{}; target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
        arg = coerce(ctx, arg, arg_ts, target_ts);
        const std::string shift = fresh_tmp(ctx);
        emit_instr(ctx, shift + " = ashr " + ity + " " + arg + ", " + std::to_string(bitwidth - 1));
        const std::string xored = fresh_tmp(ctx);
        emit_instr(ctx, xored + " = xor " + ity + " " + arg + ", " + shift);
        const std::string clz = fresh_tmp(ctx);
        emit_instr(ctx, clz + " = call " + ity + " @llvm.ctlz." + ity +
                             "(" + ity + " " + xored + ", i1 false)");
        const std::string sub1 = fresh_tmp(ctx);
        emit_instr(ctx, sub1 + " = sub " + ity + " " + clz + ", 1");
        if (is_ll) {
          const std::string trunc = fresh_tmp(ctx);
          emit_instr(ctx, trunc + " = trunc i64 " + sub1 + " to i32");
          return trunc;
        }
        return sub1;
      }
      if ((builtin_id == BuiltinId::Conj || builtin_id == BuiltinId::ConjF ||
           builtin_id == BuiltinId::ConjL) && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        if (!is_complex_base(arg_ts.base)) return arg;
        const TypeSpec elem_ts = complex_component_ts(arg_ts.base);
        const std::string complex_ty = llvm_ty(arg_ts);
        const std::string elem_ty = llvm_ty(elem_ts);
        const std::string real_v = fresh_tmp(ctx);
        emit_instr(ctx, real_v + " = extractvalue " + complex_ty + " " + arg + ", 0");
        const std::string imag_v0 = fresh_tmp(ctx);
        emit_instr(ctx, imag_v0 + " = extractvalue " + complex_ty + " " + arg + ", 1");
        const std::string imag_v = fresh_tmp(ctx);
        if (is_float_base(elem_ts.base)) {
          emit_instr(ctx, imag_v + " = fneg " + elem_ty + " " + imag_v0);
        } else {
          emit_instr(ctx, imag_v + " = sub " + elem_ty + " 0, " + imag_v0);
        }
        const std::string with_real = fresh_tmp(ctx);
        emit_instr(ctx, with_real + " = insertvalue " + complex_ty + " undef, " + elem_ty +
                                 " " + real_v + ", 0");
        const std::string out = fresh_tmp(ctx);
        emit_instr(ctx, out + " = insertvalue " + complex_ty + " " + with_real + ", " +
                            elem_ty + " " + imag_v + ", 1");
        return out;
      }
    }

    // Implicit builtins: standard library functions recognized by GCC/Clang
    // even without __builtin_ prefix. abs/labs/llabs → llvm.abs intrinsic.
    if (builtin_id == BuiltinId::Unknown && call.args.size() == 1 &&
        (fn_name == "abs" || fn_name == "labs" || fn_name == "llabs")) {
      TypeSpec arg_ts{};
      std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
      const bool is_ll = (fn_name == "llabs" || fn_name == "labs");
      const std::string ity = is_ll ? "i64" : "i32";
      TypeSpec target_ts{}; target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
      arg = coerce(ctx, arg, arg_ts, target_ts);
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirAbsOp{tmp, arg, ity});
      need_llvm_abs_ = true;
      return tmp;
    }

    // Implicit builtin: alloca() → LLVM alloca instruction (like __builtin_alloca).
    if (builtin_id == BuiltinId::Unknown && call.args.size() == 1 &&
        fn_name == "alloca") {
      TypeSpec size_ts{};
      std::string size = emit_rval_id(ctx, call.args[0], size_ts);
      TypeSpec i64_ts{}; i64_ts.base = TB_ULONGLONG;
      size = coerce(ctx, size, size_ts, i64_ts);
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = alloca i8, i64 " + size + ", align 16");
      return tmp;
    }

    const Function* target_fn = nullptr;
    if (!fn_name.empty()) {
      const auto fit = mod_.fn_index.find(fn_name);
      if (fit != mod_.fn_index.end() && fit->second.value < mod_.functions.size()) {
        target_fn = &mod_.functions[fit->second.value];
      }
    }
    const FnPtrSig* callee_fn_ptr_sig = target_fn ? nullptr : resolve_callee_fn_ptr_sig(ctx, callee_e);

    auto callee_needs_va_list_by_value_copy = [&](size_t arg_index) -> bool {
      if (target_fn) {
        if (arg_index < target_fn->params.size()) {
          const TypeSpec& param_ts = target_fn->params[arg_index].type.spec;
          return param_ts.base == TB_VA_LIST &&
                 param_ts.ptr_level == 0 &&
                 param_ts.array_rank == 0;
        }
        return false;
      }
      // Phase 4: use canonical-aware helper for va_list detection.
      if (callee_fn_ptr_sig) {
        if (arg_index < sig_param_count(*callee_fn_ptr_sig)) {
          return sig_param_is_va_list_value(*callee_fn_ptr_sig, arg_index);
        }
        return false;
      }
      return fn_name == "vprintf"   || fn_name == "vfprintf" ||
             fn_name == "vsprintf"  || fn_name == "vsnprintf" ||
             fn_name == "vscanf"    || fn_name == "vfscanf" ||
             fn_name == "vsscanf"   || fn_name == "vasprintf" ||
             fn_name == "vdprintf";
    };

    std::string args_str;
    auto apply_default_arg_promotion = [&](std::string& arg, TypeSpec& out_ts, const TypeSpec& in_ts) {
      TypeSpec promoted = in_ts;
      if (promoted.array_rank > 0 && !promoted.is_ptr_to_array) {
        promoted.array_rank = 0;
        promoted.array_size = -1;
        promoted.ptr_level += 1;
      }
      if (promoted.ptr_level == 0 && promoted.array_rank == 0) {
        if (promoted.base == TB_FLOAT) {
          promoted.base = TB_DOUBLE;
        } else if (promoted.base == TB_BOOL || promoted.base == TB_CHAR ||
                   promoted.base == TB_SCHAR || promoted.base == TB_UCHAR ||
                   promoted.base == TB_SHORT || promoted.base == TB_USHORT) {
          promoted.base = TB_INT;
        }
      }
      arg = coerce(ctx, arg, in_ts, promoted);
      out_ts = promoted;
    };
    for (size_t i = 0; i < call.args.size(); ++i) {
      TypeSpec arg_ts{};
      std::string arg = emit_rval_id(ctx, call.args[i], arg_ts);
      TypeSpec out_arg_ts = arg_ts;
      const bool is_va_list_value =
          arg_ts.base == TB_VA_LIST &&
          arg_ts.ptr_level == 0 &&
          arg_ts.array_rank == 0;
      const bool is_variadic_aggregate =
          target_fn && target_fn->attrs.variadic && i >= target_fn->params.size() &&
          (arg_ts.base == TB_STRUCT || arg_ts.base == TB_UNION) &&
          arg_ts.ptr_level == 0 && arg_ts.array_rank == 0 &&
          arg_ts.tag && arg_ts.tag[0];
      if (target_fn) {
        const bool has_void_param_list =
            target_fn->params.size() == 1 &&
            target_fn->params[0].type.spec.base == TB_VOID &&
            target_fn->params[0].type.spec.ptr_level == 0 &&
            target_fn->params[0].type.spec.array_rank == 0;
        if (!has_void_param_list && i < target_fn->params.size()) {
          out_arg_ts = target_fn->params[i].type.spec;
          arg = coerce(ctx, arg, arg_ts, out_arg_ts);
        } else if (target_fn->attrs.variadic && !is_variadic_aggregate) {
          apply_default_arg_promotion(arg, out_arg_ts, arg_ts);
        }
      } else if (callee_fn_ptr_sig) {
        // Phase 4: use canonical-aware helpers for param type extraction.
        const bool has_void_pl = sig_has_void_param_list(*callee_fn_ptr_sig);
        if (!has_void_pl && i < sig_param_count(*callee_fn_ptr_sig)) {
          out_arg_ts = sig_param_type(*callee_fn_ptr_sig, i);
          arg = coerce(ctx, arg, arg_ts, out_arg_ts);
        } else if (sig_is_variadic(*callee_fn_ptr_sig) && !is_variadic_aggregate) {
          apply_default_arg_promotion(arg, out_arg_ts, arg_ts);
        }
      }
      if (is_va_list_value && callee_needs_va_list_by_value_copy(i)) {
        TypeSpec ap_ts{};
        const std::string src_ptr = emit_va_list_obj_ptr(ctx, call.args[i], ap_ts);
        const std::string tmp_addr = fresh_tmp(ctx);
        emit_instr(ctx, tmp_addr + " = alloca %struct.__va_list_tag_");
        need_llvm_memcpy_ = true;
        emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, src_ptr, "32", false});
        arg = tmp_addr;
        out_arg_ts = arg_ts;
      }
      if (is_variadic_aggregate) {
        const auto sit = mod_.struct_defs.find(arg_ts.tag);
        const int payload_sz = sit == mod_.struct_defs.end() ? 0 : sit->second.size_bytes;
        if (payload_sz == 0) continue;

        std::string obj_ptr;
        if (get_expr(call.args[i]).type.category == ValueCategory::LValue) {
          TypeSpec obj_ts{};
          obj_ptr = emit_lval(ctx, call.args[i], obj_ts);
        } else {
          const std::string tmp_addr = fresh_tmp(ctx);
          emit_instr(ctx, tmp_addr + " = alloca " + llvm_ty(arg_ts));
          emit_instr(ctx, "store " + llvm_ty(arg_ts) + " " + arg + ", ptr " + tmp_addr);
          obj_ptr = tmp_addr;
        }

        need_llvm_memcpy_ = true;
        if (i) args_str += ", ";
        if (payload_sz > 16) {
          args_str += "ptr " + obj_ptr;
          continue;
        }
        if (payload_sz <= 8) {
          const std::string tmp_addr = fresh_tmp(ctx);
          emit_instr(ctx, tmp_addr + " = alloca i64");
          emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, obj_ptr, std::to_string(payload_sz), false});
          const std::string packed = fresh_tmp(ctx);
          emit_instr(ctx, packed + " = load i64, ptr " + tmp_addr);
          args_str += "i64 " + packed;
          continue;
        }

        const std::string tmp_addr = fresh_tmp(ctx);
        emit_instr(ctx, tmp_addr + " = alloca [2 x i64]");
        emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, obj_ptr, std::to_string(payload_sz), false});
        const std::string packed = fresh_tmp(ctx);
        emit_instr(ctx, packed + " = load [2 x i64], ptr " + tmp_addr);
        args_str += "[2 x i64] " + packed;
        continue;
      }
      if (i) args_str += ", ";
      args_str += llvm_ty(out_arg_ts) + " " + arg;
    }

    if (ret_ty == "void") {
      emit_instr(ctx, "call void " + callee_val + "(" + args_str + ")");
      return "";
    }
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = call " + ret_ty + " " + callee_val + "(" + args_str + ")");
    return tmp;
  }

std::string HirEmitter::emit_aarch64_vaarg_gp_src_ptr(FnCtx& ctx, const std::string& ap_ptr, int slot_bytes){
    const std::string offs_ptr = fresh_tmp(ctx);
    emit_instr(ctx, offs_ptr + " = getelementptr %struct.__va_list_tag_, ptr " + ap_ptr +
                            ", i32 0, i32 3");
    const std::string offs = fresh_tmp(ctx);
    emit_instr(ctx, offs + " = load i32, ptr " + offs_ptr);

    const std::string stack_lbl = fresh_lbl(ctx, "vaarg.stack.");
    const std::string reg_try_lbl = fresh_lbl(ctx, "vaarg.regtry.");
    const std::string reg_lbl = fresh_lbl(ctx, "vaarg.reg.");
    const std::string join_lbl = fresh_lbl(ctx, "vaarg.join.");

    const std::string is_stack0 = fresh_tmp(ctx);
    emit_instr(ctx, is_stack0 + " = icmp sge i32 " + offs + ", 0");
    emit_term(ctx, "br i1 " + is_stack0 + ", label %" + stack_lbl + ", label %" + reg_try_lbl);

    emit_lbl(ctx, reg_try_lbl);
    const std::string next_offs = fresh_tmp(ctx);
    emit_instr(ctx, next_offs + " = add i32 " + offs + ", " + std::to_string(slot_bytes));
    emit_instr(ctx, "store i32 " + next_offs + ", ptr " + offs_ptr);
    const std::string use_reg = fresh_tmp(ctx);
    emit_instr(ctx, use_reg + " = icmp sle i32 " + next_offs + ", 0");
    emit_term(ctx, "br i1 " + use_reg + ", label %" + reg_lbl + ", label %" + stack_lbl);

    emit_lbl(ctx, reg_lbl);
    const std::string gr_top_ptr = fresh_tmp(ctx);
    emit_instr(ctx, gr_top_ptr + " = getelementptr %struct.__va_list_tag_, ptr " + ap_ptr +
                             ", i32 0, i32 1");
    const std::string gr_top = fresh_tmp(ctx);
    emit_instr(ctx, gr_top + " = load ptr, ptr " + gr_top_ptr);
    const std::string reg_addr = fresh_tmp(ctx);
    emit_instr(ctx, reg_addr + " = getelementptr i8, ptr " + gr_top + ", i32 " + offs);
    emit_term(ctx, "br label %" + join_lbl);

    emit_lbl(ctx, stack_lbl);
    const std::string stack_ptr_ptr = fresh_tmp(ctx);
    emit_instr(ctx, stack_ptr_ptr + " = getelementptr %struct.__va_list_tag_, ptr " + ap_ptr +
                                ", i32 0, i32 0");
    const std::string stack_ptr = fresh_tmp(ctx);
    emit_instr(ctx, stack_ptr + " = load ptr, ptr " + stack_ptr_ptr);
    const std::string stack_next = fresh_tmp(ctx);
    emit_instr(ctx, stack_next + " = getelementptr i8, ptr " + stack_ptr + ", i64 " +
                            std::to_string(slot_bytes));
    emit_instr(ctx, "store ptr " + stack_next + ", ptr " + stack_ptr_ptr);
    emit_term(ctx, "br label %" + join_lbl);

    emit_lbl(ctx, join_lbl);
    const std::string src_ptr = fresh_tmp(ctx);
    emit_instr(ctx, src_ptr + " = phi ptr [ " + reg_addr + ", %" + reg_lbl + " ], [ " +
                             stack_ptr + ", %" + stack_lbl + " ]");
    return src_ptr;
  }

std::string HirEmitter::emit_aarch64_vaarg_fp_src_ptr(
      FnCtx& ctx, const std::string& ap_ptr, int reg_slot_bytes, int stack_slot_bytes,
      int stack_align_bytes){
    const std::string offs_ptr = fresh_tmp(ctx);
    emit_instr(ctx, offs_ptr + " = getelementptr %struct.__va_list_tag_, ptr " + ap_ptr +
                            ", i32 0, i32 4");
    const std::string offs = fresh_tmp(ctx);
    emit_instr(ctx, offs + " = load i32, ptr " + offs_ptr);

    const std::string stack_lbl = fresh_lbl(ctx, "vaarg.fp.stack.");
    const std::string reg_try_lbl = fresh_lbl(ctx, "vaarg.fp.regtry.");
    const std::string reg_lbl = fresh_lbl(ctx, "vaarg.fp.reg.");
    const std::string join_lbl = fresh_lbl(ctx, "vaarg.fp.join.");

    const std::string is_stack0 = fresh_tmp(ctx);
    emit_instr(ctx, is_stack0 + " = icmp sge i32 " + offs + ", 0");
    emit_term(ctx, "br i1 " + is_stack0 + ", label %" + stack_lbl + ", label %" + reg_try_lbl);

    emit_lbl(ctx, reg_try_lbl);
    const std::string next_offs = fresh_tmp(ctx);
    emit_instr(ctx, next_offs + " = add i32 " + offs + ", " + std::to_string(reg_slot_bytes));
    emit_instr(ctx, "store i32 " + next_offs + ", ptr " + offs_ptr);
    const std::string use_reg = fresh_tmp(ctx);
    emit_instr(ctx, use_reg + " = icmp sle i32 " + next_offs + ", 0");
    emit_term(ctx, "br i1 " + use_reg + ", label %" + reg_lbl + ", label %" + stack_lbl);

    emit_lbl(ctx, reg_lbl);
    const std::string vr_top_ptr = fresh_tmp(ctx);
    emit_instr(ctx, vr_top_ptr + " = getelementptr %struct.__va_list_tag_, ptr " + ap_ptr +
                             ", i32 0, i32 2");
    const std::string vr_top = fresh_tmp(ctx);
    emit_instr(ctx, vr_top + " = load ptr, ptr " + vr_top_ptr);
    const std::string reg_addr = fresh_tmp(ctx);
    emit_instr(ctx, reg_addr + " = getelementptr i8, ptr " + vr_top + ", i32 " + offs);
    emit_term(ctx, "br label %" + join_lbl);

    emit_lbl(ctx, stack_lbl);
    const std::string stack_ptr_ptr = fresh_tmp(ctx);
    emit_instr(ctx, stack_ptr_ptr + " = getelementptr %struct.__va_list_tag_, ptr " + ap_ptr +
                                ", i32 0, i32 0");
    const std::string stack_ptr = fresh_tmp(ctx);
    emit_instr(ctx, stack_ptr + " = load ptr, ptr " + stack_ptr_ptr);
    std::string aligned_stack_ptr = stack_ptr;
    if (stack_align_bytes > 1) {
      const std::string stack_i = fresh_tmp(ctx);
      emit_instr(ctx, stack_i + " = ptrtoint ptr " + stack_ptr + " to i64");
      const std::string plus_mask = fresh_tmp(ctx);
      emit_instr(ctx, plus_mask + " = add i64 " + stack_i + ", " +
                              std::to_string(stack_align_bytes - 1));
      const std::string masked = fresh_tmp(ctx);
      emit_instr(ctx, masked + " = and i64 " + plus_mask + ", " +
                              std::to_string(-stack_align_bytes));
      aligned_stack_ptr = fresh_tmp(ctx);
      emit_instr(ctx, aligned_stack_ptr + " = inttoptr i64 " + masked + " to ptr");
    }
    const std::string stack_next = fresh_tmp(ctx);
    emit_instr(ctx, stack_next + " = getelementptr i8, ptr " + aligned_stack_ptr + ", i64 " +
                            std::to_string(stack_slot_bytes));
    emit_instr(ctx, "store ptr " + stack_next + ", ptr " + stack_ptr_ptr);
    emit_term(ctx, "br label %" + join_lbl);

    emit_lbl(ctx, join_lbl);
    const std::string src_ptr = fresh_tmp(ctx);
    emit_instr(ctx, src_ptr + " = phi ptr [ " + reg_addr + ", %" + reg_lbl + " ], [ " +
                             aligned_stack_ptr + ", %" + stack_lbl + " ]");
    return src_ptr;
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const VaArgExpr& v, const Expr& e){
    TypeSpec ap_ts{};
    const std::string ap_ptr = emit_va_list_obj_ptr(ctx, v.ap, ap_ts);
    TypeSpec res_ts = e.type.spec;
    if (!has_concrete_type(res_ts)) res_ts = resolve_expr_type(ctx, e);
    const std::string res_ty = llvm_ty(res_ts);
    if (res_ty == "void") return "";
    if ((res_ts.base == TB_STRUCT || res_ts.base == TB_UNION) &&
        res_ts.ptr_level == 0 && res_ts.array_rank == 0 &&
        res_ts.tag && res_ts.tag[0]) {
      const auto it = mod_.struct_defs.find(res_ts.tag);
      if (it != mod_.struct_defs.end()) {
        const HirStructDef& sd = it->second;
        int payload_sz = 0;
        if (sd.is_union) {
          for (const auto& f : sd.fields) {
            payload_sz = std::max(payload_sz, f.size_bytes);
          }
        } else {
          payload_sz = sd.size_bytes;
        }
        // GCC zero-size aggregate extension: va_arg yields a zero-initialized value
        // and does not consume any argument slot.
        if (payload_sz == 0) return "zeroinitializer";
      }
    }
    if ((res_ts.base == TB_STRUCT || res_ts.base == TB_UNION) &&
        res_ts.ptr_level == 0 && res_ts.array_rank == 0 &&
        res_ts.tag && res_ts.tag[0]) {
      const auto it = mod_.struct_defs.find(res_ts.tag);
      if (it != mod_.struct_defs.end()) {
        const int payload_sz = it->second.size_bytes;
        if (payload_sz == 0) return "zeroinitializer";
        if (payload_sz > 0) {
          if (payload_sz > 16) {
            const std::string slot_ptr = emit_aarch64_vaarg_gp_src_ptr(ctx, ap_ptr, 8);
            const std::string src_ptr = fresh_tmp(ctx);
            emit_instr(ctx, src_ptr + " = load ptr, ptr " + slot_ptr);
            const std::string tmp_addr = fresh_tmp(ctx);
            emit_instr(ctx, tmp_addr + " = alloca " + res_ty);
            need_llvm_memcpy_ = true;
            emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, src_ptr, std::to_string(payload_sz), false});
            const std::string out = fresh_tmp(ctx);
            emit_instr(ctx, out + " = load " + res_ty + ", ptr " + tmp_addr);
            return out;
          }

          const int slot_bytes = payload_sz > 8 ? 16 : 8;
          const std::string src_ptr = emit_aarch64_vaarg_gp_src_ptr(ctx, ap_ptr, slot_bytes);
          const std::string tmp_addr = fresh_tmp(ctx);
          emit_instr(ctx, tmp_addr + " = alloca " + res_ty);
          need_llvm_memcpy_ = true;
          emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, src_ptr, std::to_string(payload_sz), false});
          const std::string out = fresh_tmp(ctx);
          emit_instr(ctx, out + " = load " + res_ty + ", ptr " + tmp_addr);
          return out;
        }
      }
    }

    const bool is_gp_scalar =
        (res_ty == "ptr") ||
        (res_ts.ptr_level == 0 && res_ts.array_rank == 0 && is_any_int(res_ts.base));
    const bool is_fp_scalar =
        res_ts.ptr_level == 0 && res_ts.array_rank == 0 &&
        (res_ts.base == TB_FLOAT || res_ts.base == TB_DOUBLE);
    const bool is_fp128_scalar =
        res_ts.ptr_level == 0 && res_ts.array_rank == 0 && res_ts.base == TB_LONGDOUBLE;

    // AArch64 va_list matches clang's __va_list_tag layout:
    // { stack, gr_top, vr_top, gr_offs, vr_offs }.
    // For integer/pointer args we first try GR save area, then fall back to stack.
    if (is_gp_scalar) {
      const std::string src_ptr = emit_aarch64_vaarg_gp_src_ptr(ctx, ap_ptr, 8);
      const std::string out = fresh_tmp(ctx);
      emit_instr(ctx, out + " = load " + res_ty + ", ptr " + src_ptr);
      return out;
    }
    if (is_fp_scalar) {
      const std::string src_ptr = emit_aarch64_vaarg_fp_src_ptr(ctx, ap_ptr, 16, 8, 8);
      const std::string out = fresh_tmp(ctx);
      emit_instr(ctx, out + " = load " + res_ty + ", ptr " + src_ptr);
      return out;
    }
    if (is_fp128_scalar) {
      const std::string src_ptr = emit_aarch64_vaarg_fp_src_ptr(ctx, ap_ptr, 16, 16, 16);
      const std::string out = fresh_tmp(ctx);
      emit_instr(ctx, out + " = load " + res_ty + ", ptr " + src_ptr);
      return out;
    }

    const std::string out = fresh_tmp(ctx);
    emit_instr(ctx, out + " = va_arg ptr " + ap_ptr + ", " + res_ty);
    return out;
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const TernaryExpr& t, const Expr& e){
    TypeSpec cond_ts{};
    const std::string cond_v = emit_rval_id(ctx, t.cond, cond_ts);
    const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);

    const std::string then_lbl     = fresh_lbl(ctx, "tern.then.");
    const std::string then_end_lbl = fresh_lbl(ctx, "tern.then.end.");
    const std::string else_lbl     = fresh_lbl(ctx, "tern.else.");
    const std::string else_end_lbl = fresh_lbl(ctx, "tern.else.end.");
    const std::string end_lbl      = fresh_lbl(ctx, "tern.end.");
    TypeSpec res_spec = resolve_expr_type(ctx, e);
    if (!has_concrete_type(res_spec)) res_spec.base = TB_INT;
    const std::string res_ty = llvm_ty(res_spec);

    emit_term(ctx, "br i1 " + cond_i1 + ", label %" + then_lbl + ", label %" + else_lbl);

    emit_lbl(ctx, then_lbl);
    TypeSpec then_ts{};
    std::string then_v = emit_rval_id(ctx, t.then_expr, then_ts);
    then_v = coerce(ctx, then_v, then_ts, res_spec);
    emit_term(ctx, "br label %" + then_end_lbl);
    emit_lbl(ctx, then_end_lbl);
    emit_term(ctx, "br label %" + end_lbl);

    emit_lbl(ctx, else_lbl);
    TypeSpec else_ts{};
    std::string else_v = emit_rval_id(ctx, t.else_expr, else_ts);
    else_v = coerce(ctx, else_v, else_ts, res_spec);
    emit_term(ctx, "br label %" + else_end_lbl);
    emit_lbl(ctx, else_end_lbl);
    emit_term(ctx, "br label %" + end_lbl);

    emit_lbl(ctx, end_lbl);
    if (res_ty == "void") return "";
    // If either arm produced a void/empty value (e.g. void function call in one arm),
    // substitute a zero value to avoid an invalid empty phi operand.
    auto void_to_zero = [&](const std::string& v) -> std::string {
      if (!v.empty()) return v;
      if (res_ty == "ptr") return "null";
      if (res_ty == "float" || res_ty == "double") return "0.0";
      return "0";
    };
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = phi " + res_ty +
                        " [ " + void_to_zero(then_v) + ", %" + then_end_lbl + " ]," +
                        " [ " + void_to_zero(else_v) + ", %" + else_end_lbl + " ]");
    return tmp;
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const SizeofExpr& s, const Expr&){
    const Expr& op = get_expr(s.expr);
    TypeSpec op_ts = resolve_expr_type(ctx, op);
    if (!has_concrete_type(op_ts)) {
      op_ts = std::visit([&](const auto& p) -> TypeSpec {
        return resolve_payload_type(ctx, p);
      }, op.payload);
    }
    // String literals: sizeof("abc") = 4 (array size, not pointer size)
    if (const auto* sl = std::get_if<StringLiteral>(&op.payload)) {
      if (sl->is_wide) {
        const auto vals = decode_wide_string_values(sl->raw);
        return std::to_string(vals.size() * 4);  // vals includes null terminator
      }
      const auto bytes = bytes_from_string_literal(*sl);
      return std::to_string(bytes.size() + 1);  // +1 for null terminator
    }
    // DeclRef: get type from global/local declaration (NK_VAR nodes have no type set).
    if (const auto* r = std::get_if<DeclRef>(&op.payload)) {
      auto resolve_named_global_object_type = [&](const std::string& name) -> std::optional<TypeSpec> {
        if (const GlobalVar* best = select_global_object(name)) return best->type.spec;
        return std::nullopt;
      };
      if (r->global) {
        if (const GlobalVar* gv = select_global_object(*r->global)) op_ts = gv->type.spec;
      } else if (r->local) {
        const auto it = ctx.local_types.find(r->local->value);
        if (it != ctx.local_types.end()) op_ts = it->second;
      } else if (r->param_index && ctx.fn &&
                 *r->param_index < ctx.fn->params.size()) {
        op_ts = ctx.fn->params[*r->param_index].type.spec;
        // Array params decay to pointer in C — sizeof(param) = sizeof(void*)
        if (op_ts.array_rank > 0) {
          op_ts.array_rank = 0; op_ts.array_size = -1; op_ts.ptr_level = 1;
        }
      } else if (!r->name.empty()) {
        if (auto named_ts = resolve_named_global_object_type(r->name)) {
          op_ts = *named_ts;
        }
      }
      if (op_ts.array_rank == 0 && !r->name.empty()) {
        if (auto named_ts = resolve_named_global_object_type(r->name)) {
          op_ts = *named_ts;
        }
      }
    }
    if (!has_concrete_type(op_ts)) return "8";
    return std::to_string(sizeof_ts(mod_, op_ts));
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const SizeofTypeExpr& s, const Expr&){
    return std::to_string(sizeof_ts(mod_, s.type.spec));
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const LabelAddrExpr& la, const Expr&){
    return "blockaddress(@" + ctx.fn->name + ", %ulbl_" + la.label_name + ")";
  }

std::string HirEmitter::emit_rval_payload(FnCtx&, const PendingConstevalExpr& p, const Expr&){
    throw std::runtime_error(
        "HirEmitter: unevaluated PendingConstevalExpr reached codegen for '" + p.fn_name + "'");
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const IndexExpr&, const Expr& e){
    if (const auto* idx = std::get_if<IndexExpr>(&e.payload)) {
      const TypeSpec base_ts = resolve_expr_type(ctx, idx->base);
      if (is_vector_value(base_ts)) {
        TypeSpec vec_ts{};
        const std::string vec = emit_rval_id(ctx, idx->base, vec_ts);
        TypeSpec ix_ts{};
        std::string ix = emit_rval_id(ctx, idx->index, ix_ts);
        TypeSpec i32_ts{};
        i32_ts.base = TB_INT;
        ix = coerce(ctx, ix, ix_ts, i32_ts);
        TypeSpec elem_ts = base_ts;
        elem_ts.is_vector = false;
        elem_ts.vector_lanes = 0;
        elem_ts.vector_bytes = 0;
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = extractelement " + llvm_ty(base_ts) + " " + vec +
                            ", i32 " + ix);
        return tmp;
      }
    }
    TypeSpec pts{};
    const std::string ptr = emit_lval_dispatch(ctx, e, pts);
    // Array element decay: if the element type is itself an array, return its
    // address directly (C array-to-pointer decay) rather than loading it.
    if (pts.array_rank > 0 && pts.ptr_level == 0) return ptr;
    TypeSpec load_ts = resolve_expr_type(ctx, e);
    if (!has_concrete_type(load_ts)) load_ts = pts;
    const std::string ty = llvm_ty(load_ts);
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = load " + ty + ", ptr " + ptr);
    return tmp;
  }

std::string HirEmitter::emit_rval_payload(FnCtx& ctx, const MemberExpr& m, const Expr& e){
    TypeSpec field_ts{};
    BitfieldAccess bf;
    const std::string gep = emit_member_lval(ctx, m, field_ts, &bf);
    // Bitfield: use shift/mask load
    if (bf.is_bitfield()) {
      return emit_bitfield_load(ctx, gep, bf);
    }
    // Use stored type if available, else use resolved field type
    const TypeSpec& load_ts = (e.type.spec.base != TB_VOID || e.type.spec.ptr_level > 0)
                                  ? e.type.spec
                                  : field_ts;
    // Array fields: decay to pointer-to-first-element (no load needed)
    if (load_ts.array_rank > 0 || field_ts.array_rank > 0) {
      const TypeSpec& arr_ts = (field_ts.array_rank > 0) ? field_ts : load_ts;
      const std::string arr_alloca_ty = llvm_alloca_ty(arr_ts);
      if (arr_alloca_ty == "ptr") {
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = load ptr, ptr " + gep);
        return tmp;
      }
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = getelementptr " + arr_alloca_ty +
                          ", ptr " + gep + ", i64 0, i64 0");
      return tmp;
    }
    const std::string ty = llvm_ty(load_ts);
    if (ty == "void") return "";
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = load " + ty + ", ptr " + gep);
    return tmp;
  }

void HirEmitter::emit_stmt(FnCtx& ctx, const Stmt& stmt){
    std::visit([&](const auto& s) { emit_stmt_impl(ctx, s); }, stmt.payload);
  }

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const LocalDecl& d){
    if (d.fn_ptr_sig) {
      ctx.local_fn_ptr_sigs[d.id.value] = *d.fn_ptr_sig;
    }
    if (d.vla_size) {
      const std::string slot = ctx.local_slots.at(d.id.value);
      TypeSpec sz_ts{};
      std::string count = emit_rval_id(ctx, *d.vla_size, sz_ts);
      TypeSpec i64_ts{};
      i64_ts.base = TB_LONGLONG;
      count = coerce(ctx, count, sz_ts, i64_ts);

      long long static_mult = 1;
      for (int i = 1; i < d.type.spec.array_rank; ++i) {
        const long long dim = d.type.spec.array_dims[i];
        if (dim > 0) static_mult *= dim;
      }
      if (static_mult > 1) {
        const std::string scaled = fresh_tmp(ctx);
        emit_instr(ctx, scaled + " = mul i64 " + count + ", " + std::to_string(static_mult));
        count = scaled;
      }

      TypeSpec elem_ts = d.type.spec;
      elem_ts.array_rank = 0;
      elem_ts.array_size = -1;
      for (int i = 0; i < 8; ++i) elem_ts.array_dims[i] = -1;
      std::string elem_ty = llvm_ty(elem_ts);
      if (elem_ty == "void") elem_ty = "i8";
      const std::string dyn_ptr = fresh_tmp(ctx);
      emit_instr(ctx, dyn_ptr + " = alloca " + elem_ty + ", i64 " + count);
      emit_instr(ctx, "store ptr " + dyn_ptr + ", ptr " + slot);
    }

    if (!d.init) return;
    const std::string slot = ctx.local_slots.at(d.id.value);
    TypeSpec rhs_ts{};
    std::string rhs = emit_rval_id(ctx, *d.init, rhs_ts);
    const std::string ty =
        (d.type.spec.array_rank > 0) ? llvm_alloca_ty(d.type.spec) : llvm_ty(d.type.spec);
    const bool is_agg_or_array =
        d.type.spec.array_rank > 0 ||
        (d.type.spec.is_vector && d.type.spec.vector_lanes > 0) ||
        (d.type.spec.ptr_level == 0 &&
         (d.type.spec.base == TB_STRUCT || d.type.spec.base == TB_UNION) &&
         d.type.spec.array_rank == 0);
    if (is_agg_or_array && (rhs == "0" || rhs.empty())) {
      emit_instr(ctx, "store " + ty + " zeroinitializer, ptr " + slot);
      return;
    }
    rhs = coerce(ctx, rhs, rhs_ts, d.type.spec);
    emit_instr(ctx, "store " + ty + " " + rhs + ", ptr " + slot);
  }

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const ExprStmt& s){
    if (!s.expr) return;
    TypeSpec ts{};
    emit_rval_id(ctx, *s.expr, ts);
  }

// Rewrite GCC inline asm constraints that LLVM aarch64 backend doesn't support.
// 'g' (general operand) → 'imr' (immediate, memory, register).
static std::string rewrite_asm_constraints(const std::string& raw) {
    std::string result;
    result.reserve(raw.size() + 8);
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '~' && i + 1 < raw.size() && raw[i + 1] == '{') {
            // skip clobber spec ~{...} verbatim
            auto end = raw.find('}', i);
            if (end != std::string::npos) {
                result.append(raw, i, end - i + 1);
                i = end;
                continue;
            }
        }
        if (raw[i] == 'g') {
            // Ensure 'g' is a standalone constraint letter, not part of a clobber name.
            // In the constraint string, each comma-separated segment is a constraint spec.
            // 'g' should be replaced with 'imr'.
            result += "imr";
        } else {
            result += raw[i];
        }
    }
    return result;
}

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const InlineAsmStmt& s){
    const std::string asm_text = escape_llvm_c_bytes(s.asm_template);
    const std::string constraints = rewrite_asm_constraints(escape_llvm_c_bytes(s.constraints));
    TypeSpec ret_ts{};
    std::string ret_ty = "void";
    if (s.output) {
      ret_ts = resolve_expr_type(ctx, *s.output);
      ret_ty = llvm_ty(ret_ts);
    }
    std::vector<std::string> arg_vals;
    std::vector<TypeSpec> arg_tys;
    for (ExprId input : s.inputs) {
      TypeSpec in_ts{};
      arg_vals.push_back(emit_rval_id(ctx, input, in_ts));
      arg_tys.push_back(in_ts);
    }
    std::string call = "call " + ret_ty + " asm ";
    if (s.has_side_effects) call += "sideeffect ";
    call += "\"" + asm_text + "\", \"" + constraints + "\"(";
    for (size_t i = 0; i < arg_vals.size(); ++i) {
      if (i) call += ", ";
      call += llvm_ty(arg_tys[i]) + " " + arg_vals[i];
    }
    call += ")";
    if (!s.output) {
      emit_instr(ctx, call);
      return;
    }

    const std::string result = fresh_tmp(ctx);
    emit_instr(ctx, result + " = " + call);
    TypeSpec out_pointee_ts{};
    const std::string out_ptr = emit_lval(ctx, *s.output, out_pointee_ts);
    const std::string coerced = coerce(ctx, result, ret_ts, out_pointee_ts);
    emit_instr(ctx, "store " + llvm_ty(out_pointee_ts) + " " + coerced + ", ptr " + out_ptr);
  }

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const ReturnStmt& s){
    if (!s.expr) {
      const auto& rts = ctx.fn->return_type.spec;
      if (rts.base == TB_VOID && rts.ptr_level == 0 && rts.array_rank == 0 &&
          !rts.is_lvalue_ref && !rts.is_rvalue_ref) {
        emit_term(ctx, "ret void");
      } else {
        // C89: bare 'return;' in non-void function → return 0/null/0.0
        const std::string ret_ty = llvm_ret_ty(rts);
        if (ret_ty == "ptr") {
          emit_term(ctx, "ret ptr null");
        } else if (is_float_base(rts.base) && rts.ptr_level == 0) {
          emit_term(ctx, "ret " + ret_ty + " 0.0");
        } else {
          emit_term(ctx, "ret " + ret_ty + " 0");
        }
      }
      return;
    }
    TypeSpec ts{};
    std::string val = emit_rval_id(ctx, *s.expr, ts);
    // For reference returns, the LLVM return type is ptr, so coerce to ptr.
    TypeSpec coerce_target = ctx.fn->return_type.spec;
    if ((coerce_target.is_lvalue_ref || coerce_target.is_rvalue_ref) && coerce_target.ptr_level == 0) {
      coerce_target.ptr_level++;
    }
    val = coerce(ctx, val, ts, coerce_target);
    emit_term(ctx, "ret " + llvm_ret_ty(ctx.fn->return_type.spec) + " " + val);
  }

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const IfStmt& s){
    TypeSpec cond_ts{};
    const std::string cond_v  = emit_rval_id(ctx, s.cond, cond_ts);
    const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);
    const std::string then_lbl = block_lbl(s.then_block);
    const std::string after_lbl = block_lbl(s.after_block);
    if (s.else_block) {
      emit_term(ctx, "br i1 " + cond_i1 + ", label %" + then_lbl +
                         ", label %" + block_lbl(*s.else_block));
    } else {
      emit_term(ctx, "br i1 " + cond_i1 + ", label %" + then_lbl +
                         ", label %" + after_lbl);
    }
    // Store after_block in meta for then/else blocks (so they fall through)
    ctx.block_meta[s.then_block.value].break_label = std::nullopt;  // not a loop
    if (s.else_block)
      ctx.block_meta[s.else_block->value].break_label = std::nullopt;
    // after_block label is needed as fallthrough for then/else
    // We inject it into the meta as "after" by convention (handled in emit_function)
    (void)after_lbl;
  }

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const WhileStmt& s){
    const std::string cond_lbl = s.continue_target
        ? block_lbl(*s.continue_target)
        : fresh_lbl(ctx, "while.cond.");
    const std::string body_lbl = block_lbl(s.body_block);
    const std::string end_lbl  = s.break_target
        ? block_lbl(*s.break_target)
        : fresh_lbl(ctx, "while.end.");

    ctx.continue_redirect[s.body_block.value] = cond_lbl;

    TypeSpec cond_ts{};
    const std::string cond_v = emit_rval_id(ctx, s.cond, cond_ts);
    const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);
    emit_term(ctx, "br i1 " + cond_i1 + ", label %" + body_lbl + ", label %" + end_lbl);
  }

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const ForStmt& s){
    if (s.init) {
      TypeSpec ts{};
      emit_rval_id(ctx, *s.init, ts);
    }
    const std::string body_lbl = block_lbl(s.body_block);
    const std::string end_lbl  = s.break_target
        ? block_lbl(*s.break_target)
        : fresh_lbl(ctx, "for.end.");
    const std::string cond_lbl = "for.cond." + std::to_string(s.body_block.value);
    const std::string latch_lbl = "for.latch." + std::to_string(s.body_block.value);
    ctx.continue_redirect[s.body_block.value] = latch_lbl;

    emit_term(ctx, "br label %" + cond_lbl);
    emit_lbl(ctx, cond_lbl);
    if (s.cond) {
      TypeSpec cts{};
      std::string cv = emit_rval_id(ctx, *s.cond, cts);
      cv = to_bool(ctx, cv, cts);
      emit_term(ctx, "br i1 " + cv + ", label %" + body_lbl + ", label %" + end_lbl);
    } else {
      emit_term(ctx, "br label %" + body_lbl);
    }
    emit_lbl(ctx, latch_lbl);
    if (s.update) {
      TypeSpec uts{};
      (void)emit_rval_id(ctx, *s.update, uts);
    }
    emit_term(ctx, "br label %" + cond_lbl);
  }

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const DoWhileStmt& s){
    const std::string body_lbl = block_lbl(s.body_block);
    const std::string end_lbl  = s.break_target
        ? block_lbl(*s.break_target)
        : fresh_lbl(ctx, "dowhile.end.");
    const std::string cond_lbl = "dowhile.cond." + std::to_string(s.body_block.value);
    ctx.continue_redirect[s.body_block.value] = cond_lbl;
    emit_term(ctx, "br label %" + cond_lbl);
    emit_lbl(ctx, cond_lbl);
    TypeSpec cond_ts{};
    const std::string cond_v = emit_rval_id(ctx, s.cond, cond_ts);
    const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);
    emit_term(ctx, "br i1 " + cond_i1 + ", label %" + body_lbl + ", label %" + end_lbl);
  }

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const SwitchStmt& s){
    TypeSpec ts{};
    std::string val = emit_rval_id(ctx, s.cond, ts);
    // C requires integer promotion on the controlling expression.
    if (ts.ptr_level == 0 && ts.array_rank == 0 && is_any_int(ts.base)) {
      TypeBase promoted = integer_promote(ts.base);
      if (promoted != ts.base) {
        TypeSpec promoted_ts{}; promoted_ts.base = promoted;
        val = coerce(ctx, val, ts, promoted_ts);
        ts = promoted_ts;
      }
    }
    const std::string ty  = llvm_ty(ts);

    // Default label: use explicit default block if present, else the break (after-switch) block
    const std::string default_lbl = s.default_block
        ? block_lbl(*s.default_block)
        : (s.break_block ? block_lbl(*s.break_block) : fresh_lbl(ctx, "sw.end."));
    const std::string break_lbl = s.break_block
        ? block_lbl(*s.break_block)
        : default_lbl;

    ctx.block_meta[s.body_block.value].break_label = break_lbl;

    // Collect case values by scanning body block statements
    // All cases jump to body_block (they share the body, case markers are sequential)
    const Block* body_blk = nullptr;
    for (const auto& blk : ctx.fn->blocks) {
      if (blk.id.value == s.body_block.value) { body_blk = &blk; break; }
    }

    const std::string body_lbl = block_lbl(s.body_block);

    // Case ranges need comparison chains before the switch.
    // For each range [lo, hi], emit: if (val >= lo && val <= hi) goto case_block
    // This is done before the switch instruction so the switch only handles discrete cases.
    if (!s.case_range_blocks.empty()) {
      for (const auto& [lo, hi, bid] : s.case_range_blocks) {
        const std::string cmp_ge = is_signed_int(ts.base)
            ? "icmp sge" : "icmp uge";
        const std::string cmp_le = is_signed_int(ts.base)
            ? "icmp sle" : "icmp ule";
        const std::string t_ge = fresh_tmp(ctx);
        emit_instr(ctx, t_ge + " = " + cmp_ge + " " + ty + " " + val + ", " + std::to_string(lo));
        const std::string t_le = fresh_tmp(ctx);
        emit_instr(ctx, t_le + " = " + cmp_le + " " + ty + " " + val + ", " + std::to_string(hi));
        const std::string t_and = fresh_tmp(ctx);
        emit_instr(ctx, t_and + " = and i1 " + t_ge + ", " + t_le);
        const std::string next_lbl = fresh_lbl(ctx, "sw.range.next.");
        emit_term(ctx, "br i1 " + t_and + ", label %" + block_lbl(bid) + ", label %" + next_lbl);
        emit_lbl(ctx, next_lbl);
      }
    }

    std::string sw = "switch " + ty + " " + val + ", label %" + default_lbl + " [\n";
    if (!s.case_blocks.empty()) {
      // Use pre-collected case→block mappings (supports Duff's device / interleaved cases).
      for (const auto& [case_val, case_bid] : s.case_blocks) {
        sw += "    " + ty + " " + std::to_string(case_val) +
              ", label %" + block_lbl(case_bid) + "\n";
      }
    } else if (body_blk) {
      // Fallback: scan body block for inline CaseStmt markers (all map to body start).
      for (const auto& stmt : body_blk->stmts) {
        if (const auto* cs = std::get_if<CaseStmt>(&stmt.payload)) {
          sw += "    " + ty + " " + std::to_string(cs->value) +
                ", label %" + body_lbl + "\n";
        } else if (std::get_if<CaseRangeStmt>(&stmt.payload)) {
          // Handled by range chains above
        }
      }
    }
    sw += "  ]";
    emit_term(ctx, sw);
  }

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const GotoStmt& s){
    if (ctx.vla_stack_save_ptr && s.target.resolved_block.valid() &&
        s.target.resolved_block.value <= ctx.current_block_id) {
      need_llvm_stackrestore_ = true;
      emit_lir_op(ctx, lir::LirStackRestoreOp{*ctx.vla_stack_save_ptr});
    }
    if (s.target.resolved_block.valid()) {
      emit_term(ctx, "br label %" + block_lbl(s.target.resolved_block));
    } else {
      emit_term(ctx, "br label %ulbl_" + s.target.user_name);
    }
  }

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const IndirBrStmt& s){
    // Collect all user labels in this function as possible targets.
    std::vector<std::string> targets;
    for (const auto& bb : ctx.fn->blocks) {
      for (const auto& stmt : bb.stmts) {
        if (const auto* ls = std::get_if<LabelStmt>(&stmt.payload)) {
          targets.push_back("%ulbl_" + ls->name);
        }
      }
    }
    TypeSpec dummy_ts{};
    const std::string val = emit_rval_id(ctx, s.target, dummy_ts);
    if (!ctx.last_term) {
      ctx.cur_block().insts.push_back(
          lir::LirIndirectBrOp{val, std::move(targets)});
      ctx.last_term = true;
    }
  }

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const LabelStmt& s){
    if (ctx.last_term) {
      // We need a new basic block for the label
      emit_lbl(ctx, "ulbl_" + s.name);
    } else {
      emit_term(ctx, "br label %ulbl_" + s.name);
      emit_lbl(ctx, "ulbl_" + s.name);
    }
  }

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const BreakStmt& s){
    if (s.target) emit_term(ctx, "br label %" + block_lbl(*s.target));
  }

void HirEmitter::emit_stmt_impl(FnCtx& ctx, const ContinueStmt& s){
    if (!s.target) return;
    const auto it = ctx.continue_redirect.find(s.target->value);
    if (it != ctx.continue_redirect.end()) {
      emit_term(ctx, "br label %" + it->second);
      return;
    }
    emit_term(ctx, "br label %" + block_lbl(*s.target));
  }

void HirEmitter::emit_stmt_impl(FnCtx&, const CaseStmt&){}

void HirEmitter::emit_stmt_impl(FnCtx&, const CaseRangeStmt&){}

void HirEmitter::emit_stmt_impl(FnCtx&, const DefaultStmt&){}

std::unordered_set<uint32_t> HirEmitter::find_modified_params(const Function& fn){
    std::unordered_set<uint32_t> result;
    // Recursively check all expressions in the function body.
    std::function<void(ExprId)> scan = [&](ExprId id) {
      const Expr& e = get_expr(id);
      std::visit([&](const auto& p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, UnaryExpr>) {
          if (p.op == UnaryOp::PreInc || p.op == UnaryOp::PostInc ||
              p.op == UnaryOp::PreDec || p.op == UnaryOp::PostDec ||
              p.op == UnaryOp::AddrOf) {
            // If operand is a param, mark it
            const Expr& op_e = get_expr(p.operand);
            if (const auto* r = std::get_if<DeclRef>(&op_e.payload)) {
              if (r->param_index) result.insert(*r->param_index);
            }
          }
          scan(p.operand);
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
          // lhs assignment
          const Expr& lhs_e = get_expr(p.lhs);
          if (const auto* r = std::get_if<DeclRef>(&lhs_e.payload)) {
            if (r->param_index) result.insert(*r->param_index);
          }
          scan(p.lhs); scan(p.rhs);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
          scan(p.lhs); scan(p.rhs);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
          scan(p.callee);
          for (const auto& a : p.args) scan(a);
        } else if constexpr (std::is_same_v<T, TernaryExpr>) {
          scan(p.cond); scan(p.then_expr); scan(p.else_expr);
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
          scan(p.base); scan(p.index);
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
          scan(p.base);
        } else if constexpr (std::is_same_v<T, CastExpr>) {
          scan(p.expr);
        } else if constexpr (std::is_same_v<T, SizeofExpr>) {
          scan(p.expr);
        }
      }, e.payload);
    };
    for (const auto& blk : fn.blocks) {
      for (const auto& stmt : blk.stmts) {
        std::visit([&](const auto& s) {
          using S = std::decay_t<decltype(s)>;
          if constexpr (std::is_same_v<S, LocalDecl>) {
            if (s.init) scan(*s.init);
          } else if constexpr (std::is_same_v<S, ExprStmt>) {
            if (s.expr) scan(*s.expr);
          } else if constexpr (std::is_same_v<S, ReturnStmt>) {
            if (s.expr) scan(*s.expr);
          } else if constexpr (std::is_same_v<S, IfStmt>) {
            scan(s.cond);
          } else if constexpr (std::is_same_v<S, WhileStmt>) {
            scan(s.cond);
          } else if constexpr (std::is_same_v<S, ForStmt>) {
            if (s.init) scan(*s.init);
            if (s.cond) scan(*s.cond);
            if (s.update) scan(*s.update);
          } else if constexpr (std::is_same_v<S, SwitchStmt>) {
            scan(s.cond);
          }
        }, stmt.payload);
      }
    }
    return result;
  }

bool HirEmitter::fn_has_vla_locals(const Function& fn){
    for (const auto& blk : fn.blocks) {
      for (const auto& stmt : blk.stmts) {
        if (const auto* d = std::get_if<LocalDecl>(&stmt.payload)) {
          if (d->vla_size.has_value()) return true;
        }
      }
    }
    return false;
  }

void HirEmitter::hoist_allocas(FnCtx& ctx, const Function& fn){
    // Identify parameters that are modified (need spill slots)
    const std::unordered_set<uint32_t> modified_params = find_modified_params(fn);
    for (size_t i = 0; i < fn.params.size(); ++i) {
      if (!modified_params.count(static_cast<uint32_t>(i))) continue;
      const auto& param = fn.params[i];
      const std::string slot = "%lv.param." + sanitize_llvm_ident(param.name);
      const std::string pname = "%p." + sanitize_llvm_ident(param.name);
      // Store the spill slot under key (param_index + sentinel)
      ctx.param_slots[static_cast<uint32_t>(i) + 0x80000000u] = slot;
      ctx.alloca_insts.push_back(lir::LirRawLine{"  " + slot + " = alloca " + llvm_alloca_ty(param.type.spec)});
      ctx.alloca_insts.push_back(lir::LirRawLine{"  store " + llvm_ty(param.type.spec) + " " + pname + ", ptr " + slot});
    }

    std::unordered_map<std::string, int> name_count;
    for (const auto& blk : fn.blocks) {
      for (const auto& stmt : blk.stmts) {
        const auto* d = std::get_if<LocalDecl>(&stmt.payload);
        if (!d) continue;
        if (ctx.local_slots.count(d->id.value)) continue;
        // Dedup slot names for shadowing locals
        const int cnt = name_count[d->name]++;
        const std::string base = sanitize_llvm_ident(d->name);
        const std::string slot = cnt == 0
            ? "%lv." + base
            : "%lv." + base + "." + std::to_string(cnt);
        ctx.local_slots[d->id.value] = slot;
        ctx.local_types[d->id.value] = d->type.spec;
        ctx.local_is_vla[d->id.value] = d->vla_size.has_value();
        if (d->vla_size) {
          ctx.alloca_insts.push_back(lir::LirRawLine{"  " + slot + " = alloca ptr"});
        } else {
          const std::string alloca_ty = llvm_alloca_ty(d->type.spec);
          ctx.alloca_insts.push_back(lir::LirRawLine{"  " + slot + " = alloca " + alloca_ty});
          if (d->init &&
              (d->type.spec.array_rank > 0 ||
               (d->type.spec.ptr_level == 0 &&
                (d->type.spec.base == TB_STRUCT ||
                 d->type.spec.base == TB_UNION)))) {
            ctx.alloca_insts.push_back(lir::LirRawLine{"  store " + alloca_ty + " zeroinitializer, ptr " + slot});
          }
        }
      }
    }
  }

void HirEmitter::emit_function(const Function& fn){
    FnCtx ctx;
    ctx.fn = &fn;
    for (size_t i = 0; i < fn.params.size(); ++i) {
      if (fn.params[i].fn_ptr_sig) {
        ctx.param_fn_ptr_sigs[static_cast<uint32_t>(i)] = *fn.params[i].fn_ptr_sig;
      }
    }
    for (const auto& g : mod_.globals) {
      if (g.fn_ptr_sig) ctx.global_fn_ptr_sigs[g.id.value] = *g.fn_ptr_sig;
    }

    std::ostringstream sig_out;
    const std::string ret_ty = llvm_ret_ty(fn.return_type.spec);

    const bool void_param_list =
        fn.params.size() == 1 &&
        fn.params[0].type.spec.base == TB_VOID &&
        fn.params[0].type.spec.ptr_level == 0 &&
        fn.params[0].type.spec.array_rank == 0;

    if (fn.linkage.is_extern && fn.blocks.empty()) {
      const std::string decl_kw = fn.linkage.is_weak ? "declare extern_weak " : "declare ";
      sig_out << decl_kw << llvm_visibility(fn.linkage.visibility) << ret_ty << " "
              << llvm_global_sym(fn.name) << "(";
      for (size_t i = 0; i < fn.params.size(); ++i) {
        if (void_param_list) break;
        if (i) sig_out << ", ";
        sig_out << llvm_ty(fn.params[i].type.spec);
      }
      if (fn.attrs.variadic) {
        if (!fn.params.empty() && !void_param_list) sig_out << ", ";
        sig_out << "...";
      }
      sig_out << ")\n\n";
      lir::LirFunction lir_fn;
      lir_fn.name = quote_llvm_ident(fn.name);
      lir_fn.is_internal = fn.linkage.is_static;
      lir_fn.is_declaration = true;
      lir_fn.signature_text = sig_out.str();
      module_.functions.push_back(std::move(lir_fn));
      return;
    }

    // Emit specialization identity metadata as comments for template instantiations.
    if (!fn.template_origin.empty()) {
      sig_out << "; template-origin: " << fn.template_origin << "\n";
      if (!fn.spec_key.empty()) {
        sig_out << "; spec-key: " << fn.spec_key.canonical << "\n";
        spec_entries_.push_back({fn.spec_key.canonical, fn.template_origin, std::string(fn.name)});
      }
    }

    // Signature
    std::string fn_lk = fn.linkage.is_static ? "internal " : "";
    if (fn.linkage.is_weak && !fn.linkage.is_static) fn_lk = "weak ";
    sig_out << "define " << fn_lk << llvm_visibility(fn.linkage.visibility) << ret_ty << " "
            << llvm_global_sym(fn.name) << "(";
    for (size_t i = 0; i < fn.params.size(); ++i) {
      if (void_param_list) break;
      if (i) sig_out << ", ";
      const std::string pty = llvm_ty(fn.params[i].type.spec);
      const std::string pname = "%p." + sanitize_llvm_ident(fn.params[i].name);
      sig_out << pty << " " << pname;
      ctx.param_slots[static_cast<uint32_t>(i)] = pname;
    }
    if (fn.attrs.variadic) {
      if (!fn.params.empty()) sig_out << ", ";
      sig_out << "...";
    }
    sig_out << ")";
    // Emit function attributes.
    if (fn.attrs.no_inline) sig_out << " noinline";
    if (fn.attrs.always_inline) sig_out << " alwaysinline";
    sig_out << "\n";

    // Create entry block for alloca hoisting and entry statements.
    {
      lir::LirBlock entry_blk;
      entry_blk.id = lir::LirBlockId{0};
      entry_blk.label = "entry";
      ctx.lir_blocks.push_back(std::move(entry_blk));
      ctx.current_block_idx = 0;
    }

    // Alloca hoisting
    hoist_allocas(ctx, fn);
    if (fn_has_vla_locals(fn)) {
      need_llvm_stacksave_ = true;
      const std::string saved_sp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirStackSaveOp{saved_sp});
      ctx.vla_stack_save_ptr = saved_sp;
    }

    // Emit blocks in order (entry first, then rest)
    const Block* entry_blk = nullptr;
    std::vector<const Block*> ordered;
    for (const auto& blk : fn.blocks) {
      if (blk.id.value == fn.entry.value) { entry_blk = &blk; }
    }
    if (entry_blk) ordered.push_back(entry_blk);
    for (const auto& blk : fn.blocks) {
      if (blk.id.value != fn.entry.value) ordered.push_back(&blk);
    }

    for (size_t bi = 0; bi < ordered.size(); ++bi) {
      const Block* blk = ordered[bi];
      ctx.current_block_id = blk->id.value;

      // Emit block label (except for entry which uses "entry:")
      if (bi > 0) {
        emit_lbl(ctx, block_lbl(blk->id));
      }

      // Emit statements
      for (const auto& stmt : blk->stmts) {
        emit_stmt(ctx, stmt);
      }

      // HIR lowering must encode CFG edges explicitly.
      if (!ctx.last_term) {
        if (fn.return_type.spec.base == TB_VOID &&
            fn.return_type.spec.ptr_level == 0 &&
            fn.return_type.spec.array_rank == 0 &&
            !fn.return_type.spec.is_lvalue_ref &&
            !fn.return_type.spec.is_rvalue_ref) {
          emit_term(ctx, "ret void");
        } else if (ret_ty == "ptr") {
          emit_term(ctx, "ret ptr null");
        } else if (is_float_base(fn.return_type.spec.base) &&
                   fn.return_type.spec.ptr_level == 0 && fn.return_type.spec.array_rank == 0) {
          const std::string zero_val = fp_literal(fn.return_type.spec.base, 0.0);
          emit_term(ctx, "ret " + ret_ty + " " + zero_val);
        } else if (is_complex_base(fn.return_type.spec.base) ||
                   ((fn.return_type.spec.base == TB_STRUCT || fn.return_type.spec.base == TB_UNION) &&
                    fn.return_type.spec.ptr_level == 0 && fn.return_type.spec.array_rank == 0)) {
          emit_term(ctx, "ret " + ret_ty + " zeroinitializer");
        } else {
          emit_term(ctx, "ret " + ret_ty + " 0");
        }
      }
    }

    // Build LirFunction from accumulated blocks.
    lir::LirFunction lir_fn;
    lir_fn.name = quote_llvm_ident(fn.name);
    lir_fn.is_internal = fn.linkage.is_static;
    lir_fn.is_declaration = false;
    lir_fn.return_type = fn.return_type.spec;
    lir_fn.signature_text = sig_out.str();
    lir_fn.alloca_insts = std::move(ctx.alloca_insts);
    lir_fn.blocks = std::move(ctx.lir_blocks);
    module_.functions.push_back(std::move(lir_fn));
  }

}  // namespace tinyc2ll::codegen::llvm_backend
