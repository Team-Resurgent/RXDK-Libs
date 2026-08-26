/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * HLSL vertex-shader front end for XGCompileShader (vs_1_1 only).
 *
 * Pipeline: HLSL text -> (this) vs.1.1 assembly text -> CD3DXAssembler +
 * optimizer -> NV2A microcode. This file is purely the front end; it emits
 * straight-line, unoptimised assembly and leaves register pressure /
 * instruction pairing to the assembler's optimiser.
 *
 * Supported subset (what real vs_1_1 HLSL shaders use):
 *   * struct types with semantics; the entry function taking a struct or
 *     individual semantic parameters and returning a struct (or a single
 *     semantic-tagged value)
 *   * global uniforms (float/floatN/float3x3/float4x3/float4x4) bound to
 *     constant registers in declaration order from c0
 *   * local variables, assignment (with swizzled l-values), return
 *   * expressions: + - * / , unary -, member/swizzle, parentheses, literals,
 *     constructors floatN(...), and the intrinsics mul, dot, normalize, cross,
 *     length, saturate, min, max, abs, lerp, reflect, rsqrt, frac, pow
 *
 * Deliberately out of scope (diagnosed, not mis-compiled): user-defined helper
 * functions, flow control (vs_1_1 has none), arrays, and pixel shaders.
 */

#include "pchshadeasm.h"
#include "hlslcompile.h"

namespace XGRAPHICS {

namespace {

// The single V-code for every front-end diagnostic.  The message text carries
// the specifics; retail's fxc used X-codes but this log stream is V-coded.
const DWORD HLSL_ERROR = 5100;

// ---- types -------------------------------------------------------------

enum HType {
    T_VOID, T_FLOAT, T_FLOAT2, T_FLOAT3, T_FLOAT4,
    T_FLOAT3X3, T_FLOAT4X3, T_FLOAT4X4, T_STRUCT
};

static int VecComps(HType t) {
    switch (t) { case T_FLOAT: return 1; case T_FLOAT2: return 2;
                 case T_FLOAT3: return 3; case T_FLOAT4: return 4; default: return 0; }
}
static bool IsMatrix(HType t) { return t == T_FLOAT3X3 || t == T_FLOAT4X3 || t == T_FLOAT4X4; }
static int MatRows(HType t) {
    switch (t) { case T_FLOAT3X3: return 3; case T_FLOAT4X3: return 3; case T_FLOAT4X4: return 4; default: return 0; }
}
static int MatRegs(HType t) {
    // constant registers consumed: one per row (row-major storage)
    switch (t) { case T_FLOAT3X3: return 3; case T_FLOAT4X3: return 4; case T_FLOAT4X4: return 4; default: return 0; }
}
static HType VecType(int comps) {
    switch (comps) { case 1: return T_FLOAT; case 2: return T_FLOAT2;
                     case 3: return T_FLOAT3; default: return T_FLOAT4; }
}

// ---- lexer -------------------------------------------------------------

enum TK { TK_EOF, TK_ID, TK_NUM, TK_PUNCT };
struct Tok { int kind; const char* s; int len; float num; int line; };

static bool IsIdStart(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static bool IsIdCont(char c)  { return IsIdStart(c) || (c >= '0' && c <= '9'); }
static bool IsDigit(char c)   { return c >= '0' && c <= '9'; }

// ---- symbol tables -----------------------------------------------------

struct Field { char name[32]; HType type; char semantic[24]; char reg[10]; };  // reg bound from semantic
struct StructT { char name[40]; Field fields[32]; int count; };
struct Uniform { char name[40]; HType type; int creg; };
struct Local {
    char  name[40];
    HType type;
    char  reg[10];     // temp register for scalar/vector locals
    int   structIdx;   // >=0 if this is a struct instance
    bool  isInput;     // input-struct instance (fields are v-registers)
    bool  isOutput;    // output-struct instance (fields are output registers)
};

// A code-gen value: where a scalar/vector result currently lives.
struct Value {
    HType type;
    char  reg[10];
    char  swz[6];      // component selectors, no dot; "" = natural
    bool  neg;
    int   ownTemp;     // temp index owned by this value, or -1
    bool  isMat; int matBase; int matRows;   // uniform used as a matrix operand
    bool  isLit; float lit[4]; int litN;
};

// ---- the compiler ------------------------------------------------------

class Compiler {
public:
    Compiler(const char* src, DWORD len, const char* entry, const char* target,
             const char* file, Buffer* out, XD3DXErrorLog* log)
      : m_src(src), m_len((int)len), m_entry(entry ? entry : "main"),
        m_target(target), m_file(file ? file : "shader"), m_out(out), m_log(log),
        m_ntok(0), m_pos(0), m_nStruct(0), m_nUniform(0), m_nLocal(0),
        m_nextConst(0), m_nLit(0), m_failed(false),
        m_retType(T_VOID), m_entryParsed(false) {
        for (int i = 0; i < 12; i++) m_temp[i] = false;
        m_retReg[0] = 0;
    }

    HRESULT Run();

private:
    // input
    const char* m_src; int m_len;
    const char* m_entry; const char* m_target; const char* m_file;
    Buffer* m_out; XD3DXErrorLog* m_log;

    // tokens
    Tok m_tok[8192]; int m_ntok; int m_pos;

    // symbols
    StructT m_struct[16]; int m_nStruct;
    Uniform m_uniform[128]; int m_nUniform;
    Local   m_local[64]; int m_nLocal;
    int     m_nextConst;

    // literal constant pool (def cN, ...)
    float m_litVal[64][4]; int m_litReg[64]; int m_nLit;  // packed 4 scalars per creg
    int   m_litUsed[64];                                   // components used per pool entry

    // generated text
    Buffer m_body;   // instruction lines
    bool   m_failed;
    bool   m_temp[12];

    // entry state
    char  m_retReg[10];    // output register for a value-returning entry ("" if struct return)
    HType m_retType;
    bool  m_entryParsed;

    // xbox/screenspace flavour derived from target
    bool xbox() const { return m_target && (m_target[0]=='x'); }

    // ---- diagnostics ----
    void Error(int line, const char* fmt, ...);
    bool Failed() const { return m_failed; }

    // ---- lexer ----
    bool Tokenize();

    // token cursor
    const Tok& Cur()  { return m_tok[m_pos]; }
    const Tok& Peek(int n=1) { int p=m_pos+n; if(p>=m_ntok)p=m_ntok-1; return m_tok[p]; }
    void Next() { if (m_pos < m_ntok-1) m_pos++; }
    bool IsId(const Tok& t, const char* kw);
    bool CurId(const char* kw) { return IsId(Cur(), kw); }
    bool CurPunct(const char* p);
    bool Accept(const char* p);          // punct
    bool AcceptId(const char* kw);
    bool Expect(const char* p);          // punct, error if missing
    void CopyTok(const Tok& t, char* out, int cap);

    // ---- types & semantics ----
    bool ParseType(HType* t, int* structIdx);
    bool IsTypeStart();
    static void MapInputSemantic(const char* sem, char* out);
    static void MapOutputSemantic(const char* sem, char* out);

    // ---- symbol lookup ----
    int  FindStruct(const char* name);
    Uniform* FindUniform(const char* name);
    Local*   FindLocal(const char* name);

    // ---- register allocation ----
    int  AllocTemp();
    void FreeTemp(int i);
    void TempName(int i, char* out);
    void Consume(Value& v) { if (v.ownTemp >= 0) { FreeTemp(v.ownTemp); v.ownTemp = -1; } }

    // literal constants
    void GetScalarConst(float v, Value* out);
    void GetVec4Const(const float* v4, Value* out);
    void EmitDefs(Buffer* dst);

    // ---- emit ----
    void Emit(const char* fmt, ...);
    void SrcText(const Value& v, char* out);          // "[-]reg[.swz]"
    void Materialize(Value& v);                        // literal -> const reg

    // ---- codegen: expressions ----
    Value GenExpr();          // lowest precedence (add/sub)
    Value GenMul();
    Value GenUnary();
    Value GenPrimary();
    Value GenCall(const char* name, int line);
    Value GenConstruct(HType t, int line);
    Value SwizzleOf(Value base, const char* sw, int line);
    Value ToTemp(Value v, int comps);                  // force into a fresh temp

    // intrinsics
    Value IMul(int line);
    Value IDot(int line);
    Value INormalize(int line);
    Value ICross(int line);
    Value ISimple2(const char* op, int line);          // min/max
    Value ILerp(int line);
    Value IReflect(int line);
    Value IUnaryFn(const char* which, int line);       // abs/saturate/rsqrt/frac/length

    // ---- codegen: statements/decls ----
    void GenBody();
    void GenStatement();
    void GenLocalDecl();
    void GenAssignOrExpr();
    bool ParseLValue(char* reg, char* mask, int* comps);  // reg + writemask

    // ---- top level ----
    void ParseTopLevel();
    void ParseStruct();
    void ParseGlobalOrFunc();
    void ParseEntry(HType retType, int retStruct, const char* retSemantic);
};

// ======================================================================
//  diagnostics
// ======================================================================
void Compiler::Error(int line, const char* fmt, ...) {
    char msg[512];
    va_list ap; va_start(ap, fmt);
    _vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    msg[sizeof(msg)-1] = 0;
    if (m_log) m_log->Log(true, HLSL_ERROR, m_file, (DWORD)line, msg);
    m_failed = true;
}

// ======================================================================
//  lexer
// ======================================================================
bool Compiler::Tokenize() {
    int line = 1;
    int i = 0;
    while (i < m_len) {
        char c = m_src[i];
        if (c == '\n') { line++; i++; continue; }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v') { i++; continue; }
        // comments
        if (c == '/' && i+1 < m_len && m_src[i+1] == '/') {
            i += 2; while (i < m_len && m_src[i] != '\n') i++;
            continue;
        }
        if (c == '/' && i+1 < m_len && m_src[i+1] == '*') {
            i += 2;
            while (i+1 < m_len && !(m_src[i]=='*' && m_src[i+1]=='/')) { if (m_src[i]=='\n') line++; i++; }
            i += 2;
            continue;
        }
        // preprocessor left-over lines (#line etc.) -- skip to EOL
        if (c == '#') { while (i < m_len && m_src[i] != '\n') i++; continue; }
        if (m_ntok >= 8191) { Error(line, "shader too large for the vs_1_1 front end"); return false; }
        Tok& t = m_tok[m_ntok];
        t.line = line;
        if (IsIdStart(c)) {
            t.kind = TK_ID; t.s = m_src + i;
            int j = i; while (j < m_len && IsIdCont(m_src[j])) j++;
            t.len = j - i; i = j; m_ntok++;
            continue;
        }
        if (IsDigit(c) || (c == '.' && i+1 < m_len && IsDigit(m_src[i+1]))) {
            t.kind = TK_NUM; t.s = m_src + i;
            int j = i;
            while (j < m_len && (IsDigit(m_src[j]) || m_src[j]=='.')) j++;
            if (j < m_len && (m_src[j]=='e' || m_src[j]=='E')) {
                j++; if (j < m_len && (m_src[j]=='+'||m_src[j]=='-')) j++;
                while (j < m_len && IsDigit(m_src[j])) j++;
            }
            char buf[64]; int n = j - i; if (n > 63) n = 63;
            memcpy(buf, m_src + i, n); buf[n] = 0;
            t.num = (float)atof(buf);
            if (j < m_len && (m_src[j]=='f' || m_src[j]=='F')) j++;   // 1.0f suffix
            t.len = j - i; i = j; m_ntok++;
            continue;
        }
        // punctuation (one or two chars)
        t.kind = TK_PUNCT; t.s = m_src + i;
        char c2 = (i+1 < m_len) ? m_src[i+1] : 0;
        if ((c=='+'||c=='-'||c=='*'||c=='/') && c2=='=') t.len = 2;
        else t.len = 1;
        i += t.len; m_ntok++;
    }
    // EOF sentinel
    Tok& e = m_tok[m_ntok]; e.kind = TK_EOF; e.s = ""; e.len = 0; e.line = line; e.num = 0;
    m_ntok++;
    return true;
}

bool Compiler::IsId(const Tok& t, const char* kw) {
    if (t.kind != TK_ID) return false;
    int n = (int)strlen(kw);
    return t.len == n && strncmp(t.s, kw, n) == 0;
}
bool Compiler::CurPunct(const char* p) {
    const Tok& t = Cur();
    if (t.kind != TK_PUNCT) return false;
    int n = (int)strlen(p);
    return t.len == n && strncmp(t.s, p, n) == 0;
}
bool Compiler::Accept(const char* p) { if (CurPunct(p)) { Next(); return true; } return false; }
bool Compiler::AcceptId(const char* kw) { if (CurId(kw)) { Next(); return true; } return false; }
bool Compiler::Expect(const char* p) {
    if (Accept(p)) return true;
    Error(Cur().line, "expected '%s'", p);
    return false;
}
void Compiler::CopyTok(const Tok& t, char* out, int cap) {
    int n = t.len; if (n > cap-1) n = cap-1;
    memcpy(out, t.s, n); out[n] = 0;
}

// ======================================================================
//  types & semantics
// ======================================================================
bool Compiler::IsTypeStart() {
    const Tok& t = Cur();
    if (t.kind != TK_ID) return false;
    if (IsId(t,"float")||IsId(t,"float2")||IsId(t,"float3")||IsId(t,"float4")||
        IsId(t,"float3x3")||IsId(t,"float4x3")||IsId(t,"float4x4")||
        IsId(t,"matrix")||IsId(t,"vector")||IsId(t,"void")||IsId(t,"half")||
        IsId(t,"half2")||IsId(t,"half3")||IsId(t,"half4"))
        return true;
    char nm[40]; CopyTok(t, nm, sizeof(nm));
    return FindStruct(nm) >= 0;
}
bool Compiler::ParseType(HType* t, int* structIdx) {
    *structIdx = -1;
    const Tok& tk = Cur();
    if (tk.kind != TK_ID) { Error(tk.line, "expected a type"); return false; }
    if      (IsId(tk,"void"))     *t = T_VOID;
    else if (IsId(tk,"float"))    *t = T_FLOAT;
    else if (IsId(tk,"float2")||IsId(tk,"half2")) *t = T_FLOAT2;
    else if (IsId(tk,"float3")||IsId(tk,"half3")) *t = T_FLOAT3;
    else if (IsId(tk,"float4")||IsId(tk,"half4")||IsId(tk,"vector")) *t = T_FLOAT4;
    else if (IsId(tk,"half"))     *t = T_FLOAT;
    else if (IsId(tk,"float3x3")) *t = T_FLOAT3X3;
    else if (IsId(tk,"float4x3")) *t = T_FLOAT4X3;
    else if (IsId(tk,"float4x4")||IsId(tk,"matrix")) *t = T_FLOAT4X4;
    else {
        char nm[40]; CopyTok(tk, nm, sizeof(nm));
        int s = FindStruct(nm);
        if (s < 0) { Error(tk.line, "unknown type '%s'", nm); return false; }
        *t = T_STRUCT; *structIdx = s;
    }
    Next();
    return true;
}

// Uppercase a semantic and split it into base + trailing index.
static void UpperSem(const char* sem, char* base, int* index) {
    char up[32]; int n = 0;
    while (sem[n] && n < 31) { char c = sem[n]; if (c>='a'&&c<='z') c = (char)(c-'a'+'A'); up[n]=c; n++; }
    up[n] = 0;
    int i = n; while (i > 0 && up[i-1] >= '0' && up[i-1] <= '9') i--;
    *index = (i < n) ? atoi(up + i) : 0;
    int j = 0; for (; j < i; j++) base[j] = up[j]; base[j] = 0;
}
void Compiler::MapInputSemantic(const char* sem, char* out) {
    char base[32]; int idx; UpperSem(sem, base, &idx);
    out[0] = 0;
    if (!strcmp(base,"POSITION"))        sprintf(out, "v%d", idx == 0 ? 0 : 15);
    else if (!strcmp(base,"BLENDWEIGHT"))sprintf(out, "v1");
    else if (!strcmp(base,"BLENDINDICES"))sprintf(out,"v2");
    else if (!strcmp(base,"NORMAL"))     sprintf(out, "v3");
    else if (!strcmp(base,"PSIZE"))      sprintf(out, "v4");
    else if (!strcmp(base,"COLOR")||!strcmp(base,"DIFFUSE"))
        sprintf(out, "v%d", 5 + idx);                     // COLOR0->v5, COLOR1->v6
    else if (!strcmp(base,"SPECULAR"))   sprintf(out, "v6");
    else if (!strcmp(base,"TEXCOORD"))   sprintf(out, "v%d", 7 + idx);   // ->v7..v14
    else if (!strcmp(base,"TANGENT"))    sprintf(out, "v%d", 7 + idx);
    else if (!strcmp(base,"BINORMAL"))   sprintf(out, "v%d", 7 + idx);
}
void Compiler::MapOutputSemantic(const char* sem, char* out) {
    char base[32]; int idx; UpperSem(sem, base, &idx);
    out[0] = 0;
    if (!strcmp(base,"POSITION"))      strcpy(out, "oPos");
    else if (!strcmp(base,"PSIZE"))    strcpy(out, "oPts");
    else if (!strcmp(base,"FOG"))      strcpy(out, "oFog");
    else if (!strcmp(base,"COLOR")||!strcmp(base,"DIFFUSE"))
        sprintf(out, "oD%d", idx);                        // COLOR0->oD0, COLOR1->oD1
    else if (!strcmp(base,"SPECULAR")) strcpy(out, "oD1");
    else if (!strcmp(base,"TEXCOORD")) sprintf(out, "oT%d", idx);  // ->oT0..oT3
}

// ======================================================================
//  symbol lookup
// ======================================================================
int Compiler::FindStruct(const char* name) {
    for (int i = 0; i < m_nStruct; i++) if (!strcmp(m_struct[i].name, name)) return i;
    return -1;
}
Uniform* Compiler::FindUniform(const char* name) {
    for (int i = 0; i < m_nUniform; i++) if (!strcmp(m_uniform[i].name, name)) return &m_uniform[i];
    return 0;
}
Local* Compiler::FindLocal(const char* name) {
    for (int i = m_nLocal-1; i >= 0; i--) if (!strcmp(m_local[i].name, name)) return &m_local[i];
    return 0;
}

// ======================================================================
//  register allocation
// ======================================================================
int Compiler::AllocTemp() {
    for (int i = 0; i < 12; i++) if (!m_temp[i]) { m_temp[i] = true; return i; }
    Error(Cur().line, "out of temporary registers (vs_1_1 has 12)");
    return 0;
}
void Compiler::FreeTemp(int i) { if (i >= 0 && i < 12) m_temp[i] = false; }
void Compiler::TempName(int i, char* out) { sprintf(out, "r%d", i); }

void Compiler::GetScalarConst(float v, Value* out) {
    // pack up to 4 scalars per constant register
    for (int i = 0; i < m_nLit; i++)
        for (int c = 0; c < m_litUsed[i]; c++)
            if (m_litVal[i][c] == v) {
                memset(out, 0, sizeof(*out));
                out->type = T_FLOAT; out->ownTemp = -1;
                sprintf(out->reg, "c%d", m_litReg[i]);
                const char* comp = "xyzw"; out->swz[0] = comp[c]; out->swz[1] = 0;
                return;
            }
    if (m_nLit == 0 || m_litUsed[m_nLit-1] == 4) {
        if (m_nLit >= 64) { Error(0, "too many literal constants"); memset(out,0,sizeof(*out)); return; }
        m_litReg[m_nLit] = m_nextConst++; m_litUsed[m_nLit] = 0;
        m_litVal[m_nLit][0]=m_litVal[m_nLit][1]=m_litVal[m_nLit][2]=m_litVal[m_nLit][3]=0;
        m_nLit++;
    }
    int e = m_nLit - 1; int c = m_litUsed[e]; m_litVal[e][c] = v; m_litUsed[e]++;
    memset(out, 0, sizeof(*out));
    out->type = T_FLOAT; out->ownTemp = -1;
    sprintf(out->reg, "c%d", m_litReg[e]);
    const char* comp = "xyzw"; out->swz[0] = comp[c]; out->swz[1] = 0;
}
void Compiler::GetVec4Const(const float* v4, Value* out) {
    if (m_nLit >= 64) { Error(0, "too many literal constants"); memset(out,0,sizeof(*out)); return; }
    m_litReg[m_nLit] = m_nextConst++; m_litUsed[m_nLit] = 4;
    for (int c = 0; c < 4; c++) m_litVal[m_nLit][c] = v4[c];
    memset(out, 0, sizeof(*out));
    out->type = T_FLOAT4; out->ownTemp = -1;
    sprintf(out->reg, "c%d", m_litReg[m_nLit]);
    m_nLit++;
}
void Compiler::EmitDefs(Buffer* dst) {
    for (int i = 0; i < m_nLit; i++)
        dst->Printf("    def c%d, %g, %g, %g, %g\n", m_litReg[i],
                    m_litVal[i][0], m_litVal[i][1], m_litVal[i][2], m_litVal[i][3]);
}

// ======================================================================
//  emit helpers
// ======================================================================
void Compiler::Emit(const char* fmt, ...) {
    char line[256];
    va_list ap; va_start(ap, fmt);
    _vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    line[sizeof(line)-1] = 0;
    m_body.Printf("    %s\n", line);
}
void Compiler::SrcText(const Value& v, char* out) {
    char tmp[24];
    if (v.swz[0]) sprintf(tmp, "%s.%s", v.reg, v.swz);
    else          strcpy(tmp, v.reg);
    if (v.neg) sprintf(out, "-%s", tmp);
    else       strcpy(out, tmp);
}
void Compiler::Materialize(Value& v) {
    if (!v.isLit) return;
    if (v.litN == 1) { bool neg = v.neg; GetScalarConst(v.lit[0], &v); v.neg = neg; }
    else {
        float p[4] = {0,0,0,0}; for (int i=0;i<v.litN;i++) p[i]=v.lit[i];
        bool neg = v.neg; GetVec4Const(p, &v); v.neg = neg;
    }
}

// Force a value into a fresh temp of `comps` components (used when we need a
// stable, writable source).  Returns a value owning that temp.
Value Compiler::ToTemp(Value v, int comps) {
    Materialize(v);
    int t = AllocTemp(); char rn[8]; TempName(t, rn);
    const char* mask = "xyzw"; char m[6]; int n = comps>0?comps:VecComps(v.type); if(n<=0)n=4;
    memcpy(m, mask, n); m[n]=0;
    char src[32]; SrcText(v, src);
    Emit("mov %s.%s, %s", rn, m, src);
    Consume(v);
    Value r; memset(&r, 0, sizeof(r));
    r.type = VecType(n); strcpy(r.reg, rn); r.ownTemp = t;
    return r;
}

// ======================================================================
//  expression codegen
// ======================================================================
Value Compiler::GenPrimary() {
    const Tok& t = Cur();
    // literal
    if (t.kind == TK_NUM) {
        Value v; memset(&v, 0, sizeof(v));
        v.type = T_FLOAT; v.ownTemp = -1; v.isLit = true; v.lit[0] = t.num; v.litN = 1;
        Next();
        return v;
    }
    // parenthesised
    if (CurPunct("(")) {
        Next(); Value v = GenExpr(); Expect(")");
        return v;
    }
    if (t.kind == TK_ID) {
        // constructor?  floatN( ... )
        if ((IsId(t,"float2")||IsId(t,"float3")||IsId(t,"float4")) && Peek().kind==TK_PUNCT && Peek().s[0]=='(') {
            HType ct = IsId(t,"float2")?T_FLOAT2:(IsId(t,"float3")?T_FLOAT3:T_FLOAT4);
            int line = t.line; Next(); Next(); // name '('
            return GenConstruct(ct, line);
        }
        char nm[40]; CopyTok(t, nm, sizeof(nm)); int line = t.line;
        // function call?
        if (Peek().kind==TK_PUNCT && Peek().s[0]=='(' && Peek().len==1) {
            Next(); Next(); // name '('
            return GenCall(nm, line);
        }
        Next();
        // variable reference (uniform / local / struct member)
        Value base; memset(&base, 0, sizeof(base)); base.ownTemp = -1;
        Uniform* u = FindUniform(nm);
        Local* lo = FindLocal(nm);
        if (u) {
            if (IsMatrix(u->type)) { base.isMat = true; base.matBase = u->creg; base.matRows = MatRegs(u->type); base.type = u->type; sprintf(base.reg,"c%d",u->creg); }
            else { base.type = u->type; sprintf(base.reg, "c%d", u->creg); if (VecComps(u->type)==1) strcpy(base.swz,"x"); }
        } else if (lo) {
            if (lo->structIdx >= 0) {
                // must be followed by .field
                if (!Accept(".")) { Error(line, "'%s' is a struct; expected .member", nm); return base; }
                char fn[32]; CopyTok(Cur(), fn, sizeof(fn)); Next();
                StructT& st = m_struct[lo->structIdx];
                Field* f = 0; for (int i=0;i<st.count;i++) if(!strcmp(st.fields[i].name,fn)) f=&st.fields[i];
                if (!f) { Error(line, "struct '%s' has no member '%s'", st.name, fn); return base; }
                base.type = f->type; strcpy(base.reg, f->reg);
                if (lo->isOutput) Error(line, "cannot read shader output '%s.%s'", nm, fn);
            } else {
                base.type = lo->type; strcpy(base.reg, lo->reg);
                // a scalar local lives in .x; reference it with a replicating
                // swizzle so it broadcasts correctly into vector expressions.
                if (VecComps(lo->type) == 1) strcpy(base.swz, "x");
            }
        } else {
            Error(line, "undeclared identifier '%s'", nm);
            base.type = T_FLOAT; strcpy(base.reg, "r0");
            return base;
        }
        // trailing .swizzle
        if (CurPunct(".")) {
            Next(); char sw[16]; CopyTok(Cur(), sw, sizeof(sw)); Next();
            return SwizzleOf(base, sw, line);
        }
        return base;
    }
    Error(t.line, "unexpected token in expression");
    Value v; memset(&v,0,sizeof(v)); v.type=T_FLOAT; strcpy(v.reg,"r0"); v.ownTemp=-1; Next();
    return v;
}

// Apply a swizzle/member selector to a value.
Value Compiler::SwizzleOf(Value base, const char* sw, int line) {
    // translate rgba->xyzw, validate
    char out[6]; int n = 0;
    for (int i = 0; sw[i] && n < 4; i++) {
        char c = sw[i];
        switch (c) { case 'r': c='x'; break; case 'g': c='y'; break;
                     case 'b': c='z'; break; case 'a': c='w'; break; }
        if (c!='x'&&c!='y'&&c!='z'&&c!='w') { Error(line, "invalid swizzle '.%s'", sw); break; }
        out[n++] = c;
    }
    out[n] = 0;
    // compose with any existing swizzle
    if (base.swz[0]) {
        char comp[6]; int m = 0;
        for (int i = 0; i < n; i++) {
            int idx = out[i]=='x'?0:out[i]=='y'?1:out[i]=='z'?2:3;
            int bl = (int)strlen(base.swz);
            comp[m++] = (idx < bl) ? base.swz[idx] : base.swz[bl-1];
        }
        comp[m]=0; strcpy(base.swz, comp);
    } else {
        strcpy(base.swz, out);
    }
    base.type = VecType(n);
    return base;
}

Value Compiler::GenConstruct(HType t, int line) {
    // floatN(a, b, ...) : gather components into a fresh temp
    int need = VecComps(t);
    int rt = AllocTemp(); char rn[8]; TempName(rt, rn);
    int filled = 0;
    const char* comp = "xyzw";
    while (!CurPunct(")") && filled < 4 && !Failed()) {
        Value a = GenExpr();
        Materialize(a);
        int ac = VecComps(a.type); if (ac <= 0) ac = 1;
        if (filled + ac > need) ac = need - filled;
        char mask[6]; for (int i=0;i<ac;i++) mask[i]=comp[filled+i]; mask[ac]=0;
        char src[32]; SrcText(a, src);
        // if source is scalar being spread over >1, replicate; else natural
        Emit("mov %s.%s, %s", rn, mask, src);
        Consume(a);
        filled += ac;
        if (!Accept(",")) break;
    }
    Expect(")");
    if (filled < need) Error(line, "floatN constructor has too few components");
    Value r; memset(&r,0,sizeof(r)); r.type = t; strcpy(r.reg, rn); r.ownTemp = rt;
    return r;
}

Value Compiler::GenCall(const char* name, int line) {
    if (!strcmp(name,"mul"))       return IMul(line);
    if (!strcmp(name,"dot"))       return IDot(line);
    if (!strcmp(name,"normalize")) return INormalize(line);
    if (!strcmp(name,"cross"))     return ICross(line);
    if (!strcmp(name,"min"))       return ISimple2("min", line);
    if (!strcmp(name,"max"))       return ISimple2("max", line);
    if (!strcmp(name,"lerp"))      return ILerp(line);
    if (!strcmp(name,"reflect"))   return IReflect(line);
    if (!strcmp(name,"abs")||!strcmp(name,"saturate")||!strcmp(name,"rsqrt")||
        !strcmp(name,"frac")||!strcmp(name,"length"))
        return IUnaryFn(name, line);
    Error(line, "unsupported function '%s' (vs_1_1 front end has no user functions)", name);
    // consume args to keep parser sane
    int depth = 1; while (depth && Cur().kind != TK_EOF) { if (CurPunct("(")) depth++; else if (CurPunct(")")) depth--; Next(); }
    Value v; memset(&v,0,sizeof(v)); v.type=T_FLOAT; strcpy(v.reg,"r0"); v.ownTemp=-1; return v;
}

Value Compiler::IMul(int line) {
    Value a = GenExpr(); Expect(","); Value b = GenExpr(); Expect(")");
    // Identify the matrix operand.
    Value* mat = 0; Value* vec = 0;
    if (a.isMat) { mat = &a; vec = &b; } else if (b.isMat) { mat = &b; vec = &a; }
    if (mat) {
        Materialize(*vec);
        int rows = mat->matRows;
        int rt = AllocTemp(); char rn[8]; TempName(rt, rn);
        char vsrc[32]; SrcText(*vec, vsrc);
        const char* comp = "xyzw";
        // result[i] = dot(vec, matrixRow_i)  -- row-major transform, the Xbox idiom
        for (int i = 0; i < rows; i++)
            Emit("dp4 %s.%c, %s, c%d", rn, comp[i], vsrc, mat->matBase + i);
        Consume(a); Consume(b);
        Value r; memset(&r,0,sizeof(r)); r.type = VecType(rows); strcpy(r.reg, rn); r.ownTemp = rt;
        return r;
    }
    // vector*vector or scalar -> componentwise multiply
    Materialize(a); Materialize(b);
    int comps = VecComps(a.type); int bc = VecComps(b.type); if (bc>comps) comps=bc; if(comps<=0)comps=1;
    int rt = AllocTemp(); char rn[8]; TempName(rt, rn);
    char as[32], bs[32]; SrcText(a, as); SrcText(b, bs);
    char mask[6]; const char* cc="xyzw"; memcpy(mask,cc,comps); mask[comps]=0;
    Emit("mul %s.%s, %s, %s", rn, mask, as, bs);
    Consume(a); Consume(b);
    Value r; memset(&r,0,sizeof(r)); r.type = VecType(comps); strcpy(r.reg, rn); r.ownTemp = rt;
    return r;
}

Value Compiler::IDot(int line) {
    Value a = GenExpr(); Expect(","); Value b = GenExpr(); Expect(")");
    Materialize(a); Materialize(b);
    int comps = VecComps(a.type); if (comps < VecComps(b.type)) comps = VecComps(b.type);
    int rt = AllocTemp(); char rn[8]; TempName(rt, rn);
    char as[32], bs[32]; SrcText(a, as); SrcText(b, bs);
    if (comps >= 4)      Emit("dp4 %s.x, %s, %s", rn, as, bs);
    else if (comps == 3) Emit("dp3 %s.x, %s, %s", rn, as, bs);
    else { // 2-component dot: mul + add
        Emit("mul %s, %s, %s", rn, as, bs);
        Emit("add %s.x, %s.x, %s.y", rn, rn, rn);
    }
    Consume(a); Consume(b);
    Value r; memset(&r,0,sizeof(r)); r.type = T_FLOAT; strcpy(r.reg, rn); strcpy(r.swz,"x"); r.ownTemp = rt;
    return r;
}

Value Compiler::INormalize(int line) {
    Value a = GenExpr(); Expect(")");
    Materialize(a);
    int comps = VecComps(a.type); if (comps < 3) comps = 3;
    int rt = AllocTemp(); char rn[8]; TempName(rt, rn);
    char as[32]; SrcText(a, as);
    const char* dp = (comps >= 4) ? "dp4" : "dp3";
    Emit("%s %s.w, %s, %s", dp, rn, as, as);
    Emit("rsq %s.w, %s.w", rn, rn);
    char mask[6]; const char* cc="xyzw"; memcpy(mask,cc,comps); mask[comps]=0;
    Emit("mul %s.%s, %s, %s.w", rn, mask, as, rn);
    Consume(a);
    Value r; memset(&r,0,sizeof(r)); r.type = VecType(comps); strcpy(r.reg, rn); r.ownTemp = rt;
    return r;
}

Value Compiler::ICross(int line) {
    Value a = GenExpr(); Expect(","); Value b = GenExpr(); Expect(")");
    Materialize(a); Materialize(b);
    Value at = ToTemp(a, 3); Value bt = ToTemp(b, 3);
    int rt = AllocTemp(); char rn[8]; TempName(rt, rn);
    // cross = a.yzx*b.zxy - a.zxy*b.yzx
    Emit("mul %s.xyz, %s.zxy, %s.yzx", rn, at.reg, bt.reg);
    Emit("mad %s.xyz, %s.yzx, %s.zxy, -%s", rn, at.reg, bt.reg, rn);
    Consume(at); Consume(bt);
    Value r; memset(&r,0,sizeof(r)); r.type = T_FLOAT3; strcpy(r.reg, rn); r.ownTemp = rt;
    return r;
}

Value Compiler::ISimple2(const char* op, int line) {
    Value a = GenExpr(); Expect(","); Value b = GenExpr(); Expect(")");
    Materialize(a); Materialize(b);
    int comps = VecComps(a.type); if (comps<VecComps(b.type)) comps=VecComps(b.type); if(comps<=0)comps=1;
    int rt = AllocTemp(); char rn[8]; TempName(rt, rn);
    char as[32], bs[32]; SrcText(a, as); SrcText(b, bs);
    char mask[6]; const char* cc="xyzw"; memcpy(mask,cc,comps); mask[comps]=0;
    Emit("%s %s.%s, %s, %s", op, rn, mask, as, bs);
    Consume(a); Consume(b);
    Value r; memset(&r,0,sizeof(r)); r.type = VecType(comps); strcpy(r.reg, rn); r.ownTemp = rt;
    return r;
}

Value Compiler::ILerp(int line) {
    Value a = GenExpr(); Expect(","); Value b = GenExpr(); Expect(","); Value t = GenExpr(); Expect(")");
    Materialize(a); Materialize(b); Materialize(t);
    int comps = VecComps(a.type); if (comps<VecComps(b.type)) comps=VecComps(b.type); if(comps<=0)comps=1;
    int rt = AllocTemp(); char rn[8]; TempName(rt, rn);
    char as[32], bs[32], ts[32]; SrcText(a, as); SrcText(b, bs); SrcText(t, ts);
    char mask[6]; const char* cc="xyzw"; memcpy(mask,cc,comps); mask[comps]=0;
    // a + t*(b-a)
    Emit("add %s.%s, %s, -%s", rn, mask, bs, as);
    Emit("mad %s.%s, %s, %s, %s", rn, mask, rn, ts, as);
    Consume(a); Consume(b); Consume(t);
    Value r; memset(&r,0,sizeof(r)); r.type = VecType(comps); strcpy(r.reg, rn); r.ownTemp = rt;
    return r;
}

Value Compiler::IReflect(int line) {
    Value i = GenExpr(); Expect(","); Value n = GenExpr(); Expect(")");
    Value it = ToTemp(i, 3); Value nt = ToTemp(n, 3);
    int rt = AllocTemp(); char rn[8]; TempName(rt, rn);
    // r = i - 2*dot(i,n)*n
    Emit("dp3 %s.w, %s, %s", rn, it.reg, nt.reg);
    Emit("add %s.w, %s.w, %s.w", rn, rn, rn);
    Emit("mul %s.xyz, %s, %s.w", rn, nt.reg, rn);
    Emit("add %s.xyz, %s, -%s", rn, it.reg, rn);
    Consume(it); Consume(nt);
    Value r; memset(&r,0,sizeof(r)); r.type = T_FLOAT3; strcpy(r.reg, rn); r.ownTemp = rt;
    return r;
}

Value Compiler::IUnaryFn(const char* which, int line) {
    Value a = GenExpr(); Expect(")");
    Materialize(a);
    int comps = VecComps(a.type); if (comps<=0) comps=1;
    int rt = AllocTemp(); char rn[8]; TempName(rt, rn);
    char as[32]; SrcText(a, as);
    char mask[6]; const char* cc="xyzw"; memcpy(mask,cc,comps); mask[comps]=0;
    if (!strcmp(which,"abs")) {
        Emit("max %s.%s, %s, -%s", rn, mask, as, as);
        Value r; memset(&r,0,sizeof(r)); r.type=VecType(comps); strcpy(r.reg,rn); r.ownTemp=rt; Consume(a); return r;
    }
    if (!strcmp(which,"saturate")) {
        Value z; GetScalarConst(0.0f, &z); Value o; GetScalarConst(1.0f, &o);
        char zs[16], os[16]; SrcText(z, zs); SrcText(o, os);
        Emit("max %s.%s, %s, %s", rn, mask, as, zs);
        Emit("min %s.%s, %s, %s", rn, mask, rn, os);
        Value r; memset(&r,0,sizeof(r)); r.type=VecType(comps); strcpy(r.reg,rn); r.ownTemp=rt; Consume(a); return r;
    }
    if (!strcmp(which,"rsqrt")) {
        Emit("rsq %s.x, %s", rn, as);
        Value r; memset(&r,0,sizeof(r)); r.type=T_FLOAT; strcpy(r.reg,rn); strcpy(r.swz,"x"); r.ownTemp=rt; Consume(a); return r;
    }
    if (!strcmp(which,"frac")) {
        Emit("frc %s.%s, %s", rn, mask, as);
        Value r; memset(&r,0,sizeof(r)); r.type=VecType(comps); strcpy(r.reg,rn); r.ownTemp=rt; Consume(a); return r;
    }
    // length(v) = sqrt(dot(v,v)) = dot*rsq(dot)
    if (!strcmp(which,"length")) {
        int c = VecComps(a.type); const char* dp = (c>=4)?"dp4":"dp3";
        Emit("%s %s.x, %s, %s", dp, rn, as, as);
        Emit("rsq %s.y, %s.x", rn, rn);
        Emit("mul %s.x, %s.x, %s.y", rn, rn, rn);
        Value r; memset(&r,0,sizeof(r)); r.type=T_FLOAT; strcpy(r.reg,rn); strcpy(r.swz,"x"); r.ownTemp=rt; Consume(a); return r;
    }
    Consume(a);
    Value r; memset(&r,0,sizeof(r)); r.type=VecType(comps); strcpy(r.reg,rn); r.ownTemp=rt; return r;
}

Value Compiler::GenUnary() {
    if (CurPunct("-")) {
        Next(); Value v = GenUnary();
        if (v.isLit) { for (int i=0;i<v.litN;i++) v.lit[i] = -v.lit[i]; return v; }
        v.neg = !v.neg; return v;
    }
    if (CurPunct("+")) { Next(); return GenUnary(); }
    return GenPrimary();
}

Value Compiler::GenMul() {
    Value a = GenUnary();
    while (CurPunct("*") || CurPunct("/")) {
        bool div = CurPunct("/"); int line = Cur().line; Next();
        Value b = GenUnary();
        Materialize(a); Materialize(b);
        int comps = VecComps(a.type); int bc=VecComps(b.type); if(bc>comps)comps=bc; if(comps<=0)comps=1;
        int rt = AllocTemp(); char rn[8]; TempName(rt, rn);
        char as[32], bs[32]; SrcText(a, as); SrcText(b, bs);
        char mask[6]; const char* cc="xyzw"; memcpy(mask,cc,comps); mask[comps]=0;
        if (div) {
            // a / b : reciprocal of (scalar) b, then multiply
            if (VecComps(b.type) != 1) Error(line, "vector divisor not supported; divide by a scalar");
            int tt = AllocTemp(); char tn[8]; TempName(tt, tn);
            Emit("rcp %s.x, %s", tn, bs);
            Emit("mul %s.%s, %s, %s.x", rn, mask, as, tn);
            FreeTemp(tt);
        } else {
            Emit("mul %s.%s, %s, %s", rn, mask, as, bs);
        }
        Consume(a); Consume(b);
        Value r; memset(&r,0,sizeof(r)); r.type=VecType(comps); strcpy(r.reg,rn); r.ownTemp=rt;
        a = r;
    }
    return a;
}

Value Compiler::GenExpr() {
    Value a = GenMul();
    while (CurPunct("+") || CurPunct("-")) {
        bool sub = CurPunct("-"); Next();
        Value b = GenMul();
        Materialize(a); Materialize(b);
        int comps = VecComps(a.type); int bc=VecComps(b.type); if(bc>comps)comps=bc; if(comps<=0)comps=1;
        int rt = AllocTemp(); char rn[8]; TempName(rt, rn);
        char as[32], bs[32]; SrcText(a, as); SrcText(b, bs);
        char mask[6]; const char* cc="xyzw"; memcpy(mask,cc,comps); mask[comps]=0;
        if (sub) Emit("add %s.%s, %s, -%s", rn, mask, as, bs);
        else     Emit("add %s.%s, %s, %s", rn, mask, as, bs);
        Consume(a); Consume(b);
        Value r; memset(&r,0,sizeof(r)); r.type=VecType(comps); strcpy(r.reg,rn); r.ownTemp=rt;
        a = r;
    }
    return a;
}

// ======================================================================
//  statements
// ======================================================================
bool Compiler::ParseLValue(char* reg, char* mask, int* comps) {
    char nm[40]; CopyTok(Cur(), nm, sizeof(nm)); int line = Cur().line; Next();
    Local* lo = FindLocal(nm);
    if (!lo) { Error(line, "undeclared identifier '%s'", nm); strcpy(reg,"r0"); mask[0]=0; *comps=4; return false; }
    if (lo->structIdx >= 0) {
        if (!Expect(".")) return false;
        char fn[32]; CopyTok(Cur(), fn, sizeof(fn)); Next();
        StructT& st = m_struct[lo->structIdx];
        Field* f = 0; for (int i=0;i<st.count;i++) if(!strcmp(st.fields[i].name,fn)) f=&st.fields[i];
        if (!f) { Error(line, "struct '%s' has no member '%s'", st.name, fn); return false; }
        if (lo->isInput) { Error(line, "cannot assign to shader input '%s.%s'", nm, fn); return false; }
        strcpy(reg, f->reg); *comps = VecComps(f->type);
    } else {
        strcpy(reg, lo->reg); *comps = VecComps(lo->type);
    }
    // optional writemask swizzle
    mask[0] = 0;
    if (CurPunct(".")) {
        Next(); char sw[16]; CopyTok(Cur(), sw, sizeof(sw)); Next();
        int n = 0; for (int i=0; sw[i] && n<4; i++) {
            char c=sw[i]; if(c=='r')c='x'; else if(c=='g')c='y'; else if(c=='b')c='z'; else if(c=='a')c='w';
            mask[n++]=c;
        }
        mask[n]=0; *comps = n;
    }
    return true;
}

void Compiler::GenLocalDecl() {
    HType t; int si;
    if (!ParseType(&t, &si)) return;
    for (;;) {
        char nm[40]; CopyTok(Cur(), nm, sizeof(nm)); Next();
        if (m_nLocal >= 64) { Error(Cur().line, "too many locals"); return; }
        Local& L = m_local[m_nLocal];
        strcpy(L.name, nm); L.type = t; L.structIdx = si; L.isInput = false; L.isOutput = false; L.reg[0]=0;
        if (si >= 0) {
            // struct instance: an output-struct's fields alias output registers
            L.isOutput = true;   // locally-declared structs are outputs by convention
            m_nLocal++;
        } else {
            int rt = AllocTemp(); TempName(rt, L.reg);
            m_nLocal++;
            if (Accept("=")) {
                Value v = GenExpr(); Materialize(v);
                int comps = VecComps(t); char mask[6]; const char* cc="xyzw"; memcpy(mask,cc,comps); mask[comps]=0;
                char src[32]; SrcText(v, src);
                Emit("mov %s.%s, %s", L.reg, mask, src);
                Consume(v);
            }
        }
        if (!Accept(",")) break;
    }
    Expect(";");
}

void Compiler::GenAssignOrExpr() {
    char reg[10], mask[6]; int comps;
    if (!ParseLValue(reg, mask, &comps)) { // resync to ';'
        while (!CurPunct(";") && Cur().kind!=TK_EOF) Next(); Accept(";"); return;
    }
    // compound assignment?
    char op = 0;
    if (CurPunct("+=")||CurPunct("-=")||CurPunct("*=")||CurPunct("/=")) { op = Cur().s[0]; Next(); }
    else if (!Expect("=")) { while (!CurPunct(";") && Cur().kind!=TK_EOF) Next(); Accept(";"); return; }

    Value v = GenExpr(); Materialize(v);
    char src[32]; SrcText(v, src);
    if (mask[0] == 0) { const char* cc="xyzw"; memcpy(mask,cc,comps); mask[comps]=0; }
    if (op) {
        const char* o = op=='+'?"add":op=='-'?"add":op=='*'?"mul":"mul";
        // a op= b   ->  reg = reg (op) v
        if (op=='-') Emit("add %s.%s, %s, -%s", reg, mask, reg, src);
        else if (op=='/') { int tt=AllocTemp(); char tn[8]; TempName(tt,tn); Emit("rcp %s.x, %s", tn, src); Emit("mul %s.%s, %s, %s.x", reg, mask, reg, tn); FreeTemp(tt); }
        else Emit("%s %s.%s, %s, %s", o, reg, mask, reg, src);
    } else {
        Emit("mov %s.%s, %s", reg, mask, src);
    }
    Consume(v);
    Expect(";");
}

void Compiler::GenStatement() {
    if (Accept(";")) return;
    if (CurPunct("{")) { Next(); while (!CurPunct("}") && Cur().kind!=TK_EOF && !Failed()) GenStatement(); Expect("}"); return; }
    if (CurId("return")) {
        Next();
        if (!CurPunct(";")) {
            // return <expr> for a value-returning entry: write to the entry's output reg
            // (handled via m_local[0] alias when the entry returns a struct variable).
            // For a bare struct variable, this is a no-op; otherwise evaluate & discard-assign.
            if (m_retReg[0]) {
                Value v = GenExpr(); Materialize(v);
                char src[32]; SrcText(v, src);
                int comps = VecComps(m_retType); if (comps<=0) comps=4;
                char mask[6]; const char* cc="xyzw"; memcpy(mask,cc,comps); mask[comps]=0;
                Emit("mov %s.%s, %s", m_retReg, mask, src);
                Consume(v);
            } else {
                // struct return: fields already written to output regs; just skip the name
                while (!CurPunct(";") && Cur().kind!=TK_EOF) Next();
            }
        }
        Expect(";");
        return;
    }
    if (CurId("if")||CurId("for")||CurId("while")||CurId("do")) {
        Error(Cur().line, "flow control is not available in vs_1_1");
        while (!CurPunct(";") && Cur().kind!=TK_EOF) Next(); Accept(";");
        return;
    }
    if (IsTypeStart()) { GenLocalDecl(); return; }
    GenAssignOrExpr();
}

void Compiler::GenBody() {
    Expect("{");
    while (!CurPunct("}") && Cur().kind != TK_EOF && !Failed()) GenStatement();
    Expect("}");
}

// ======================================================================
//  top level
// ======================================================================
void Compiler::ParseStruct() {
    Next(); // 'struct'
    char nm[40]; CopyTok(Cur(), nm, sizeof(nm)); Next();
    if (m_nStruct >= 16) { Error(Cur().line, "too many structs"); return; }
    StructT& st = m_struct[m_nStruct];
    strcpy(st.name, nm); st.count = 0;
    Expect("{");
    while (!CurPunct("}") && Cur().kind != TK_EOF && !Failed()) {
        HType ft; int fsi;
        if (!ParseType(&ft, &fsi)) break;
        char fn[32]; CopyTok(Cur(), fn, sizeof(fn)); Next();
        char sem[24]; sem[0]=0;
        if (Accept(":")) { CopyTok(Cur(), sem, sizeof(sem)); Next(); }
        Expect(";");
        if (st.count < 32) {
            Field& f = st.fields[st.count];
            strcpy(f.name, fn); f.type = ft; f.reg[0]=0;
            // semantic->register is resolved by usage direction in ParseEntry
            strncpy(f.semantic, sem, sizeof(f.semantic)-1); f.semantic[sizeof(f.semantic)-1]=0;
            st.count++;
        }
    }
    Expect("}"); Accept(";");
    m_nStruct++;
}

void Compiler::ParseGlobalOrFunc() {
    // skip storage-class / qualifier keywords
    while (CurId("uniform")||CurId("const")||CurId("static")||CurId("extern")||
           CurId("shared")||CurId("volatile")||CurId("row_major")||CurId("column_major")||
           CurId("inline")) Next();
    HType t; int si;
    int line = Cur().line;
    if (!ParseType(&t, &si)) { Next(); return; }
    char nm[40]; CopyTok(Cur(), nm, sizeof(nm)); Next();

    // function?  name '('
    if (CurPunct("(")) {
        // return semantic (for value-returning entries) comes after ')'
        bool isEntry = !strcmp(nm, m_entry);
        if (isEntry) { ParseEntry(t, si, ""); return; }
        // non-entry function: not supported; skip its body
        Error(line, "user-defined function '%s' is not supported by the vs_1_1 front end", nm);
        int depth = 0; bool started=false;
        while (Cur().kind != TK_EOF) {
            if (CurPunct("{")) { depth++; started=true; }
            else if (CurPunct("}")) { depth--; if (started && depth==0) { Next(); break; } }
            Next();
        }
        return;
    }

    // global variable (uniform).  Allow : register(cN) and initializer (ignored).
    for (;;) {
        int creg = m_nextConst;
        if (Accept(":")) {
            // register(cN) or a semantic -- only register() affects binding
            if (CurId("register")) {
                Next(); Expect("(");
                char r[16]; CopyTok(Cur(), r, sizeof(r)); Next();
                if ((r[0]=='c'||r[0]=='C') && r[1]) creg = atoi(r+1);
                Expect(")");
            } else { Next(); /* skip a semantic on a global */ }
        }
        if (Accept("=")) { // skip initializer up to , or ;
            int depth=0;
            while (Cur().kind!=TK_EOF) { if(CurPunct("("))depth++; else if(CurPunct(")"))depth--; else if(depth==0&&(CurPunct(",")||CurPunct(";")))break; Next(); }
        }
        if (m_nUniform < 128) {
            Uniform& u = m_uniform[m_nUniform];
            strcpy(u.name, nm); u.type = t; u.creg = creg;
            m_nUniform++;
            int used = IsMatrix(t) ? MatRegs(t) : 1;
            if (creg + used > m_nextConst) m_nextConst = creg + used;
        }
        if (Accept(",")) { CopyTok(Cur(), nm, sizeof(nm)); Next(); continue; }
        break;
    }
    Expect(";");
}

void Compiler::ParseEntry(HType retType, int retStruct, const char* /*retSemantic*/) {
    m_retReg[0] = 0; m_retType = retType;
    // parameters
    Expect("(");
    while (!CurPunct(")") && Cur().kind != TK_EOF && !Failed()) {
        while (CurId("in")||CurId("out")||CurId("inout")||CurId("uniform")||CurId("const")) Next();
        HType pt; int psi;
        if (!ParseType(&pt, &psi)) break;
        char pn[40]; CopyTok(Cur(), pn, sizeof(pn)); Next();
        char sem[24]; sem[0]=0;
        if (Accept(":")) { CopyTok(Cur(), sem, sizeof(sem)); Next(); }
        // register the parameter
        if (m_nLocal < 64) {
            Local& L = m_local[m_nLocal];
            strcpy(L.name, pn); L.type = pt; L.structIdx = psi;
            L.isInput = true; L.isOutput = false; L.reg[0]=0;
            if (psi >= 0) {
                // resolve each field's semantic -> input v-register
                StructT& st = m_struct[psi];
                for (int i = 0; i < st.count; i++) {
                    char vr[10]; MapInputSemantic(st.fields[i].semantic, vr);
                    if (!vr[0]) Error(Cur().line, "unmapped input semantic ':%s' on %s.%s", st.fields[i].semantic, st.name, st.fields[i].name);
                    strcpy(st.fields[i].reg, vr[0]?vr:"v0");
                }
            } else {
                char vr[10]; MapInputSemantic(sem, vr);
                if (!vr[0]) Error(Cur().line, "unmapped input semantic ':%s' on parameter %s", sem, pn);
                strcpy(L.reg, vr[0]?vr:"v0");
            }
            m_nLocal++;
        }
        if (!Accept(",")) break;
    }
    Expect(")");

    // value-returning entry: return semantic after ')'
    if (retStruct < 0 && retType != T_VOID) {
        if (Accept(":")) { char sem[24]; CopyTok(Cur(), sem, sizeof(sem)); Next(); MapOutputSemantic(sem, m_retReg); }
    }
    if (retStruct >= 0) {
        // resolve the output struct's field semantics -> output registers now,
        // so an output-struct local's fields alias them.
        StructT& st = m_struct[retStruct];
        for (int i = 0; i < st.count; i++) {
            char orr[10]; MapOutputSemantic(st.fields[i].semantic, orr);
            if (!orr[0]) Error(Cur().line, "unmapped output semantic ':%s' on %s.%s", st.fields[i].semantic, st.name, st.fields[i].name);
            strcpy(st.fields[i].reg, orr[0]?orr:"oPos");
        }
    }
    GenBody();
    m_entryParsed = true;
}

// ======================================================================
//  driver
// ======================================================================
HRESULT Compiler::Run() {
    if (!m_target || (strncmp(m_target,"vs",2) && strncmp(m_target,"xvs",3))) {
        Error(0, "unsupported target '%s' (use vs.1.1, xvs.1.1 or xvss.1.1)", m_target ? m_target : "(null)");
        return E_INVALIDARG;
    }
    if (m_body.Initialize(8192) != S_OK) return E_OUTOFMEMORY;
    if (!Tokenize()) return E_FAIL;

    ParseTopLevel();
    if (Failed()) return E_FAIL;

    // Assemble the final text: version line, def constants, then the body.
    const char* ver = "vs.1.1";
    if (!strncmp(m_target,"xvss",4)) ver = "xvss.1.1";
    else if (!strncmp(m_target,"xvs",3)) ver = "xvs.1.1";

    Buffer defs; if (defs.Initialize(1024) != S_OK) return E_OUTOFMEMORY;
    EmitDefs(&defs);

    if (m_out->Initialize(m_body.GetUsed() + defs.GetUsed() + 256) != S_OK) return E_OUTOFMEMORY;
    m_out->Printf("%s\n", ver);
    if (defs.GetUsed()) m_out->Append(defs);
    m_out->Append(m_body);
    return S_OK;
}

void Compiler::ParseTopLevel() {
    while (Cur().kind != TK_EOF && !Failed()) {
        if (CurPunct(";")) { Next(); continue; }
        if (CurId("typedef")) { while (!CurPunct(";") && Cur().kind!=TK_EOF) Next(); Accept(";"); continue; }
        if (CurId("struct")) { ParseStruct(); continue; }
        int before = m_pos;
        ParseGlobalOrFunc();
        if (m_pos == before) Next();   // guard against no-progress
    }
    if (!m_entryParsed && !Failed())
        Error(0, "entry function '%s' not found", m_entry);
}

} // anonymous namespace

// ======================================================================
//  public entry
// ======================================================================
HRESULT CompileHlslVertexShader(
    const char* pSource, DWORD sourceLen,
    const char* pEntryName, const char* pTargetName,
    const char* pSourceFileName, Buffer* pAsmOut, XD3DXErrorLog* pErrorLog)
{
    if (!pSource || !pAsmOut) return E_INVALIDARG;
    Compiler c(pSource, sourceLen, pEntryName, pTargetName, pSourceFileName, pAsmOut, pErrorLog);
    return c.Run();
}

} // namespace XGRAPHICS
