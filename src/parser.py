from typing import Dict, List, Optional

from ast_nodes import *
from errors import CompileError
from lexer import Token, TokenType
class Parser:
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.i = 0
        self.struct_defs: Dict[str, StructDef] = {}
        self.anon_struct_idx = 0
        self.typedef_names: Dict[str, str] = {}
        self.enum_consts: Dict[str, int] = {}

    def cur(self) -> Token:
        return self.tokens[self.i]

    def peek(self, n: int = 1) -> Token:
        j = self.i + n
        if j >= len(self.tokens):
            return self.tokens[-1]
        return self.tokens[j]

    def advance(self) -> None:
        self.i += 1

    def error(self, msg: str) -> CompileError:
        t = self.cur()
        return CompileError(f"{t.line}:{t.col}: {msg} (got {t.text!r})")

    def eat(self, typ: TokenType) -> Token:
        t = self.cur()
        if t.typ != typ:
            raise self.error(f"expected {typ.name}")
        self.advance()
        return t

    def skip_type_qualifiers(self) -> None:
        """Skip const, volatile, restrict, inline, register, _Atomic, signed, __const, etc."""
        qual_types = {
            TokenType.KW_CONST, TokenType.KW_VOLATILE, TokenType.KW_RESTRICT,
            TokenType.KW_INLINE, TokenType.KW_REGISTER,
        }
        while self.cur().typ in qual_types:
            self.advance()

    def parse_type(self) -> str:
        # Skip leading qualifiers
        self.skip_type_qualifiers()
        match self.cur().typ:
            case TokenType.KW_INT:
                self.advance()
                # Consume trailing qualifiers/modifiers that may follow
                self.skip_type_qualifiers()
                return "int"
            case TokenType.KW_CHAR:
                self.advance()
                self.skip_type_qualifiers()
                return "char"
            case TokenType.KW_VOID:
                self.advance()
                self.skip_type_qualifiers()
                return "void"
            case TokenType.KW_LONG:
                self.advance()
                # long double
                if self.cur().typ == TokenType.KW_DOUBLE:
                    self.advance()
                    self.skip_type_qualifiers()
                    return "double"
                # long long, long int, etc.
                if self.cur().typ == TokenType.KW_LONG:
                    self.advance()
                if self.cur().typ == TokenType.KW_INT:
                    self.advance()
                if self.cur().typ == TokenType.KW_UNSIGNED:
                    self.advance()
                self.skip_type_qualifiers()
                return "int"
            case TokenType.KW_SHORT:
                self.advance()
                if self.cur().typ == TokenType.KW_INT:
                    self.advance()
                self.skip_type_qualifiers()
                return "int"
            case TokenType.KW_UNSIGNED:
                self.advance()
                # unsigned int, unsigned long, unsigned char, etc.
                if self.cur().typ in (
                    TokenType.KW_INT, TokenType.KW_LONG, TokenType.KW_SHORT, TokenType.KW_CHAR
                ):
                    inner = self.cur().text
                    self.advance()
                    if inner == "char":
                        if self.cur().typ == TokenType.KW_LONG:
                            self.advance()
                    if self.cur().typ == TokenType.KW_LONG:
                        self.advance()  # unsigned long long
                    if self.cur().typ == TokenType.KW_INT:
                        self.advance()
                self.skip_type_qualifiers()
                return "int"
            case TokenType.KW_SIGNED:
                self.advance()
                if self.cur().typ in (
                    TokenType.KW_INT, TokenType.KW_LONG, TokenType.KW_SHORT, TokenType.KW_CHAR
                ):
                    self.advance()
                    if self.cur().typ == TokenType.KW_LONG:
                        self.advance()
                    if self.cur().typ == TokenType.KW_INT:
                        self.advance()
                self.skip_type_qualifiers()
                return "int"
            case TokenType.KW_DOUBLE | TokenType.KW_FLOAT:
                self.advance()
                self.skip_type_qualifiers()
                return "double"
            case TokenType.KW_STRUCT | TokenType.KW_UNION:
                is_union = self.cur().typ == TokenType.KW_UNION
                self.advance()
                # Skip __attribute__ if any
                tag: Optional[str] = None
                if self.cur().typ == TokenType.ID:
                    tag = self.eat(TokenType.ID).text
                if self.cur().typ == TokenType.LBRACE:
                    self.advance()
                    fields: List[StructField] = []
                    while self.cur().typ != TokenType.RBRACE:
                        # Allow empty declarations like just a semicolon
                        if self.cur().typ == TokenType.SEMI:
                            self.advance()
                            continue
                        field_base = self.parse_type()
                        field_ptr = 0
                        while self.cur().typ == TokenType.STAR:
                            self.advance()
                            field_ptr += 1
                        # Function pointer field: skip it entirely
                        if self.cur().typ == TokenType.LPAREN:
                            # e.g. int (*fptr)(int) - skip entire declarator
                            depth = 0
                            while self.cur().typ != TokenType.SEMI or depth > 0:
                                if self.cur().typ == TokenType.LPAREN:
                                    depth += 1
                                elif self.cur().typ == TokenType.RPAREN:
                                    depth -= 1
                                elif self.cur().typ == TokenType.EOF:
                                    break
                                self.advance()
                            if self.cur().typ == TokenType.SEMI:
                                self.advance()
                            continue
                        # Anonymous struct/union field (no field name before ';')
                        if self.cur().typ == TokenType.SEMI and (
                            field_base.startswith("struct:") or field_base.startswith("union:")
                        ):
                            # Anonymous nested struct/union: inline its fields
                            anon_tag = field_base.split(":", 1)[1]
                            anon_def = self.struct_defs.get(anon_tag)
                            if anon_def is not None:
                                fields.extend(anon_def.fields)
                            self.advance()  # eat SEMI
                            continue
                        if self.cur().typ == TokenType.SEMI:
                            # Naked type with no field name - skip
                            self.advance()
                            continue
                        field_name = self.eat(TokenType.ID).text
                        # Handle array field: int arr[N] or arr[N][M] (multi-dim)
                        if self.cur().typ == TokenType.LBRACKET:
                            # Skip all array dimensions
                            while self.cur().typ == TokenType.LBRACKET:
                                self.advance()
                                # Skip expression inside brackets (may be complex like 256/8)
                                depth = 1
                                while self.cur().typ != TokenType.EOF and depth > 0:
                                    if self.cur().typ == TokenType.LBRACKET:
                                        depth += 1
                                    elif self.cur().typ == TokenType.RBRACKET:
                                        depth -= 1
                                    if depth > 0:
                                        self.advance()
                                if self.cur().typ == TokenType.RBRACKET:
                                    self.advance()
                            # Store as array field - just use base type for simplicity
                            fields.append(StructField(field_base + ("*" * field_ptr), field_name))
                            # Handle multiple names: int a[4], b[4];
                            while self.cur().typ == TokenType.COMMA:
                                self.advance()
                                fptr2 = 0
                                while self.cur().typ == TokenType.STAR:
                                    self.advance()
                                    fptr2 += 1
                                if self.cur().typ == TokenType.ID:
                                    fname2 = self.eat(TokenType.ID).text
                                    while self.cur().typ == TokenType.LBRACKET:
                                        self.advance()
                                        while self.cur().typ not in (TokenType.RBRACKET, TokenType.EOF):
                                            self.advance()
                                        if self.cur().typ == TokenType.RBRACKET:
                                            self.advance()
                                    fields.append(StructField(field_base + ("*" * fptr2), fname2))
                            self.eat(TokenType.SEMI)
                            continue
                        # Handle bit fields: int x : N
                        if self.cur().typ == TokenType.COLON:
                            self.advance()
                            # Skip bit field width (could be complex expression)
                            while self.cur().typ not in (TokenType.SEMI, TokenType.COMMA, TokenType.EOF):
                                self.advance()
                            fields.append(StructField(field_base + ("*" * field_ptr), field_name))
                            # Handle multiple bit fields: int a:1, b:2;
                            while self.cur().typ == TokenType.COMMA:
                                self.advance()
                                fptr2 = 0
                                while self.cur().typ == TokenType.STAR:
                                    self.advance()
                                    fptr2 += 1
                                if self.cur().typ == TokenType.ID:
                                    fname2 = self.eat(TokenType.ID).text
                                    if self.cur().typ == TokenType.COLON:
                                        self.advance()
                                        while self.cur().typ not in (TokenType.SEMI, TokenType.COMMA, TokenType.EOF):
                                            self.advance()
                                    fields.append(StructField(field_base + ("*" * fptr2), fname2))
                            self.eat(TokenType.SEMI)
                            continue
                        fields.append(
                            StructField(field_base + ("*" * field_ptr), field_name)
                        )
                        # Handle multiple field names: int a, b, c;
                        while self.cur().typ == TokenType.COMMA:
                            self.advance()
                            fptr2 = 0
                            while self.cur().typ == TokenType.STAR:
                                self.advance()
                                fptr2 += 1
                            if self.cur().typ == TokenType.LPAREN:
                                # function pointer field - skip
                                depth = 0
                                while self.cur().typ != TokenType.SEMI or depth > 0:
                                    if self.cur().typ == TokenType.LPAREN:
                                        depth += 1
                                    elif self.cur().typ == TokenType.RPAREN:
                                        depth -= 1
                                    elif self.cur().typ == TokenType.EOF:
                                        break
                                    self.advance()
                                break
                            if self.cur().typ == TokenType.ID:
                                fname2 = self.eat(TokenType.ID).text
                                # Handle array suffix
                                while self.cur().typ == TokenType.LBRACKET:
                                    self.advance()
                                    while self.cur().typ not in (TokenType.RBRACKET, TokenType.EOF):
                                        self.advance()
                                    if self.cur().typ == TokenType.RBRACKET:
                                        self.advance()
                                # Handle bit field
                                if self.cur().typ == TokenType.COLON:
                                    self.advance()
                                    while self.cur().typ not in (TokenType.SEMI, TokenType.COMMA, TokenType.EOF):
                                        self.advance()
                                fields.append(StructField(field_base + ("*" * fptr2), fname2))
                        self.eat(TokenType.SEMI)
                    self.eat(TokenType.RBRACE)
                    if tag is None:
                        tag = f"__anon{self.anon_struct_idx}"
                        self.anon_struct_idx += 1
                    prefix = "union" if is_union else "struct"
                    self.struct_defs[tag] = StructDef(tag, fields)
                    return f"struct:{tag}"
                if tag is None:
                    # Forward reference without body - can't do much, return placeholder
                    tag = f"__anon{self.anon_struct_idx}"
                    self.anon_struct_idx += 1
                    return f"struct:{tag}"
                return f"struct:{tag}"
            case TokenType.KW_ENUM:
                self.advance()
                # Parse enum E { A, B=5, C }
                enum_tag: Optional[str] = None
                if self.cur().typ == TokenType.ID:
                    enum_tag = self.eat(TokenType.ID).text
                if self.cur().typ == TokenType.LBRACE:
                    self.advance()
                    val = 0
                    while self.cur().typ != TokenType.RBRACE:
                        if self.cur().typ == TokenType.EOF:
                            break
                        name = self.eat(TokenType.ID).text
                        if self.cur().typ == TokenType.ASSIGN:
                            self.advance()
                            val = self._parse_const_int()
                        # Register as enum constant (treat as global int)
                        self.enum_consts[name] = val
                        val += 1
                        if self.cur().typ == TokenType.COMMA:
                            self.advance()
                    if self.cur().typ == TokenType.RBRACE:
                        self.eat(TokenType.RBRACE)
                # enum type → int
                return "int"
            case TokenType.ID:
                t = self.cur()
                ty = self.typedef_names.get(t.text)
                if ty is None:
                    # Be lenient with unknown builtin/system-header typedef-like names.
                    if t.text.startswith("__"):
                        self.advance()
                        self.skip_type_qualifiers()
                        return "int"
                    raise self.error("expected type")
                self.advance()
                self.skip_type_qualifiers()
                return ty
            case _:
                raise self.error("expected type")

    def _parse_const_int(self) -> int:
        """Parse a simple integer constant expression (for enum values)."""
        negative = False
        if self.cur().typ == TokenType.MINUS:
            negative = True
            self.advance()
        if self.cur().typ == TokenType.LPAREN:
            self.advance()
            v = self._parse_const_int()
            if self.cur().typ == TokenType.RPAREN:
                self.advance()
            return -v if negative else v
        if self.cur().typ == TokenType.NUM:
            v = int(self.eat(TokenType.NUM).text)
            # Handle binary ops like A+1, A-1
            while self.cur().typ in (TokenType.PLUS, TokenType.MINUS, TokenType.STAR,
                                     TokenType.SLASH, TokenType.LSHIFT, TokenType.RSHIFT,
                                     TokenType.AMP, TokenType.PIPE, TokenType.CARET):
                op = self.cur().text
                self.advance()
                rhs = self._parse_const_int()
                if op == "+": v += rhs
                elif op == "-": v -= rhs
                elif op == "*": v *= rhs
                elif op == "/": v = int(v / rhs) if rhs else 0
                elif op == "<<": v <<= rhs
                elif op == ">>": v >>= rhs
                elif op == "&": v &= rhs
                elif op == "|": v |= rhs
                elif op == "^": v ^= rhs
            return -v if negative else v
        if self.cur().typ == TokenType.ID:
            name = self.eat(TokenType.ID).text
            v = self.enum_consts.get(name, 0)
            while self.cur().typ in (TokenType.PLUS, TokenType.MINUS):
                op = self.cur().text
                self.advance()
                rhs = self._parse_const_int()
                if op == "+": v += rhs
                else: v -= rhs
            return -v if negative else v
        return 0

    def is_type_start(self) -> bool:
        type_starts = {
            TokenType.KW_INT, TokenType.KW_VOID, TokenType.KW_STRUCT, TokenType.KW_CHAR,
            TokenType.KW_LONG, TokenType.KW_SHORT, TokenType.KW_UNSIGNED, TokenType.KW_SIGNED,
            TokenType.KW_DOUBLE, TokenType.KW_FLOAT, TokenType.KW_ENUM, TokenType.KW_UNION,
            TokenType.KW_CONST, TokenType.KW_VOLATILE, TokenType.KW_RESTRICT,
            TokenType.KW_INLINE, TokenType.KW_REGISTER,
        }
        if self.cur().typ in type_starts:
            return True
        return self.cur().typ == TokenType.ID and self.cur().text in self.typedef_names

    def parse_sizeof_type(self) -> str:
        base = self.parse_type()
        ptr_level = 0
        while self.cur().typ == TokenType.STAR:
            self.advance()
            ptr_level += 1
        return base + ("*" * ptr_level)

    def parse_params(self) -> List[Param]:
        if self.cur().typ == TokenType.RPAREN:
            return []

        if self.cur().typ == TokenType.KW_VOID and self.peek().typ == TokenType.RPAREN:
            self.advance()
            return []

        params: List[Param] = []
        while True:
            # Handle variadic ...
            if self.cur().typ == TokenType.ELLIPSIS:
                self.advance()
                params.append(Param("...", None))
                break
            if not self.is_type_start():
                break
            typ = self.parse_type()
            ptr_level = 0
            # Consume stars and qualifiers interleaved: char * const * restrict
            _star_quals = {
                TokenType.STAR, TokenType.KW_CONST, TokenType.KW_VOLATILE,
                TokenType.KW_RESTRICT, TokenType.KW_INLINE, TokenType.KW_REGISTER,
            }
            while self.cur().typ in _star_quals:
                if self.cur().typ == TokenType.STAR:
                    ptr_level += 1
                self.advance()
            typ = typ + ("*" * ptr_level)
            # Skip function pointer params like int (*)(...)
            if self.cur().typ == TokenType.LPAREN:
                # skip the parenthesized declarator
                depth = 0
                while True:
                    if self.cur().typ == TokenType.LPAREN:
                        depth += 1
                    elif self.cur().typ == TokenType.RPAREN:
                        depth -= 1
                        if depth == 0:
                            self.advance()
                            break
                    elif self.cur().typ == TokenType.EOF:
                        break
                    self.advance()
                # Skip return type params if present: (*)(int, int)
                if self.cur().typ == TokenType.LPAREN:
                    depth = 0
                    while True:
                        if self.cur().typ == TokenType.LPAREN:
                            depth += 1
                        elif self.cur().typ == TokenType.RPAREN:
                            depth -= 1
                            if depth == 0:
                                self.advance()
                                break
                        elif self.cur().typ == TokenType.EOF:
                            break
                        self.advance()
                params.append(Param("ptr", None))
            else:
                # Handle array param: int x[] or int x[100]
                name: Optional[str] = None
                if self.cur().typ == TokenType.ID:
                    name = self.eat(TokenType.ID).text
                    self.skip_type_qualifiers()
                if self.cur().typ == TokenType.LBRACKET:
                    self.advance()
                    if self.cur().typ == TokenType.NUM:
                        self.advance()
                    self.eat(TokenType.RBRACKET)
                    # array param becomes pointer
                    if not typ.endswith("*"):
                        typ = typ + "*"
                params.append(Param(typ, name))
            if self.cur().typ != TokenType.COMMA:
                break
            self.advance()
        return params

    def parse_program(self) -> Program:
        decls: List[FunctionDecl] = []
        globals_: List[GlobalVar] = []
        funcs: List[Function] = []
        while self.cur().typ != TokenType.EOF:
            ext = self.parse_external()
            if ext is None:
                continue
            if isinstance(ext, FunctionDecl):
                decls.append(ext)
            elif isinstance(ext, list):
                globals_.extend(ext)
            else:
                funcs.append(ext)
        self.eat(TokenType.EOF)
        return Program(list(self.struct_defs.values()), decls, globals_, funcs, dict(self.enum_consts))

    def skip_init_braces(self) -> None:
        """Skip a brace-enclosed initializer like {1, 2, {3, 4}}."""
        if self.cur().typ != TokenType.LBRACE:
            return
        self.advance()
        depth = 1
        while self.cur().typ != TokenType.EOF:
            if self.cur().typ == TokenType.LBRACE:
                depth += 1
            elif self.cur().typ == TokenType.RBRACE:
                depth -= 1
                if depth == 0:
                    self.advance()
                    return
            self.advance()

    def parse_global_declarator(
        self, base_type: str
    ) -> Optional[GlobalVar]:
        """Parse one global variable declarator (name + optional array/init)."""
        # Handle function pointer: (*name)(...)
        if self.cur().typ == TokenType.LPAREN:
            # Skip the entire declarator + optional initializer until COMMA or SEMI
            depth = 0
            while self.cur().typ != TokenType.EOF:
                if self.cur().typ == TokenType.LPAREN:
                    depth += 1
                elif self.cur().typ == TokenType.RPAREN:
                    depth -= 1
                elif self.cur().typ in (TokenType.COMMA, TokenType.SEMI) and depth == 0:
                    break
                self.advance()
            return None

        ptr_level = 0
        while self.cur().typ == TokenType.STAR:
            self.advance()
            ptr_level += 1

        if self.cur().typ != TokenType.ID:
            return None
        name = self.eat(TokenType.ID).text

        size: Optional[int] = None
        if self.cur().typ == TokenType.LBRACKET:
            self.advance()
            if self.cur().typ == TokenType.NUM:
                size = int(self.eat(TokenType.NUM).text)
            # skip variable-length or empty: int a[]
            if self.cur().typ == TokenType.RBRACKET:
                self.eat(TokenType.RBRACKET)
            # May have [N][M] multi-dim - just skip extra dims
            while self.cur().typ == TokenType.LBRACKET:
                self.advance()
                while self.cur().typ not in (TokenType.RBRACKET, TokenType.EOF):
                    self.advance()
                if self.cur().typ == TokenType.RBRACKET:
                    self.advance()

        init: Optional[Node] = None
        if self.cur().typ == TokenType.ASSIGN:
            self.advance()
            if self.cur().typ == TokenType.LBRACE:
                # Array/struct initializer - skip for now
                self.skip_init_braces()
            else:
                try:
                    init = self.parse_expr()
                except CompileError:
                    init = None
        return GlobalVar(name, base_type, ptr_level, size, init)

    def parse_external(self) -> FunctionDecl | list[GlobalVar] | Function | None:
        is_typedef = False
        if self.cur().typ == TokenType.KW_TYPEDEF:
            is_typedef = True
            self.advance()
        # Skip storage class specifiers (may be multiple)
        while self.cur().typ in (
            TokenType.KW_EXTERN, TokenType.KW_STATIC, TokenType.KW_INLINE,
            TokenType.KW_REGISTER,
        ):
            self.advance()

        # Handle lone semicolons (e.g., after struct/enum definitions)
        if self.cur().typ == TokenType.SEMI:
            self.advance()
            return None

        base_type = self.parse_type()

        if is_typedef:
            # Handle typedef struct { ... } Name; or typedef int Name, *PName;
            # After parse_type, we may have:
            # - a name directly: typedef int myint;
            # - a function pointer: typedef int (*fn)(int);
            if self.cur().typ == TokenType.LPAREN:
                # typedef with function pointer: typedef int (*fn)(int);
                # Skip until SEMI
                while self.cur().typ not in (TokenType.SEMI, TokenType.EOF):
                    self.advance()
                if self.cur().typ == TokenType.SEMI:
                    self.advance()
                return None
            ptr = 0
            while self.cur().typ == TokenType.STAR:
                self.advance()
                ptr += 1
            if self.cur().typ == TokenType.ID:
                alias = self.eat(TokenType.ID).text
                self.typedef_names[alias] = base_type + ("*" * ptr)
                # Handle array typedef: typedef int arr[10]
                if self.cur().typ == TokenType.LBRACKET:
                    while self.cur().typ not in (TokenType.SEMI, TokenType.EOF):
                        self.advance()
                while self.cur().typ == TokenType.COMMA:
                    self.advance()
                    ptr2 = 0
                    while self.cur().typ == TokenType.STAR:
                        self.advance()
                        ptr2 += 1
                    if self.cur().typ == TokenType.ID:
                        alias2 = self.eat(TokenType.ID).text
                        self.typedef_names[alias2] = base_type + ("*" * ptr2)
                    # Handle array typedef
                    if self.cur().typ == TokenType.LBRACKET:
                        while self.cur().typ not in (TokenType.SEMI, TokenType.COMMA, TokenType.EOF):
                            self.advance()
            if self.cur().typ == TokenType.SEMI:
                self.advance()
            return None

        # After parse_type, handle:
        # - function pointer global: int (*name)(params) = ...;
        # - regular variable or function declaration/definition
        ret_ptr_level = 0
        while self.cur().typ == TokenType.STAR:
            self.advance()
            ret_ptr_level += 1
        ret_type = base_type + ("*" * ret_ptr_level)

        # Handle function pointer global variable: int (*name)(...) [= val];
        if self.cur().typ == TokenType.LPAREN and self.peek().typ == TokenType.STAR:
            # Skip function pointer variable declaration
            while self.cur().typ not in (TokenType.SEMI, TokenType.EOF):
                t = self.cur()
                self.advance()
                if t.typ == TokenType.LBRACE:
                    # This is actually a function definition masquerading - skip
                    depth = 1
                    while self.cur().typ != TokenType.EOF and depth > 0:
                        if self.cur().typ == TokenType.LBRACE:
                            depth += 1
                        elif self.cur().typ == TokenType.RBRACE:
                            depth -= 1
                        self.advance()
                    return None
            if self.cur().typ == TokenType.SEMI:
                self.advance()
            return None

        if self.cur().typ != TokenType.ID:
            # Can't parse - skip until next SEMI or EOF
            while self.cur().typ not in (TokenType.SEMI, TokenType.EOF):
                self.advance()
            if self.cur().typ == TokenType.SEMI:
                self.advance()
            return None

        name = self.eat(TokenType.ID).text

        if self.cur().typ != TokenType.LPAREN:
            # Global variable declaration
            decls: List[GlobalVar] = []

            ptr_here = ret_ptr_level
            size: Optional[int] = None
            if self.cur().typ == TokenType.LBRACKET:
                self.advance()
                if self.cur().typ == TokenType.NUM:
                    size = int(self.eat(TokenType.NUM).text)
                if self.cur().typ == TokenType.RBRACKET:
                    self.eat(TokenType.RBRACKET)
                # Multi-dimensional: skip
                while self.cur().typ == TokenType.LBRACKET:
                    self.advance()
                    while self.cur().typ not in (TokenType.RBRACKET, TokenType.EOF):
                        self.advance()
                    if self.cur().typ == TokenType.RBRACKET:
                        self.advance()
            init: Optional[Node] = None
            if self.cur().typ == TokenType.ASSIGN:
                self.advance()
                if self.cur().typ == TokenType.LBRACE:
                    self.skip_init_braces()
                else:
                    try:
                        init = self.parse_expr()
                    except CompileError:
                        init = None
            if ret_type != "void":
                decls.append(GlobalVar(name, base_type, ptr_here, size, init))
            while self.cur().typ == TokenType.COMMA:
                self.advance()
                g_ptr_level = 0
                while self.cur().typ == TokenType.STAR:
                    self.advance()
                    g_ptr_level += 1
                if self.cur().typ == TokenType.ID:
                    gname = self.eat(TokenType.ID).text
                    gsize: Optional[int] = None
                    if self.cur().typ == TokenType.LBRACKET:
                        self.advance()
                        if self.cur().typ == TokenType.NUM:
                            gsize = int(self.eat(TokenType.NUM).text)
                        if self.cur().typ == TokenType.RBRACKET:
                            self.eat(TokenType.RBRACKET)
                    ginit: Optional[Node] = None
                    if self.cur().typ == TokenType.ASSIGN:
                        self.advance()
                        if self.cur().typ == TokenType.LBRACE:
                            self.skip_init_braces()
                        else:
                            try:
                                ginit = self.parse_expr()
                            except CompileError:
                                ginit = None
                    decls.append(GlobalVar(gname, base_type, g_ptr_level, gsize, ginit))
            self.eat(TokenType.SEMI)
            return decls if decls else None

        self.eat(TokenType.LPAREN)
        params = self.parse_params()
        self.eat(TokenType.RPAREN)

        if self.cur().typ == TokenType.SEMI:
            self.advance()
            return FunctionDecl(ret_type, name, params)

        self.eat(TokenType.LBRACE)
        body: List[Node] = []
        while self.cur().typ != TokenType.RBRACE:
            body.append(self.parse_stmt())
        self.eat(TokenType.RBRACE)
        return Function(ret_type, name, params, body)

    def parse_stmt(self) -> Node:
        # Handle typedef inside function body - register the alias and skip
        if self.cur().typ == TokenType.KW_TYPEDEF:
            self.advance()
            # Skip storage class specifiers
            while self.cur().typ in (TokenType.KW_EXTERN, TokenType.KW_STATIC, TokenType.KW_INLINE, TokenType.KW_REGISTER):
                self.advance()
            if self.is_type_start():
                base_type = self.parse_type()
                # Handle function pointer typedef: typedef int (*fn)(int);
                if self.cur().typ == TokenType.LPAREN:
                    while self.cur().typ not in (TokenType.SEMI, TokenType.EOF):
                        self.advance()
                    if self.cur().typ == TokenType.SEMI:
                        self.advance()
                    return EmptyStmt()
                ptr = 0
                while self.cur().typ == TokenType.STAR:
                    self.advance()
                    ptr += 1
                if self.cur().typ == TokenType.ID:
                    alias = self.eat(TokenType.ID).text
                    self.typedef_names[alias] = base_type + ("*" * ptr)
                    if self.cur().typ == TokenType.LBRACKET:
                        while self.cur().typ not in (TokenType.SEMI, TokenType.EOF):
                            self.advance()
                    while self.cur().typ == TokenType.COMMA:
                        self.advance()
                        ptr2 = 0
                        while self.cur().typ == TokenType.STAR:
                            self.advance()
                            ptr2 += 1
                        if self.cur().typ == TokenType.ID:
                            alias2 = self.eat(TokenType.ID).text
                            self.typedef_names[alias2] = base_type + ("*" * ptr2)
                if self.cur().typ == TokenType.SEMI:
                    self.advance()
            else:
                while self.cur().typ not in (TokenType.SEMI, TokenType.EOF):
                    self.advance()
                if self.cur().typ == TokenType.SEMI:
                    self.advance()
            return EmptyStmt()
        # Skip storage class specifiers at statement level (static, extern, register)
        if self.cur().typ in (TokenType.KW_STATIC, TokenType.KW_EXTERN, TokenType.KW_REGISTER):
            self.advance()
        match self.cur().typ:
            case _ if self.is_type_start():
                base_type = self.parse_type()
                decls: List[Node] = []
                while True:
                    ptr_level = 0
                    while self.cur().typ == TokenType.STAR:
                        self.advance()
                        ptr_level += 1

                    # Skip function pointer declarations: int (*fn)(int)
                    if self.cur().typ == TokenType.LPAREN:
                        depth = 0
                        while True:
                            if self.cur().typ == TokenType.LPAREN:
                                depth += 1
                            elif self.cur().typ == TokenType.RPAREN:
                                depth -= 1
                                if depth == 0:
                                    self.advance()
                                    break
                            elif self.cur().typ == TokenType.EOF:
                                break
                            self.advance()
                        # Skip optional second parens (parameter list of function ptr)
                        if self.cur().typ == TokenType.LPAREN:
                            depth = 0
                            while True:
                                if self.cur().typ == TokenType.LPAREN:
                                    depth += 1
                                elif self.cur().typ == TokenType.RPAREN:
                                    depth -= 1
                                    if depth == 0:
                                        self.advance()
                                        break
                                elif self.cur().typ == TokenType.EOF:
                                    break
                                self.advance()
                        # Skip initializer if any
                        if self.cur().typ == TokenType.ASSIGN:
                            self.advance()
                            self.parse_expr()
                        # Treat as a void* variable (skipped in IR)
                        if self.cur().typ != TokenType.COMMA:
                            break
                        self.advance()
                        continue

                    if self.cur().typ != TokenType.ID:
                        break

                    # void ptr: void *p - treat as "void*"
                    if base_type == "void" and ptr_level == 0:
                        # skip this declaration
                        if self.cur().typ != TokenType.COMMA:
                            break
                        self.advance()
                        continue

                    name = self.eat(TokenType.ID).text
                    size: Optional[int] = None
                    if self.cur().typ == TokenType.LBRACKET:
                        self.advance()
                        if self.cur().typ == TokenType.NUM:
                            size = int(self.eat(TokenType.NUM).text)
                        else:
                            size = 1  # int a[] - default to 1
                        self.eat(TokenType.RBRACKET)
                        # Skip extra dimensions [M] - just ignore
                        while self.cur().typ == TokenType.LBRACKET:
                            self.advance()
                            while self.cur().typ not in (TokenType.RBRACKET, TokenType.EOF):
                                self.advance()
                            if self.cur().typ == TokenType.RBRACKET:
                                self.advance()
                    init: Optional[Node] = None
                    if self.cur().typ == TokenType.ASSIGN:
                        self.advance()
                        if self.cur().typ == TokenType.LBRACE:
                            # Skip array/struct initializer
                            self.skip_init_braces()
                        else:
                            init = self.parse_expr()
                    decls.append(Decl(name, base_type, ptr_level, size, init))
                    if self.cur().typ != TokenType.COMMA:
                        break
                    self.advance()
                self.eat(TokenType.SEMI)
                if not decls:
                    return EmptyStmt()
                if len(decls) == 1:
                    return decls[0]
                return Block(decls)
            case TokenType.LBRACE:
                self.advance()
                body: List[Node] = []
                while self.cur().typ != TokenType.RBRACE:
                    body.append(self.parse_stmt())
                self.eat(TokenType.RBRACE)
                return Block(body)
            case TokenType.KW_IF:
                self.advance()
                self.eat(TokenType.LPAREN)
                cond = self.parse_expr()
                self.eat(TokenType.RPAREN)
                then_stmt = self.parse_stmt()
                else_stmt: Optional[Node] = None
                if self.cur().typ == TokenType.KW_ELSE:
                    self.advance()
                    else_stmt = self.parse_stmt()
                return If(cond, then_stmt, else_stmt)
            case TokenType.KW_WHILE:
                self.advance()
                self.eat(TokenType.LPAREN)
                cond = self.parse_expr()
                self.eat(TokenType.RPAREN)
                body = self.parse_stmt()
                return While(cond, body)
            case TokenType.KW_FOR:
                self.advance()
                self.eat(TokenType.LPAREN)
                init: Optional[Node] = None
                # Handle storage class specifiers in for-init
                while self.cur().typ in (TokenType.KW_STATIC, TokenType.KW_EXTERN, TokenType.KW_REGISTER):
                    self.advance()
                if self.cur().typ != TokenType.SEMI:
                    if self.is_type_start():
                        # for (int i = 0; ...) style init-declaration
                        init = self.parse_stmt()
                        # parse_stmt already consumed the SEMI, so skip the extra eat below
                        # But we need to re-check: parse_stmt returns a Decl and eats SEMI
                        # We already consumed the first SEMI as part of the decl
                        cond_: Optional[Node] = None
                        if self.cur().typ != TokenType.SEMI:
                            cond_ = self.parse_expr()
                        self.eat(TokenType.SEMI)
                        post_: Optional[Node] = None
                        if self.cur().typ != TokenType.RPAREN:
                            post_ = self.parse_expr()
                            if self.cur().typ == TokenType.COMMA:
                                post_stmts_ = [ExprStmt(post_)]
                                while self.cur().typ == TokenType.COMMA:
                                    self.advance()
                                    post_stmts_.append(ExprStmt(self.parse_expr()))
                                post_ = Block(post_stmts_)
                        self.eat(TokenType.RPAREN)
                        body = self.parse_stmt()
                        return For(init, cond_, post_, body)
                    else:
                        init = self.parse_expr()
                        # Handle comma expressions in for-init: for(a=0,b=0; ...)
                        if self.cur().typ == TokenType.COMMA:
                            init_stmts = [ExprStmt(init)]
                            while self.cur().typ == TokenType.COMMA:
                                self.advance()
                                init_stmts.append(ExprStmt(self.parse_expr()))
                            init = Block(init_stmts)
                self.eat(TokenType.SEMI)
                cond: Optional[Node] = None
                if self.cur().typ != TokenType.SEMI:
                    cond = self.parse_expr()
                self.eat(TokenType.SEMI)
                post: Optional[Node] = None
                if self.cur().typ != TokenType.RPAREN:
                    post = self.parse_expr()
                    # Handle comma expressions in for-post: for(;;f++,g++)
                    if self.cur().typ == TokenType.COMMA:
                        post_stmts = [ExprStmt(post)]
                        while self.cur().typ == TokenType.COMMA:
                            self.advance()
                            post_stmts.append(ExprStmt(self.parse_expr()))
                        post = Block(post_stmts)
                self.eat(TokenType.RPAREN)
                body = self.parse_stmt()
                return For(init, cond, post, body)
            case TokenType.KW_DO:
                self.advance()
                body = self.parse_stmt()
                self.eat(TokenType.KW_WHILE)
                self.eat(TokenType.LPAREN)
                cond = self.parse_expr()
                self.eat(TokenType.RPAREN)
                self.eat(TokenType.SEMI)
                return DoWhile(body, cond)
            case TokenType.KW_SWITCH:
                self.advance()
                self.eat(TokenType.LPAREN)
                expr = self.parse_expr()
                self.eat(TokenType.RPAREN)
                body = self.parse_stmt()
                return Switch(expr, body)
            case TokenType.KW_CASE:
                self.advance()
                value = self.parse_expr()
                self.eat(TokenType.COLON)
                stmt = self.parse_stmt()
                return Case(value, stmt)
            case TokenType.KW_DEFAULT:
                self.advance()
                self.eat(TokenType.COLON)
                stmt = self.parse_stmt()
                return Default(stmt)
            case TokenType.KW_BREAK:
                self.advance()
                self.eat(TokenType.SEMI)
                return Break()
            case TokenType.KW_CONTINUE:
                self.advance()
                self.eat(TokenType.SEMI)
                return Continue()
            case TokenType.KW_GOTO:
                self.advance()
                name = self.eat(TokenType.ID).text
                self.eat(TokenType.SEMI)
                return Goto(name)
            case TokenType.SEMI:
                self.advance()
                return EmptyStmt()
            case TokenType.KW_RETURN:
                self.advance()
                expr: Optional[Node] = None
                if self.cur().typ != TokenType.SEMI:
                    expr = self.parse_expr()
                self.eat(TokenType.SEMI)
                return Return(expr)
            case TokenType.ID if self.peek().typ == TokenType.ASSIGN:
                name = self.eat(TokenType.ID).text
                self.eat(TokenType.ASSIGN)
                expr = self.parse_expr()
                self.eat(TokenType.SEMI)
                return Assign(name, expr)
            case TokenType.ID if self.peek().typ == TokenType.COLON:
                name = self.eat(TokenType.ID).text
                self.eat(TokenType.COLON)
                stmt = self.parse_stmt()
                return LabelStmt(name, stmt)
            case _:
                expr = self.parse_expr()
                self.eat(TokenType.SEMI)
                return ExprStmt(expr)

    def parse_expr(self) -> Node:
        return self.parse_assignment()

    def parse_assignment(self) -> Node:
        lhs = self.parse_conditional()
        assign_ops = {
            TokenType.ASSIGN: "=",
            TokenType.PLUSEQ: "+=",
            TokenType.MINUSEQ: "-=",
            TokenType.STAREQ: "*=",
            TokenType.SLASHEQ: "/=",
            TokenType.PERCENTEQ: "%=",
            TokenType.AMPEQ: "&=",
            TokenType.PIPEEQ: "|=",
            TokenType.CARETEQ: "^=",
            TokenType.LSHIFTEQ: "<<=",
            TokenType.RSHIFTEQ: ">>=",
        }
        op = assign_ops.get(self.cur().typ)
        if op is None:
            return lhs
        self.advance()
        rhs = self.parse_assignment()
        if not (
            isinstance(lhs, (Var, Index, Member))
            or (isinstance(lhs, UnaryOp) and lhs.op == "*")
        ):
            raise self.error("left-hand side of assignment must be a variable")
        return AssignExpr(lhs, op, rhs)

    def parse_conditional(self) -> Node:
        node = self.parse_logical_or()
        if self.cur().typ != TokenType.QUESTION:
            return node
        self.advance()
        then_expr = self.parse_expr()
        self.eat(TokenType.COLON)
        else_expr = self.parse_conditional()
        return TernaryOp(node, then_expr, else_expr)

    def parse_logical_or(self) -> Node:
        node = self.parse_logical_and()
        while self.cur().typ == TokenType.PIPEPIPE:
            self.advance()
            node = BinOp("||", node, self.parse_logical_and())
        return node

    def parse_logical_and(self) -> Node:
        node = self.parse_bit_or()
        while self.cur().typ == TokenType.AMPAMP:
            self.advance()
            node = BinOp("&&", node, self.parse_bit_or())
        return node

    def parse_bit_or(self) -> Node:
        node = self.parse_bit_xor()
        while self.cur().typ == TokenType.PIPE:
            self.advance()
            node = BinOp("|", node, self.parse_bit_xor())
        return node

    def parse_bit_xor(self) -> Node:
        node = self.parse_bit_and()
        while self.cur().typ == TokenType.CARET:
            self.advance()
            node = BinOp("^", node, self.parse_bit_and())
        return node

    def parse_bit_and(self) -> Node:
        node = self.parse_equality()
        while self.cur().typ == TokenType.AMP:
            self.advance()
            node = BinOp("&", node, self.parse_equality())
        return node

    def parse_equality(self) -> Node:
        node = self.parse_relational()
        while self.cur().typ in (TokenType.EQ, TokenType.NE):
            op = self.cur().text
            self.advance()
            node = BinOp(op, node, self.parse_relational())
        return node

    def parse_relational(self) -> Node:
        node = self.parse_shift()
        while self.cur().typ in (TokenType.LT, TokenType.LE, TokenType.GT, TokenType.GE):
            op = self.cur().text
            self.advance()
            node = BinOp(op, node, self.parse_shift())
        return node

    def parse_shift(self) -> Node:
        node = self.parse_add()
        while self.cur().typ in (TokenType.LSHIFT, TokenType.RSHIFT):
            op = "<<" if self.cur().typ == TokenType.LSHIFT else ">>"
            self.advance()
            node = BinOp(op, node, self.parse_add())
        return node

    def parse_add(self) -> Node:
        node = self.parse_term()
        while self.cur().typ in (TokenType.PLUS, TokenType.MINUS):
            op = self.cur().text
            self.advance()
            node = BinOp(op, node, self.parse_term())
        return node

    def parse_term(self) -> Node:
        node = self.parse_unary()
        while self.cur().typ in (TokenType.STAR, TokenType.SLASH, TokenType.PERCENT):
            op = self.cur().text
            self.advance()
            node = BinOp(op, node, self.parse_unary())
        return node

    def parse_unary(self) -> Node:
        if self.cur().typ == TokenType.KW_SIZEOF:
            self.advance()
            if self.cur().typ == TokenType.LPAREN:
                save = self.i
                self.advance()
                try:
                    if self.cur().typ == TokenType.KW_CHAR or self.is_type_start():
                        sizeof_ty = self.parse_sizeof_type()
                        if self.cur().typ == TokenType.RPAREN:
                            self.advance()
                            return SizeofExpr(sizeof_ty, None)
                except CompileError:
                    pass
                self.i = save
                self.eat(TokenType.LPAREN)
                expr = self.parse_expr()
                self.eat(TokenType.RPAREN)
                return SizeofExpr(None, expr)
            return SizeofExpr(None, self.parse_unary())

        if self.cur().typ == TokenType.LPAREN and self.peek().typ in (
            TokenType.KW_INT,
            TokenType.KW_CHAR,
            TokenType.KW_VOID,
            TokenType.KW_STRUCT,
            TokenType.KW_UNSIGNED,
            TokenType.KW_SIGNED,
            TokenType.KW_LONG,
            TokenType.KW_SHORT,
            TokenType.KW_DOUBLE,
            TokenType.KW_FLOAT,
            TokenType.KW_UNION,
            TokenType.KW_ENUM,
            TokenType.KW_CONST,
            TokenType.ID,
        ):
            save = self.i
            self.advance()
            if self.is_type_start():
                try:
                    cast_ty = self.parse_type()
                    ptr_level = 0
                    while self.cur().typ == TokenType.STAR:
                        self.advance()
                        ptr_level += 1
                    cast_ty = cast_ty + ("*" * ptr_level)
                    if self.cur().typ == TokenType.RPAREN:
                        self.advance()
                        return Cast(cast_ty, self.parse_unary())
                except CompileError:
                    pass
            self.i = save

        if self.cur().typ in (
            TokenType.PLUS, TokenType.MINUS, TokenType.BANG,
            TokenType.STAR, TokenType.TILDE
        ):
            op = self.cur().text
            self.advance()
            return UnaryOp(op, self.parse_unary())
        if self.cur().typ == TokenType.AMP:
            self.advance()
            return UnaryOp("&", self.parse_unary())
        if self.cur().typ in (TokenType.PLUSPLUS, TokenType.MINUSMINUS):
            op = self.cur().text
            self.advance()
            node = self.parse_unary()
            if not isinstance(node, Var):
                # Be permissive for system-header idioms; drop invalid inc/dec target.
                return node
            return IncDec(node.name, op, True)
        return self.parse_postfix()

    def parse_postfix(self) -> Node:
        node = self.parse_primary()
        while True:
            if self.cur().typ == TokenType.LBRACKET:
                self.advance()
                idx = self.parse_expr()
                self.eat(TokenType.RBRACKET)
                node = Index(node, idx)
                continue
            if self.cur().typ == TokenType.DOT:
                self.advance()
                field = self.eat(TokenType.ID).text
                node = Member(node, field, False)
                continue
            if self.cur().typ == TokenType.ARROW:
                self.advance()
                field = self.eat(TokenType.ID).text
                node = Member(node, field, True)
                continue
            if self.cur().typ in (TokenType.PLUSPLUS, TokenType.MINUSMINUS):
                op = self.cur().text
                self.advance()
                if not isinstance(node, Var):
                    # Be permissive for system-header idioms; drop invalid inc/dec target.
                    continue
                node = IncDec(node.name, op, False)
                continue
            break
        return node

    def parse_args(self) -> List[Node]:
        if self.cur().typ == TokenType.RPAREN:
            return []
        args: List[Node] = []
        while True:
            # If argument looks like a type (e.g. __builtin_va_arg(ap, T)), parse as type cast
            if self.is_type_start():
                save = self.i
                try:
                    ty = self.parse_sizeof_type()
                    # Accept it as a type arg - represent as SizeofExpr with just type
                    args.append(SizeofExpr(ty, None))
                except CompileError:
                    self.i = save
                    args.append(self.parse_expr())
            else:
                args.append(self.parse_expr())
            if self.cur().typ != TokenType.COMMA:
                break
            self.advance()
        return args

    def parse_primary(self) -> Node:
        match self.cur().typ:
            case TokenType.NUM:
                return IntLit(int(self.eat(TokenType.NUM).text))
            case TokenType.STRING:
                tok = self.eat(TokenType.STRING)
                # Handle adjacent string concatenation already done in lexer
                return StringLit(tok.text)
            case TokenType.ID:
                name = self.eat(TokenType.ID).text
                if self.cur().typ == TokenType.LPAREN:
                    self.advance()
                    args = self.parse_args()
                    self.eat(TokenType.RPAREN)
                    return Call(name, args)
                return Var(name)
            case TokenType.LPAREN:
                self.advance()
                n = self.parse_expr()
                self.eat(TokenType.RPAREN)
                return n
            case _:
                raise self.error("expected expression")

