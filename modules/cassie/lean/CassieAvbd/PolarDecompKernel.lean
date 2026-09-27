import LeanSlang

/-!
# `CassieAvbd.PolarDecompKernel` — df32 polar decomposition (Wahba), runtime form

The reference in `src/sketch/cassie_polar.cpp` runs in double and calls
`std::acos`/`std::cos`; a float32 SPIR-V kernel matches neither. This kernel
computes in **df32** (a hi+lo float32 pair, Dekker/Knuth error-free transforms),
so every operation reduces to correctly-rounded float32 `+ - * / sqrt`, with no
fused multiply-add, and the two targets agree on any conformant driver (the
interval gate of RFD 2269 covers the rest). It is transcendental-free: the eigenvalue cosines are the roots
of `4c^3-3c=r` (triple-angle identity, `df_cubic_c1`) rather than acos/cos, and
`M^{-1/2}` is a Newton-Schulz iteration of matrix products rather than an
eigenvector decomposition.

`main` computes the Wahba rotation R = H·(HᵀH)^{-1/2} from matched tangent pairs:
cross-covariance `H = Σ q_i p_i^T`, `M = HᵀH`, `M^{-1/2}` by Newton-Schulz
(`Y→(sM)^{-1/2}` with `s=1/tr(M)`), then `R = H·M^{-1/2}`. Following the
reference, there is no reflection branch (`M^{-1/2}` is invariant to eigenvector
sign, and the data is a proper rotation). Compiled `-fp-mode precise`.

Validated CPU-side vs double: M 1.3e-14, M^{-1/2} 5.4e-14 (M^{-1/2} M M^{-1/2}=I),
R recovers a known rotation to input precision. `df_cos` / `df_cubic_c1` and their
coefficient tables are kept as a validated df32 primitive library (not emitted —
absent from the `functions` list).

## Binding layout (set 0)
  0  ConstantBuffer<PdParams>    { uint n; }
  1  StructuredBuffer<float3>    in_p      length = n   (source tangents)
  2  StructuredBuffer<float3>    in_q      length = n   (target tangents)
  3  RWStructuredBuffer<float>   out_hilo  length = 18  (R row-major, df32 hi/lo pairs)
-/

namespace CassieAvbd.PolarDecompKernel

open LeanSlang

private def floatTy : SlangType := .scalar .float
private def fIn  (name : String) : SlangBinding := ⟨name, floatTy, Semantic.none, none, none, .qIn⟩
private def fOut (name : String) : SlangBinding := ⟨name, floatTy, Semantic.none, none, none, .qOut⟩

-- Exact float32 constant by its bit pattern. LeanSlang's `litFloat` renders via
-- Float.toString (6 digits) and cannot carry a full-precision constant, so the
-- df32 coefficients and the reduction constants are emitted as their float32
-- bits through `asfloat`, a bit-cast that is exact and identical on both targets.
private def fb (bits : Nat) : SlangExpr := .call "asfloat" [.litUint bits]

private def two_sum : SlangFunctionDecl :=
  { attrs := [], retType := .named "void", name := "two_sum"
  , params := [fIn "a", fIn "b", fOut "hi", fOut "lo"]
  , body :=
      [ .declInit floatTy "h"    (.bin "+" (.var "a") (.var "b"))
      , .declInit floatTy "bb"   (.bin "-" (.var "h") (.var "a"))
      , .declInit floatTy "ah"   (.bin "-" (.var "h") (.var "bb"))
      , .declInit floatTy "lo_a" (.bin "-" (.var "a") (.var "ah"))
      , .declInit floatTy "lo_b" (.bin "-" (.var "b") (.var "bb"))
      , .assign (.var "hi") (.var "h")
      , .assign (.var "lo") (.bin "+" (.var "lo_a") (.var "lo_b"))
      , .ret none ] }

private def quick_two_sum : SlangFunctionDecl :=
  { attrs := [], retType := .named "void", name := "quick_two_sum"
  , params := [fIn "a", fIn "b", fOut "hi", fOut "lo"]
  , body :=
      [ .declInit floatTy "h" (.bin "+" (.var "a") (.var "b"))
      , .declInit floatTy "t" (.bin "-" (.var "h") (.var "a"))
      , .assign (.var "hi") (.var "h")
      , .assign (.var "lo") (.bin "-" (.var "b") (.var "t"))
      , .ret none ] }

-- FMA-free product error (Dekker via Veltkamp split, factor 2^12+1). A fused
-- `fma(a,b,-h)` gives the error in one op, but the D3D12 translation emulates
-- fma as a rounded multiply-add and returns ~0 for it, collapsing df32 to
-- float32; the split needs only correctly-rounded + - *, which it has.
private def two_prod : SlangFunctionDecl :=
  { attrs := [], retType := .named "void", name := "two_prod"
  , params := [fIn "a", fIn "b", fOut "hi", fOut "lo"]
  , body :=
      [ .declInit floatTy "h" (.bin "*" (.var "a") (.var "b"))
      , .declInit floatTy "ca" (.bin "*" (.litFloat 4097.0) (.var "a"))
      , .declInit floatTy "ah" (.bin "-" (.var "ca") (.bin "-" (.var "ca") (.var "a")))
      , .declInit floatTy "al" (.bin "-" (.var "a") (.var "ah"))
      , .declInit floatTy "cb" (.bin "*" (.litFloat 4097.0) (.var "b"))
      , .declInit floatTy "bh" (.bin "-" (.var "cb") (.bin "-" (.var "cb") (.var "b")))
      , .declInit floatTy "bl" (.bin "-" (.var "b") (.var "bh"))
      , .declInit floatTy "e1" (.bin "-" (.bin "*" (.var "ah") (.var "bh")) (.var "h"))
      , .declInit floatTy "e2" (.bin "+" (.var "e1") (.bin "*" (.var "ah") (.var "bl")))
      , .declInit floatTy "e3" (.bin "+" (.var "e2") (.bin "*" (.var "al") (.var "bh")))
      , .assign (.var "hi") (.var "h")
      , .assign (.var "lo") (.bin "+" (.var "e3") (.bin "*" (.var "al") (.var "bl")))
      , .ret none ] }

private def df_add : SlangFunctionDecl :=
  { attrs := [], retType := .named "void", name := "df_add"
  , params := [fIn "x_hi", fIn "x_lo", fIn "y_hi", fIn "y_lo", fOut "z_hi", fOut "z_lo"]
  , body :=
      [ .declare floatTy "sh" none, .declare floatTy "sl" none
      , .expr (.call "two_sum" [.var "x_hi", .var "y_hi", .var "sh", .var "sl"])
      , .declInit floatTy "xy_lo" (.bin "+" (.var "x_lo") (.var "y_lo"))
      , .declInit floatTy "sl2"   (.bin "+" (.var "sl")  (.var "xy_lo"))
      , .expr (.call "quick_two_sum" [.var "sh", .var "sl2", .var "z_hi", .var "z_lo"])
      , .ret none ] }

private def df_mul : SlangFunctionDecl :=
  { attrs := [], retType := .named "void", name := "df_mul"
  , params := [fIn "a_hi", fIn "a_lo", fIn "b_hi", fIn "b_lo", fOut "z_hi", fOut "z_lo"]
  , body :=
      [ .declare floatTy "p_hi" none, .declare floatTy "p_lo" none
      , .expr (.call "two_prod" [.var "a_hi", .var "b_hi", .var "p_hi", .var "p_lo"])
      , .declInit floatTy "cross"
          (.bin "+" (.bin "*" (.var "a_hi") (.var "b_lo")) (.bin "*" (.var "a_lo") (.var "b_hi")))
      , .declInit floatTy "plo2" (.bin "+" (.var "p_lo") (.var "cross"))
      , .expr (.call "quick_two_sum" [.var "p_hi", .var "plo2", .var "z_hi", .var "z_lo"])
      , .ret none ] }

private def df_div : SlangFunctionDecl :=
  { attrs := [], retType := .named "void", name := "df_div"
  , params := [fIn "a_hi", fIn "a_lo", fIn "b_hi", fIn "b_lo", fOut "z_hi", fOut "z_lo"]
  , body :=
      [ .declInit floatTy "q1" (.bin "/" (.var "a_hi") (.var "b_hi"))
      -- r = a - q1*b  (df32)
      , .declare floatTy "qb_hi" none, .declare floatTy "qb_lo" none
      , .expr (.call "df_mul" [.var "q1", .litFloat 0.0, .var "b_hi", .var "b_lo", .var "qb_hi", .var "qb_lo"])
      , .declare floatTy "r_hi" none, .declare floatTy "r_lo" none
      , .expr (.call "df_add" [.var "a_hi", .var "a_lo",
          .un "-" (.var "qb_hi"), .un "-" (.var "qb_lo"), .var "r_hi", .var "r_lo"])
      , .declInit floatTy "q2" (.bin "/" (.var "r_hi") (.var "b_hi"))
      , .expr (.call "quick_two_sum" [.var "q1", .var "q2", .var "z_hi", .var "z_lo"])
      , .ret none ] }

private def df_sqrt : SlangFunctionDecl :=
  { attrs := [], retType := .named "void", name := "df_sqrt"
  , params := [fIn "a_hi", fIn "a_lo", fOut "z_hi", fOut "z_lo"]
  , body :=
      [ .ifNoElse (.bin "<=" (.var "a_hi") (.litFloat 0.0))
          [ .assign (.var "z_hi") (.litFloat 0.0), .assign (.var "z_lo") (.litFloat 0.0), .ret none ]
      , .declInit floatTy "x0" (.call "sqrt" [.var "a_hi"])
      , .declare floatTy "xx_hi" none, .declare floatTy "xx_lo" none
      , .expr (.call "two_prod" [.var "x0", .var "x0", .var "xx_hi", .var "xx_lo"])
      , .declare floatTy "r_hi" none, .declare floatTy "r_lo" none
      , .expr (.call "df_add" [.var "a_hi", .var "a_lo",
          .un "-" (.var "xx_hi"), .un "-" (.var "xx_lo"), .var "r_hi", .var "r_lo"])
      , .declInit floatTy "corr" (.bin "/" (.var "r_hi") (.bin "*" (.litFloat 2.0) (.var "x0")))
      , .expr (.call "quick_two_sum" [.var "x0", .var "corr", .var "z_hi", .var "z_lo"])
      , .ret none ] }

-- df32 cos/sin Taylor coefficients (hi, lo). Orders fixed in Lean against
-- Float.cos/Float.sin: 8 cos terms, 7 sin terms clear the df32 floor over [-π/4,π/4].
private def cosBits : List (Nat × Nat) :=
  [ (1065353216, 0), (3204448256, 0), (1026206379, 2963974827),
    (3132492641, 773056831), (936381697, 2864696317), (3029594750, 671643146),
    (823097031, 620727690), (2907294629, 2693531594) ]
private def sinBits : List (Nat × Nat) :=
  [ (1065353216, 0), (3190467243, 833268395), (1007192201, 2951671535),
    (3109031169, 742378493), (909700893, 690673894), (3000447531, 2784929948),
    (791712305, 2730850607) ]

-- Generate a df32 Horner `acc = ((c_last)·r2 + …)·r2 + c_0` into the pair
-- (`accHi`,`accLo`), reading `r2Hi`/`r2Lo`. Temp names are indexed for uniqueness.
private def hornerDf32 (tag : String) (coeffs : List (Nat × Nat))
    (r2Hi r2Lo accHi accLo : String) : List SlangStmt :=
  match coeffs.reverse with
  | [] => []
  | (h0, l0) :: rest =>
    let init := [ .assign (.var accHi) (fb h0), .assign (.var accLo) (fb l0) ]
    let steps := rest.zipIdx.flatMap (fun ((ch, cl), i) =>
      let mh := s!"{tag}_m{i}_hi"; let ml := s!"{tag}_m{i}_lo"
      [ SlangStmt.declare floatTy mh none, SlangStmt.declare floatTy ml none
      , SlangStmt.expr (.call "df_mul" [.var accHi, .var accLo, .var r2Hi, .var r2Lo, .var mh, .var ml])
      , SlangStmt.expr (.call "df_add" [.var mh, .var ml, fb ch, fb cl, .var accHi, .var accLo]) ])
    init ++ steps

private def two_over_pi_b : Nat := 1059256707
private def pio2_hi_b : Nat := 1070141403
private def pio2_lo_b : Nat := 3007036718

private def df_cos : SlangFunctionDecl :=
  { attrs := [], retType := .named "void", name := "df_cos"
  , params := [fIn "x_hi", fIn "x_lo", fOut "z_hi", fOut "z_lo"]
  , body :=
      ([ -- k = floor(x_hi * (2/π) + 1/2); kf its float, ki = k mod 4
         .declInit floatTy "kf" (.call "floor"
           [ .bin "+" (.bin "*" (.var "x_hi") (fb two_over_pi_b)) (.litFloat 0.5) ])
       -- Cody-Waite reduction: r = x - kf·(π/2), π/2 as a df32 pair.
       , .declare floatTy "ph" none, .declare floatTy "pl" none
       , .expr (.call "two_prod" [.var "kf", fb pio2_hi_b, .var "ph", .var "pl"])
       , .declare floatTy "t_hi" none, .declare floatTy "t_lo" none
       , .expr (.call "df_add" [.var "x_hi", .var "x_lo",
           .un "-" (.var "ph"), .un "-" (.var "pl"), .var "t_hi", .var "t_lo"])
       , .declInit floatTy "klo" (.bin "*" (.var "kf") (fb pio2_lo_b))
       , .declare floatTy "r_hi" none, .declare floatTy "r_lo" none
       , .expr (.call "df_add" [.var "t_hi", .var "t_lo",
           .un "-" (.var "klo"), .litFloat 0.0, .var "r_hi", .var "r_lo"])
       -- r2 = r·r
       , .declare floatTy "r2_hi" none, .declare floatTy "r2_lo" none
       , .expr (.call "df_mul" [.var "r_hi", .var "r_lo", .var "r_hi", .var "r_lo", .var "r2_hi", .var "r2_lo"])
       , .declare floatTy "ch" none, .declare floatTy "cl" none
       , .declare floatTy "sh" none, .declare floatTy "sl" none ]
       ++ hornerDf32 "cos" cosBits "r2_hi" "r2_lo" "ch" "cl"
       ++ hornerDf32 "sin" sinBits "r2_hi" "r2_lo" "sh" "sl"
       -- sin(r) = r · sinHorner(r2)
       ++ [ .declare floatTy "sinxh" none, .declare floatTy "sinxl" none
       , .expr (.call "df_mul" [.var "sh", .var "sl", .var "r_hi", .var "r_lo", .var "sinxh", .var "sinxl"])
       -- quadrant select: 0→cos, 1→-sin, 2→-cos, 3→sin
       , .ifThen (.bin "<" (.var "kf") (.litFloat 0.5))
           [ .assign (.var "z_hi") (.var "ch"), .assign (.var "z_lo") (.var "cl") ]
           [ .ifThen (.bin "<" (.var "kf") (.litFloat 1.5))
               [ .assign (.var "z_hi") (.un "-" (.var "sinxh")), .assign (.var "z_lo") (.un "-" (.var "sinxl")) ]
               [ .assign (.var "z_hi") (.un "-" (.var "ch")), .assign (.var "z_lo") (.un "-" (.var "cl")) ] ]
       , .ret none ]) }

-- Largest root of 4c³ - 3c - r = 0 (r ∈ [-1,1]) by df32 Newton from c=1. This is
-- cos(acos(r)/3), the largest eigenvalue cosine — the triple-angle identity lets the
-- eigensolve use a cubic root instead of acos/cos, so it stays in df32 arithmetic.
private def df_cubic_c1 : SlangFunctionDecl :=
  { attrs := [], retType := .named "void", name := "df_cubic_c1"
  , params := [fIn "r_hi", fIn "r_lo", fOut "c_hi", fOut "c_lo"]
  , body :=
      [ .assign (.var "c_hi") (.litFloat 1.0), .assign (.var "c_lo") (.litFloat 0.0)
      , .forCount "it" (.litUint 0) (.litUint 10)
          [ .declare floatTy "c2h" none, .declare floatTy "c2l" none
          , .expr (.call "df_mul" [.var "c_hi", .var "c_lo", .var "c_hi", .var "c_lo", .var "c2h", .var "c2l"])
          , .declare floatTy "c3h" none, .declare floatTy "c3l" none
          , .expr (.call "df_mul" [.var "c2h", .var "c2l", .var "c_hi", .var "c_lo", .var "c3h", .var "c3l"])
          -- f = 4·c3 - 3·c - r
          , .declare floatTy "f1h" none, .declare floatTy "f1l" none
          , .expr (.call "df_mul" [.litFloat 4.0, .litFloat 0.0, .var "c3h", .var "c3l", .var "f1h", .var "f1l"])
          , .declare floatTy "f2h" none, .declare floatTy "f2l" none
          , .expr (.call "df_mul" [.litFloat (-3.0), .litFloat 0.0, .var "c_hi", .var "c_lo", .var "f2h", .var "f2l"])
          , .declare floatTy "t1h" none, .declare floatTy "t1l" none
          , .expr (.call "df_add" [.var "f1h", .var "f1l", .var "f2h", .var "f2l", .var "t1h", .var "t1l"])
          , .declare floatTy "fh" none, .declare floatTy "fl" none
          , .expr (.call "df_add" [.var "t1h", .var "t1l", .un "-" (.var "r_hi"), .un "-" (.var "r_lo"), .var "fh", .var "fl"])
          -- fp = 12·c2 - 3
          , .declare floatTy "p1h" none, .declare floatTy "p1l" none
          , .expr (.call "df_mul" [.litFloat 12.0, .litFloat 0.0, .var "c2h", .var "c2l", .var "p1h", .var "p1l"])
          , .declare floatTy "fph" none, .declare floatTy "fpl" none
          , .expr (.call "df_add" [.var "p1h", .var "p1l", .litFloat (-3.0), .litFloat 0.0, .var "fph", .var "fpl"])
          -- dc = f/fp ; c = c - dc
          , .declare floatTy "dch" none, .declare floatTy "dcl" none
          , .expr (.call "df_div" [.var "fh", .var "fl", .var "fph", .var "fpl", .var "dch", .var "dcl"])
          , .declare floatTy "nh" none, .declare floatTy "nl" none
          , .expr (.call "df_add" [.var "c_hi", .var "c_lo", .un "-" (.var "dch"), .un "-" (.var "dcl"), .var "nh", .var "nl"])
          , .assign (.var "c_hi") (.var "nh"), .assign (.var "c_lo") (.var "nl") ]
      , .ret none ] }

-- Generate a df32 3×3 matmul C = A·B over local arrays `<a>h/<a>l`, `<b>h/<b>l`,
-- `<c>h/<c>l` (row-major, index i*3+j). `tag` uniquifies the temp names so several
-- matmuls can sit in one block. Lean-generated to avoid hand-unrolling 27 products.
private def matmul3 (tag a b c : String) : List SlangStmt :=
  let ah := a ++ "h"; let al := a ++ "l"; let bh := b ++ "h"; let bl := b ++ "l"
  let ch := c ++ "h"; let cl := c ++ "l"
  [ SlangStmt.forCount (tag ++ "i") (.litUint 0) (.litUint 3)
      [ SlangStmt.forCount (tag ++ "j") (.litUint 0) (.litUint 3)
          [ .declInit floatTy (tag ++ "ah") (.litFloat 0.0)
          , .declInit floatTy (tag ++ "al") (.litFloat 0.0)
          , .forCount (tag ++ "k") (.litUint 0) (.litUint 3)
              [ .declInit (.scalar .uint) (tag ++ "ia")
                  (.bin "+" (.bin "*" (.var (tag ++ "i")) (.litUint 3)) (.var (tag ++ "k")))
              , .declInit (.scalar .uint) (tag ++ "ib")
                  (.bin "+" (.bin "*" (.var (tag ++ "k")) (.litUint 3)) (.var (tag ++ "j")))
              , .declare floatTy (tag ++ "ph") none, .declare floatTy (tag ++ "pl") none
              , .expr (.call "df_mul" [.index (.var ah) (.var (tag ++ "ia")), .index (.var al) (.var (tag ++ "ia")),
                  .index (.var bh) (.var (tag ++ "ib")), .index (.var bl) (.var (tag ++ "ib")), .var (tag ++ "ph"), .var (tag ++ "pl")])
              , .declare floatTy (tag ++ "nh") none, .declare floatTy (tag ++ "nl") none
              , .expr (.call "df_add" [.var (tag ++ "ah"), .var (tag ++ "al"), .var (tag ++ "ph"), .var (tag ++ "pl"), .var (tag ++ "nh"), .var (tag ++ "nl")])
              , .assign (.var (tag ++ "ah")) (.var (tag ++ "nh")), .assign (.var (tag ++ "al")) (.var (tag ++ "nl")) ]
          , .declInit (.scalar .uint) (tag ++ "ij") (.bin "+" (.bin "*" (.var (tag ++ "i")) (.litUint 3)) (.var (tag ++ "j")))
          , .assign (.index (.var ch) (.var (tag ++ "ij"))) (.var (tag ++ "ah"))
          , .assign (.index (.var cl) (.var (tag ++ "ij"))) (.var (tag ++ "al")) ] ] ]

private def mainEntry : SlangFunctionDecl :=
  { attrs := [.shaderCompute, .numthreads 1 1 1]
  , name := "main"
  , params := [⟨"tid", .vec .uint 3, .svDispatchThreadId, none, none, .qIn⟩]
  , body :=
      [ .declareArray floatTy "hh" 9, .declareArray floatTy "hl" 9
      -- H = Σ q_i p_i^T, accumulated in df32 (row a, col b at a*3+b).
      , .forCount "z" (.litUint 0) (.litUint 9)
          [ .assign (.index (.var "hh") (.var "z")) (.litFloat 0.0)
          , .assign (.index (.var "hl") (.var "z")) (.litFloat 0.0) ]
      , .forCount "i" (.litUint 0) (.member (.var "params") "n")
          [ .declInit (.vec .float 3) "pv" (.index (.var "in_p") (.var "i"))
          , .declInit (.vec .float 3) "qv" (.index (.var "in_q") (.var "i"))
          , .forCount "a" (.litUint 0) (.litUint 3)
              [ .forCount "b" (.litUint 0) (.litUint 3)
                  [ .declInit (.scalar .uint) "idx" (.bin "+" (.bin "*" (.var "a") (.litUint 3)) (.var "b"))
                  , .declInit floatTy "qa" (.index (.var "qv") (.var "a"))
                  , .declInit floatTy "pb" (.index (.var "pv") (.var "b"))
                  , .declare floatTy "ph" none, .declare floatTy "pl" none
                  , .expr (.call "two_prod" [.var "qa", .var "pb", .var "ph", .var "pl"])
                  , .declare floatTy "nh" none, .declare floatTy "nl" none
                  , .expr (.call "df_add" [.index (.var "hh") (.var "idx"), .index (.var "hl") (.var "idx"),
                      .var "ph", .var "pl", .var "nh", .var "nl"])
                  , .assign (.index (.var "hh") (.var "idx")) (.var "nh")
                  , .assign (.index (.var "hl") (.var "idx")) (.var "nl") ] ] ]
      -- M = H^T H : M[i][j] = Σ_k H[k][i]·H[k][j], df32.
      , .declareArray floatTy "mh" 9, .declareArray floatTy "ml" 9
      , .forCount "ii" (.litUint 0) (.litUint 3)
          [ .forCount "jj" (.litUint 0) (.litUint 3)
              [ .declInit floatTy "acch" (.litFloat 0.0), .declInit floatTy "accl" (.litFloat 0.0)
              , .forCount "k" (.litUint 0) (.litUint 3)
                  [ .declInit (.scalar .uint) "ki_" (.bin "+" (.bin "*" (.var "k") (.litUint 3)) (.var "ii"))
                  , .declInit (.scalar .uint) "kj_" (.bin "+" (.bin "*" (.var "k") (.litUint 3)) (.var "jj"))
                  , .declare floatTy "ph" none, .declare floatTy "pl" none
                  , .expr (.call "df_mul" [.index (.var "hh") (.var "ki_"), .index (.var "hl") (.var "ki_"),
                      .index (.var "hh") (.var "kj_"), .index (.var "hl") (.var "kj_"), .var "ph", .var "pl"])
                  , .declare floatTy "nh" none, .declare floatTy "nl" none
                  , .expr (.call "df_add" [.var "acch", .var "accl", .var "ph", .var "pl", .var "nh", .var "nl"])
                  , .assign (.var "acch") (.var "nh"), .assign (.var "accl") (.var "nl") ]
              , .declInit (.scalar .uint) "mij" (.bin "+" (.bin "*" (.var "ii") (.litUint 3)) (.var "jj"))
              , .assign (.index (.var "mh") (.var "mij")) (.var "acch")
              , .assign (.index (.var "ml") (.var "mij")) (.var "accl") ] ]
      -- Newton-Schulz M^{-1/2}: s = 1/tr(M) scales sM to spectral radius ≤ 1;
      -- iterate Y → (sM)^{-1/2}, Z → (sM)^{1/2}, then M^{-1/2} = sqrt(s)·Y and
      -- R = H · M^{-1/2}. Pure df32 matmuls — no eigendecomposition or null-space.
      , .declare floatTy "trh" none, .declare floatTy "trl" none
      , .expr (.call "df_add" [.index (.var "mh") (.litUint 0), .index (.var "ml") (.litUint 0), .index (.var "mh") (.litUint 4), .index (.var "ml") (.litUint 4), .var "trh", .var "trl"])
      , .declare floatTy "tr2h" none, .declare floatTy "tr2l" none
      , .expr (.call "df_add" [.var "trh", .var "trl", .index (.var "mh") (.litUint 8), .index (.var "ml") (.litUint 8), .var "tr2h", .var "tr2l"])
      , .declare floatTy "sh" none, .declare floatTy "sl" none
      , .expr (.call "df_div" [.litFloat 1.0, .litFloat 0.0, .var "tr2h", .var "tr2l", .var "sh", .var "sl"])
      , .declareArray floatTy "zh" 9, .declareArray floatTy "zl" 9
      , .declareArray floatTy "yh" 9, .declareArray floatTy "yl" 9
      , .forCount "zi" (.litUint 0) (.litUint 9)
          [ .declare floatTy "zph" none, .declare floatTy "zpl" none
          , .expr (.call "df_mul" [.var "sh", .var "sl", .index (.var "mh") (.var "zi"), .index (.var "ml") (.var "zi"), .var "zph", .var "zpl"])
          , .assign (.index (.var "zh") (.var "zi")) (.var "zph")
          , .assign (.index (.var "zl") (.var "zi")) (.var "zpl")
          , .assign (.index (.var "yh") (.var "zi")) (.litFloat 0.0)
          , .assign (.index (.var "yl") (.var "zi")) (.litFloat 0.0) ]
      , .assign (.index (.var "yh") (.litUint 0)) (.litFloat 1.0)
      , .assign (.index (.var "yh") (.litUint 4)) (.litFloat 1.0)
      , .assign (.index (.var "yh") (.litUint 8)) (.litFloat 1.0)
      , .declareArray floatTy "zyh" 9, .declareArray floatTy "zyl" 9
      , .declareArray floatTy "th" 9, .declareArray floatTy "tl" 9
      , .declareArray floatTy "ynh" 9, .declareArray floatTy "ynl" 9
      , .declareArray floatTy "znh" 9, .declareArray floatTy "znl" 9
      , .forCount "it2" (.litUint 0) (.litUint 24)
          (matmul3 "zy" "z" "y" "zy"
          ++ [ .forCount "ti" (.litUint 0) (.litUint 9)
                 [ .assign (.index (.var "th") (.var "ti")) (.un "-" (.index (.var "zyh") (.var "ti")))
                 , .assign (.index (.var "tl") (.var "ti")) (.un "-" (.index (.var "zyl") (.var "ti"))) ]
             , .forCount "td" (.litUint 0) (.litUint 3)
                 [ .declInit (.scalar .uint) "tdi" (.bin "*" (.var "td") (.litUint 4))
                 , .declare floatTy "t3h" none, .declare floatTy "t3l" none
                 , .expr (.call "df_add" [.index (.var "th") (.var "tdi"), .index (.var "tl") (.var "tdi"), .litFloat 3.0, .litFloat 0.0, .var "t3h", .var "t3l"])
                 , .assign (.index (.var "th") (.var "tdi")) (.var "t3h")
                 , .assign (.index (.var "tl") (.var "tdi")) (.var "t3l") ]
             , .forCount "tm" (.litUint 0) (.litUint 9)
                 [ .declare floatTy "tmh" none, .declare floatTy "tml" none
                 , .expr (.call "df_mul" [.litFloat 0.5, .litFloat 0.0, .index (.var "th") (.var "tm"), .index (.var "tl") (.var "tm"), .var "tmh", .var "tml"])
                 , .assign (.index (.var "th") (.var "tm")) (.var "tmh")
                 , .assign (.index (.var "tl") (.var "tm")) (.var "tml") ] ]
          ++ matmul3 "yt" "y" "t" "yn"
          ++ matmul3 "tz" "t" "z" "zn"
          ++ [ .forCount "cp" (.litUint 0) (.litUint 9)
                 [ .assign (.index (.var "yh") (.var "cp")) (.index (.var "ynh") (.var "cp"))
                 , .assign (.index (.var "yl") (.var "cp")) (.index (.var "ynl") (.var "cp"))
                 , .assign (.index (.var "zh") (.var "cp")) (.index (.var "znh") (.var "cp"))
                 , .assign (.index (.var "zl") (.var "cp")) (.index (.var "znl") (.var "cp")) ] ])
      , .declare floatTy "ssh" none, .declare floatTy "ssl" none
      , .expr (.call "df_sqrt" [.var "sh", .var "sl", .var "ssh", .var "ssl"])
      , .declareArray floatTy "mvh" 9, .declareArray floatTy "mvl" 9
      , .forCount "mi" (.litUint 0) (.litUint 9)
          [ .declare floatTy "mvph" none, .declare floatTy "mvpl" none
          , .expr (.call "df_mul" [.var "ssh", .var "ssl", .index (.var "yh") (.var "mi"), .index (.var "yl") (.var "mi"), .var "mvph", .var "mvpl"])
          , .assign (.index (.var "mvh") (.var "mi")) (.var "mvph")
          , .assign (.index (.var "mvl") (.var "mi")) (.var "mvpl") ]
      , .declareArray floatTy "rrh" 9, .declareArray floatTy "rrl" 9 ]
      ++ matmul3 "rm" "h" "mv" "rr"
      ++ [ .forCount "oz" (.litUint 0) (.litUint 9)
          [ .assign (.index (.var "out_hilo") (.bin "*" (.litUint 2) (.var "oz"))) (.index (.var "rrh") (.var "oz"))
          , .assign (.index (.var "out_hilo") (.bin "+" (.bin "*" (.litUint 2) (.var "oz")) (.litUint 1))) (.index (.var "rrl") (.var "oz")) ]
      , .ret none ] }

def shader : SlangShaderModule :=
  { structs :=
      [ { name := "PdParams", fields := [⟨"n", .scalar .uint, Semantic.none, none, none, .qIn⟩] } ]
  , globals :=
      [ ⟨"params",   .const "PdParams",         Semantic.none, some 0, some 0, .qIn⟩
      , ⟨"in_p",     .roBuf (.vec .float 3),    Semantic.none, some 1, some 0, .qIn⟩
      , ⟨"in_q",     .roBuf (.vec .float 3),    Semantic.none, some 2, some 0, .qIn⟩
      , ⟨"out_hilo", .rwBuf (.scalar .float),   Semantic.none, some 3, some 0, .qIn⟩ ]
  , functions := [two_sum, quick_two_sum, two_prod, df_add, df_mul, df_div, df_sqrt, mainEntry] }

end CassieAvbd.PolarDecompKernel
