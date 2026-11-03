#include "../slang-prelude/slang-cpp-prelude.h"

#ifdef SLANG_PRELUDE_NAMESPACE
using namespace SLANG_PRELUDE_NAMESPACE;
#endif

#undef SLANG_PRELUDE_EXPORT
#define SLANG_PRELUDE_EXPORT

namespace cassie_slang_polar_decomp {


#line 1 "../thirdparty/avbd/polar_decomp.cpu.slang"
struct PdParams_0
{
    uint32_t n_0;
};


#line 309
struct GlobalParams_0
{
    PdParams_0* params_0;
    StructuredBuffer<Vector<float, 3> > in_p_0;
    StructuredBuffer<Vector<float, 3> > in_q_0;
    RWStructuredBuffer<float> out_hilo_0;
};


#line 309
struct KernelContext_0
{
    GlobalParams_0* globalParams_0;
};


#line 33
static void two_prod_0(float a_0, float b_0, float * hi_0, float * lo_0)
{

#line 34
    float h_0 = a_0 * b_0;
    float ca_0 = 4097.0f * a_0;
    float ah_0 = ca_0 - (ca_0 - a_0);
    float al_0 = a_0 - ah_0;
    float cb_0 = 4097.0f * b_0;
    float bh_0 = cb_0 - (cb_0 - b_0);
    float bl_0 = b_0 - bh_0;


    float e3_0 = ah_0 * bh_0 - h_0 + ah_0 * bl_0 + al_0 * bh_0;
    *hi_0 = h_0;
    *lo_0 = e3_0 + al_0 * bl_0;
    return;
}


#line 14
static void two_sum_0(float a_1, float b_1, float * hi_1, float * lo_1)
{

#line 15
    float h_1 = a_1 + b_1;
    float bb_0 = h_1 - a_1;

    float lo_a_0 = a_1 - (h_1 - bb_0);
    float lo_b_0 = b_1 - bb_0;
    *hi_1 = h_1;
    *lo_1 = lo_a_0 + lo_b_0;
    return;
}

static void quick_two_sum_0(float a_2, float b_2, float * hi_2, float * lo_2)
{

#line 26
    float h_2 = a_2 + b_2;
    float t_0 = h_2 - a_2;
    *hi_2 = h_2;
    *lo_2 = b_2 - t_0;
    return;
}


#line 49
static void df_add_0(float x_hi_0, float x_lo_0, float y_hi_0, float y_lo_0, float * z_hi_0, float * z_lo_0)
{

#line 50
    float sh_0;
    float sl_0;
    two_sum_0(x_hi_0, y_hi_0, &sh_0, &sl_0);


    quick_two_sum_0(sh_0, sl_0 + (x_lo_0 + y_lo_0), z_hi_0, z_lo_0);
    return;
}

static void df_mul_0(float a_hi_0, float a_lo_0, float b_hi_0, float b_lo_0, float * z_hi_1, float * z_lo_1)
{

#line 60
    float p_hi_0;
    float p_lo_0;
    two_prod_0(a_hi_0, b_hi_0, &p_hi_0, &p_lo_0);


    quick_two_sum_0(p_hi_0, p_lo_0 + (a_hi_0 * b_lo_0 + a_lo_0 * b_hi_0), z_hi_1, z_lo_1);
    return;
}

static void df_div_0(float a_hi_1, float a_lo_1, float b_hi_1, float b_lo_1, float * z_hi_2, float * z_lo_2)
{

#line 70
    float q1_0 = a_hi_1 / b_hi_1;
    float qb_hi_0;
    float qb_lo_0;
    df_mul_0(q1_0, 0.0f, b_hi_1, b_lo_1, &qb_hi_0, &qb_lo_0);
    float r_hi_0;
    float r_lo_0;
    df_add_0(a_hi_1, a_lo_1, - qb_hi_0, - qb_lo_0, &r_hi_0, &r_lo_0);

    quick_two_sum_0(q1_0, r_hi_0 / b_hi_1, z_hi_2, z_lo_2);
    return;
}

static void df_sqrt_0(float a_hi_2, float a_lo_2, float * z_hi_3, float * z_lo_3)
{

#line 83
    if(a_hi_2 <= 0.0f)
    {

#line 84
        *z_hi_3 = 0.0f;
        *z_lo_3 = 0.0f;
        return;
    }
    float x0_0 = (F32_sqrt((a_hi_2)));
    float xx_hi_0;
    float xx_lo_0;
    two_prod_0(x0_0, x0_0, &xx_hi_0, &xx_lo_0);
    float r_hi_1;
    float r_lo_1;
    df_add_0(a_hi_2, a_lo_2, - xx_hi_0, - xx_lo_0, &r_hi_1, &r_lo_1);

    quick_two_sum_0(x0_0, r_hi_1 / (2.0f * x0_0), z_hi_3, z_lo_3);
    return;
}


void _main_0(void* _S1, void* entryPointParams_0, void* globalParams_1)
{

#line 101
    uint32_t a_3;

#line 101
    uint32_t b_3;

#line 101
    uint32_t k_0;

#line 101
    float acch_0;

#line 101
    float accl_0;

#line 101
    uint32_t zyi_0;

#line 101
    uint32_t zyj_0;

#line 101
    uint32_t zyk_0;

#line 101
    uint32_t tm_0;

#line 101
    uint32_t ytj_0;

#line 101
    uint32_t ytk_0;

#line 101
    KernelContext_0 kernelContext_0;

#line 101
    (&kernelContext_0)->globalParams_0 = (slang_bit_cast<GlobalParams_0*>(globalParams_1));
    FixedArray<float, 9>  hh_0;
    FixedArray<float, 9>  hl_0;

#line 103
    uint32_t z_0 = 0U;
    for(;;)
    {

#line 104
        if(z_0 < 9U)
        {
        }
        else
        {

#line 104
            break;
        }

#line 105
        hh_0[z_0] = 0.0f;
        hl_0[z_0] = 0.0f;

#line 104
        z_0 = z_0 + 1U;

#line 104
    }

#line 104
    uint32_t i_0 = 0U;



    for(;;)
    {

#line 108
        if(i_0 < ((&kernelContext_0)->globalParams_0->params_0->n_0))
        {
        }
        else
        {

#line 108
            break;
        }

#line 109
        Vector<float, 3>  _S2 = (&kernelContext_0)->globalParams_0->in_p_0.Load(i_0);
        Vector<float, 3>  _S3 = (&kernelContext_0)->globalParams_0->in_q_0.Load(i_0);

#line 110
        a_3 = 0U;
        for(;;)
        {

#line 111
            if(a_3 < 3U)
            {
            }
            else
            {

#line 111
                break;
            }

#line 111
            b_3 = 0U;
            for(;;)
            {

#line 112
                if(b_3 < 3U)
                {
                }
                else
                {

#line 112
                    break;
                }

#line 113
                uint32_t idx_0 = a_3 * 3U + b_3;


                float ph_0;
                float pl_0;
                two_prod_0(_slang_vector_get_element(_S3, a_3), _slang_vector_get_element(_S2, b_3), &ph_0, &pl_0);
                float nh_0;
                float nl_0;
                df_add_0(hh_0[idx_0], hl_0[idx_0], ph_0, pl_0, &nh_0, &nl_0);
                hh_0[idx_0] = nh_0;
                hl_0[idx_0] = nl_0;

#line 112
                b_3 = b_3 + 1U;

#line 112
            }

#line 111
            a_3 = a_3 + 1U;

#line 111
        }

#line 108
        i_0 = i_0 + 1U;

#line 108
    }

#line 127
    FixedArray<float, 9>  mh_0;
    FixedArray<float, 9>  ml_0;

#line 128
    a_3 = 0U;
    for(;;)
    {

#line 129
        if(a_3 < 3U)
        {
        }
        else
        {

#line 129
            break;
        }

#line 129
        b_3 = 0U;
        for(;;)
        {

#line 130
            if(b_3 < 3U)
            {
            }
            else
            {

#line 130
                break;
            }

#line 130
            acch_0 = 0.0f;

#line 130
            accl_0 = 0.0f;

#line 130
            k_0 = 0U;


            for(;;)
            {

#line 133
                if(k_0 < 3U)
                {
                }
                else
                {

#line 133
                    break;
                }

#line 134
                uint32_t _S4 = k_0 * 3U;

#line 134
                uint32_t ki_0 = _S4 + a_3;
                uint32_t kj_0 = _S4 + b_3;
                float ph_1;
                float pl_1;
                df_mul_0(hh_0[ki_0], hl_0[ki_0], hh_0[kj_0], hl_0[kj_0], &ph_1, &pl_1);
                float nh_1;
                float nl_1;
                df_add_0(acch_0, accl_0, ph_1, pl_1, &nh_1, &nl_1);
                float _S5 = nh_1;
                float _S6 = nl_1;

#line 133
                uint32_t k_1 = k_0 + 1U;

#line 133
                acch_0 = _S5;

#line 133
                accl_0 = _S6;

#line 133
                k_0 = k_1;

#line 133
            }

#line 145
            uint32_t mij_0 = a_3 * 3U + b_3;
            mh_0[mij_0] = acch_0;
            ml_0[mij_0] = accl_0;

#line 130
            b_3 = b_3 + 1U;

#line 130
        }

#line 129
        a_3 = a_3 + 1U;

#line 129
    }

#line 150
    float trh_0;
    float trl_0;
    df_add_0(mh_0[0U], ml_0[0U], mh_0[4U], ml_0[4U], &trh_0, &trl_0);
    float tr2h_0;
    float tr2l_0;
    df_add_0(trh_0, trl_0, mh_0[8U], ml_0[8U], &tr2h_0, &tr2l_0);
    float sh_1;
    float sl_1;
    df_div_0(1.0f, 0.0f, tr2h_0, tr2l_0, &sh_1, &sl_1);
    FixedArray<float, 9>  zh_0;
    FixedArray<float, 9>  zl_0;
    FixedArray<float, 9>  yh_0;
    FixedArray<float, 9>  yl_0;

#line 162
    b_3 = 0U;
    for(;;)
    {

#line 163
        if(b_3 < 9U)
        {
        }
        else
        {

#line 163
            break;
        }

#line 164
        float zph_0;
        float zpl_0;
        df_mul_0(sh_1, sl_1, mh_0[b_3], ml_0[b_3], &zph_0, &zpl_0);
        zh_0[b_3] = zph_0;
        zl_0[b_3] = zpl_0;
        yh_0[b_3] = 0.0f;
        yl_0[b_3] = 0.0f;

#line 163
        b_3 = b_3 + 1U;

#line 163
    }

#line 172
    yh_0[0U] = 1.0f;
    yh_0[4U] = 1.0f;
    yh_0[8U] = 1.0f;
    FixedArray<float, 9>  zyh_0;
    FixedArray<float, 9>  zyl_0;
    FixedArray<float, 9>  th_0;
    FixedArray<float, 9>  tl_0;
    FixedArray<float, 9>  ynh_0;
    FixedArray<float, 9>  ynl_0;
    FixedArray<float, 9>  znh_0;
    FixedArray<float, 9>  znl_0;

#line 182
    k_0 = 0U;
    for(;;)
    {

#line 183
        if(k_0 < 24U)
        {
        }
        else
        {

#line 183
            break;
        }

#line 183
        zyi_0 = 0U;
        for(;;)
        {

#line 184
            if(zyi_0 < 3U)
            {
            }
            else
            {

#line 184
                break;
            }

#line 184
            zyj_0 = 0U;
            for(;;)
            {

#line 185
                if(zyj_0 < 3U)
                {
                }
                else
                {

#line 185
                    break;
                }

#line 185
                acch_0 = 0.0f;

#line 185
                accl_0 = 0.0f;

#line 185
                zyk_0 = 0U;


                for(;;)
                {

#line 188
                    if(zyk_0 < 3U)
                    {
                    }
                    else
                    {

#line 188
                        break;
                    }

#line 189
                    uint32_t zyia_0 = zyi_0 * 3U + zyk_0;
                    uint32_t zyib_0 = zyk_0 * 3U + zyj_0;
                    float zyph_0;
                    float zypl_0;
                    df_mul_0(zh_0[zyia_0], zl_0[zyia_0], yh_0[zyib_0], yl_0[zyib_0], &zyph_0, &zypl_0);
                    float zynh_0;
                    float zynl_0;
                    df_add_0(acch_0, accl_0, zyph_0, zypl_0, &zynh_0, &zynl_0);
                    float _S7 = zynh_0;
                    float _S8 = zynl_0;

#line 188
                    uint32_t zyk_1 = zyk_0 + 1U;

#line 188
                    acch_0 = _S7;

#line 188
                    accl_0 = _S8;

#line 188
                    zyk_0 = zyk_1;

#line 188
                }

#line 200
                uint32_t zyij_0 = zyi_0 * 3U + zyj_0;
                zyh_0[zyij_0] = acch_0;
                zyl_0[zyij_0] = accl_0;

#line 185
                zyj_0 = zyj_0 + 1U;

#line 185
            }

#line 184
            zyi_0 = zyi_0 + 1U;

#line 184
        }

#line 184
        zyj_0 = 0U;

#line 205
        for(;;)
        {

#line 205
            if(zyj_0 < 9U)
            {
            }
            else
            {

#line 205
                break;
            }

#line 206
            th_0[zyj_0] = - zyh_0[zyj_0];
            tl_0[zyj_0] = - zyl_0[zyj_0];

#line 205
            zyj_0 = zyj_0 + 1U;

#line 205
        }

#line 205
        zyk_0 = 0U;



        for(;;)
        {

#line 209
            if(zyk_0 < 3U)
            {
            }
            else
            {

#line 209
                break;
            }

#line 210
            uint32_t tdi_0 = zyk_0 * 4U;
            float t3h_0;
            float t3l_0;
            df_add_0(th_0[tdi_0], tl_0[tdi_0], 3.0f, 0.0f, &t3h_0, &t3l_0);
            th_0[tdi_0] = t3h_0;
            tl_0[tdi_0] = t3l_0;

#line 209
            zyk_0 = zyk_0 + 1U;

#line 209
        }

#line 209
        tm_0 = 0U;

#line 217
        for(;;)
        {

#line 217
            if(tm_0 < 9U)
            {
            }
            else
            {

#line 217
                break;
            }

#line 218
            float tmh_0;
            float tml_0;
            df_mul_0(0.5f, 0.0f, th_0[tm_0], tl_0[tm_0], &tmh_0, &tml_0);
            th_0[tm_0] = tmh_0;
            tl_0[tm_0] = tml_0;

#line 217
            tm_0 = tm_0 + 1U;

#line 217
        }

#line 217
        uint32_t yti_0 = 0U;

#line 224
        for(;;)
        {

#line 224
            if(yti_0 < 3U)
            {
            }
            else
            {

#line 224
                break;
            }

#line 224
            ytj_0 = 0U;
            for(;;)
            {

#line 225
                if(ytj_0 < 3U)
                {
                }
                else
                {

#line 225
                    break;
                }

#line 225
                acch_0 = 0.0f;

#line 225
                accl_0 = 0.0f;

#line 225
                ytk_0 = 0U;


                for(;;)
                {

#line 228
                    if(ytk_0 < 3U)
                    {
                    }
                    else
                    {

#line 228
                        break;
                    }

#line 229
                    uint32_t ytia_0 = yti_0 * 3U + ytk_0;
                    uint32_t ytib_0 = ytk_0 * 3U + ytj_0;
                    float ytph_0;
                    float ytpl_0;
                    df_mul_0(yh_0[ytia_0], yl_0[ytia_0], th_0[ytib_0], tl_0[ytib_0], &ytph_0, &ytpl_0);
                    float ytnh_0;
                    float ytnl_0;
                    df_add_0(acch_0, accl_0, ytph_0, ytpl_0, &ytnh_0, &ytnl_0);
                    float _S9 = ytnh_0;
                    float _S10 = ytnl_0;

#line 228
                    uint32_t ytk_1 = ytk_0 + 1U;

#line 228
                    acch_0 = _S9;

#line 228
                    accl_0 = _S10;

#line 228
                    ytk_0 = ytk_1;

#line 228
                }

#line 240
                uint32_t ytij_0 = yti_0 * 3U + ytj_0;
                ynh_0[ytij_0] = acch_0;
                ynl_0[ytij_0] = accl_0;

#line 225
                ytj_0 = ytj_0 + 1U;

#line 225
            }

#line 224
            yti_0 = yti_0 + 1U;

#line 224
        }

#line 224
        ytj_0 = 0U;

#line 245
        for(;;)
        {

#line 245
            if(ytj_0 < 3U)
            {
            }
            else
            {

#line 245
                break;
            }

#line 245
            ytk_0 = 0U;
            for(;;)
            {

#line 246
                if(ytk_0 < 3U)
                {
                }
                else
                {

#line 246
                    break;
                }

#line 246
                acch_0 = 0.0f;

#line 246
                accl_0 = 0.0f;

#line 246
                uint32_t tzk_0 = 0U;


                for(;;)
                {

#line 249
                    if(tzk_0 < 3U)
                    {
                    }
                    else
                    {

#line 249
                        break;
                    }

#line 250
                    uint32_t tzia_0 = ytj_0 * 3U + tzk_0;
                    uint32_t tzib_0 = tzk_0 * 3U + ytk_0;
                    float tzph_0;
                    float tzpl_0;
                    df_mul_0(th_0[tzia_0], tl_0[tzia_0], zh_0[tzib_0], zl_0[tzib_0], &tzph_0, &tzpl_0);
                    float tznh_0;
                    float tznl_0;
                    df_add_0(acch_0, accl_0, tzph_0, tzpl_0, &tznh_0, &tznl_0);
                    float _S11 = tznh_0;
                    float _S12 = tznl_0;

#line 249
                    uint32_t tzk_1 = tzk_0 + 1U;

#line 249
                    acch_0 = _S11;

#line 249
                    accl_0 = _S12;

#line 249
                    tzk_0 = tzk_1;

#line 249
                }

#line 261
                uint32_t tzij_0 = ytj_0 * 3U + ytk_0;
                znh_0[tzij_0] = acch_0;
                znl_0[tzij_0] = accl_0;

#line 246
                ytk_0 = ytk_0 + 1U;

#line 246
            }

#line 245
            ytj_0 = ytj_0 + 1U;

#line 245
        }

#line 245
        ytk_0 = 0U;

#line 266
        for(;;)
        {

#line 266
            if(ytk_0 < 9U)
            {
            }
            else
            {

#line 266
                break;
            }

#line 267
            yh_0[ytk_0] = ynh_0[ytk_0];
            yl_0[ytk_0] = ynl_0[ytk_0];
            zh_0[ytk_0] = znh_0[ytk_0];
            zl_0[ytk_0] = znl_0[ytk_0];

#line 266
            ytk_0 = ytk_0 + 1U;

#line 266
        }

#line 183
        k_0 = k_0 + 1U;

#line 183
    }

#line 273
    float ssh_0;
    float ssl_0;
    df_sqrt_0(sh_1, sl_1, &ssh_0, &ssl_0);
    FixedArray<float, 9>  mvh_0;
    FixedArray<float, 9>  mvl_0;

#line 277
    zyi_0 = 0U;
    for(;;)
    {

#line 278
        if(zyi_0 < 9U)
        {
        }
        else
        {

#line 278
            break;
        }

#line 279
        float mvph_0;
        float mvpl_0;
        df_mul_0(ssh_0, ssl_0, yh_0[zyi_0], yl_0[zyi_0], &mvph_0, &mvpl_0);
        mvh_0[zyi_0] = mvph_0;
        mvl_0[zyi_0] = mvpl_0;

#line 278
        zyi_0 = zyi_0 + 1U;

#line 278
    }

#line 285
    FixedArray<float, 9>  rrh_0;
    FixedArray<float, 9>  rrl_0;

#line 286
    zyj_0 = 0U;
    for(;;)
    {

#line 287
        if(zyj_0 < 3U)
        {
        }
        else
        {

#line 287
            break;
        }

#line 287
        zyk_0 = 0U;
        for(;;)
        {

#line 288
            if(zyk_0 < 3U)
            {
            }
            else
            {

#line 288
                break;
            }

#line 288
            acch_0 = 0.0f;

#line 288
            accl_0 = 0.0f;

#line 288
            tm_0 = 0U;


            for(;;)
            {

#line 291
                if(tm_0 < 3U)
                {
                }
                else
                {

#line 291
                    break;
                }

#line 292
                uint32_t rmia_0 = zyj_0 * 3U + tm_0;
                uint32_t rmib_0 = tm_0 * 3U + zyk_0;
                float rmph_0;
                float rmpl_0;
                df_mul_0(hh_0[rmia_0], hl_0[rmia_0], mvh_0[rmib_0], mvl_0[rmib_0], &rmph_0, &rmpl_0);
                float rmnh_0;
                float rmnl_0;
                df_add_0(acch_0, accl_0, rmph_0, rmpl_0, &rmnh_0, &rmnl_0);
                float _S13 = rmnh_0;
                float _S14 = rmnl_0;

#line 291
                uint32_t rmk_0 = tm_0 + 1U;

#line 291
                acch_0 = _S13;

#line 291
                accl_0 = _S14;

#line 291
                tm_0 = rmk_0;

#line 291
            }

#line 303
            uint32_t rmij_0 = zyj_0 * 3U + zyk_0;
            rrh_0[rmij_0] = acch_0;
            rrl_0[rmij_0] = accl_0;

#line 288
            zyk_0 = zyk_0 + 1U;

#line 288
        }

#line 287
        zyj_0 = zyj_0 + 1U;

#line 287
    }

#line 287
    zyk_0 = 0U;

#line 308
    for(;;)
    {

#line 308
        if(zyk_0 < 9U)
        {
        }
        else
        {

#line 308
            break;
        }

#line 309
        uint32_t _S15 = 2U * zyk_0;

#line 309
        *(&((&kernelContext_0)->globalParams_0->out_hilo_0)[_S15]) = rrh_0[zyk_0];
        *(&((&kernelContext_0)->globalParams_0->out_hilo_0)[_S15 + 1U]) = rrl_0[zyk_0];

#line 308
        zyk_0 = zyk_0 + 1U;

#line 308
    }



    return;
}

// [numthreads(1, 1, 1)]
SLANG_PRELUDE_EXPORT
void main_0_Thread(ComputeThreadVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    _main_0(varyingInput, entryPointParams, globalParams);
}
// [numthreads(1, 1, 1)]
SLANG_PRELUDE_EXPORT
void main_0_Group(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    ComputeThreadVaryingInput threadInput = {};
    threadInput.groupID = varyingInput->startGroupID;
    _main_0(&threadInput, entryPointParams, globalParams);
}
// [numthreads(1, 1, 1)]
SLANG_PRELUDE_EXPORT
void main_0(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    ComputeVaryingInput vi = *varyingInput;
    ComputeVaryingInput groupVaryingInput = {};
    for (uint32_t z = vi.startGroupID.z; z < vi.endGroupID.z; ++z)
    {
        groupVaryingInput.startGroupID.z = z;
        for (uint32_t y = vi.startGroupID.y; y < vi.endGroupID.y; ++y)
        {
            groupVaryingInput.startGroupID.y = y;
            for (uint32_t x = vi.startGroupID.x; x < vi.endGroupID.x; ++x)
            {
                groupVaryingInput.startGroupID.x = x;
                main_0_Group(&groupVaryingInput, entryPointParams, globalParams);
            }
        }
    }
}
} // namespace cassie_slang_polar_decomp
