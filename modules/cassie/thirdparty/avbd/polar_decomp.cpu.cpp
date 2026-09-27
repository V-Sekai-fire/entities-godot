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


#line 300
struct GlobalParams_0
{
    PdParams_0* params_0;
    StructuredBuffer<Vector<float, 3> > in_p_0;
    StructuredBuffer<Vector<float, 3> > in_q_0;
    RWStructuredBuffer<float> out_hilo_0;
};


#line 300
struct KernelContext_0
{
    GlobalParams_0* globalParams_0;
};


#line 33
static void two_prod_0(float a_0, float b_0, float * hi_0, float * lo_0)
{

#line 34
    float h_0 = a_0 * b_0;
    *hi_0 = h_0;
    *lo_0 = (F32_fma((a_0), (b_0), (- h_0)));
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


#line 40
static void df_add_0(float x_hi_0, float x_lo_0, float y_hi_0, float y_lo_0, float * z_hi_0, float * z_lo_0)
{

#line 41
    float sh_0;
    float sl_0;
    two_sum_0(x_hi_0, y_hi_0, &sh_0, &sl_0);


    quick_two_sum_0(sh_0, sl_0 + (x_lo_0 + y_lo_0), z_hi_0, z_lo_0);
    return;
}

static void df_mul_0(float a_hi_0, float a_lo_0, float b_hi_0, float b_lo_0, float * z_hi_1, float * z_lo_1)
{

#line 51
    float p_hi_0;
    float p_lo_0;
    two_prod_0(a_hi_0, b_hi_0, &p_hi_0, &p_lo_0);


    quick_two_sum_0(p_hi_0, p_lo_0 + (a_hi_0 * b_lo_0 + a_lo_0 * b_hi_0), z_hi_1, z_lo_1);
    return;
}

static void df_div_0(float a_hi_1, float a_lo_1, float b_hi_1, float b_lo_1, float * z_hi_2, float * z_lo_2)
{

#line 61
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

#line 74
    if(a_hi_2 <= 0.0f)
    {

#line 75
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

#line 92
    uint32_t a_3;

#line 92
    uint32_t b_3;

#line 92
    uint32_t k_0;

#line 92
    float acch_0;

#line 92
    float accl_0;

#line 92
    uint32_t zyi_0;

#line 92
    uint32_t zyj_0;

#line 92
    uint32_t zyk_0;

#line 92
    uint32_t tm_0;

#line 92
    uint32_t ytj_0;

#line 92
    uint32_t ytk_0;

#line 92
    KernelContext_0 kernelContext_0;

#line 92
    (&kernelContext_0)->globalParams_0 = (slang_bit_cast<GlobalParams_0*>(globalParams_1));
    FixedArray<float, 9>  hh_0;
    FixedArray<float, 9>  hl_0;

#line 94
    uint32_t z_0 = 0U;
    for(;;)
    {

#line 95
        if(z_0 < 9U)
        {
        }
        else
        {

#line 95
            break;
        }

#line 96
        hh_0[z_0] = 0.0f;
        hl_0[z_0] = 0.0f;

#line 95
        z_0 = z_0 + 1U;

#line 95
    }

#line 95
    uint32_t i_0 = 0U;



    for(;;)
    {

#line 99
        if(i_0 < ((&kernelContext_0)->globalParams_0->params_0->n_0))
        {
        }
        else
        {

#line 99
            break;
        }

#line 100
        Vector<float, 3>  _S2 = (&kernelContext_0)->globalParams_0->in_p_0.Load(i_0);
        Vector<float, 3>  _S3 = (&kernelContext_0)->globalParams_0->in_q_0.Load(i_0);

#line 101
        a_3 = 0U;
        for(;;)
        {

#line 102
            if(a_3 < 3U)
            {
            }
            else
            {

#line 102
                break;
            }

#line 102
            b_3 = 0U;
            for(;;)
            {

#line 103
                if(b_3 < 3U)
                {
                }
                else
                {

#line 103
                    break;
                }

#line 104
                uint32_t idx_0 = a_3 * 3U + b_3;


                float ph_0;
                float pl_0;
                two_prod_0(_slang_vector_get_element(_S3, a_3), _slang_vector_get_element(_S2, b_3), &ph_0, &pl_0);
                float nh_0;
                float nl_0;
                df_add_0(hh_0[idx_0], hl_0[idx_0], ph_0, pl_0, &nh_0, &nl_0);
                hh_0[idx_0] = nh_0;
                hl_0[idx_0] = nl_0;

#line 103
                b_3 = b_3 + 1U;

#line 103
            }

#line 102
            a_3 = a_3 + 1U;

#line 102
        }

#line 99
        i_0 = i_0 + 1U;

#line 99
    }

#line 118
    FixedArray<float, 9>  mh_0;
    FixedArray<float, 9>  ml_0;

#line 119
    a_3 = 0U;
    for(;;)
    {

#line 120
        if(a_3 < 3U)
        {
        }
        else
        {

#line 120
            break;
        }

#line 120
        b_3 = 0U;
        for(;;)
        {

#line 121
            if(b_3 < 3U)
            {
            }
            else
            {

#line 121
                break;
            }

#line 121
            acch_0 = 0.0f;

#line 121
            accl_0 = 0.0f;

#line 121
            k_0 = 0U;


            for(;;)
            {

#line 124
                if(k_0 < 3U)
                {
                }
                else
                {

#line 124
                    break;
                }

#line 125
                uint32_t _S4 = k_0 * 3U;

#line 125
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

#line 124
                uint32_t k_1 = k_0 + 1U;

#line 124
                acch_0 = _S5;

#line 124
                accl_0 = _S6;

#line 124
                k_0 = k_1;

#line 124
            }

#line 136
            uint32_t mij_0 = a_3 * 3U + b_3;
            mh_0[mij_0] = acch_0;
            ml_0[mij_0] = accl_0;

#line 121
            b_3 = b_3 + 1U;

#line 121
        }

#line 120
        a_3 = a_3 + 1U;

#line 120
    }

#line 141
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

#line 153
    b_3 = 0U;
    for(;;)
    {

#line 154
        if(b_3 < 9U)
        {
        }
        else
        {

#line 154
            break;
        }

#line 155
        float zph_0;
        float zpl_0;
        df_mul_0(sh_1, sl_1, mh_0[b_3], ml_0[b_3], &zph_0, &zpl_0);
        zh_0[b_3] = zph_0;
        zl_0[b_3] = zpl_0;
        yh_0[b_3] = 0.0f;
        yl_0[b_3] = 0.0f;

#line 154
        b_3 = b_3 + 1U;

#line 154
    }

#line 163
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

#line 173
    k_0 = 0U;
    for(;;)
    {

#line 174
        if(k_0 < 24U)
        {
        }
        else
        {

#line 174
            break;
        }

#line 174
        zyi_0 = 0U;
        for(;;)
        {

#line 175
            if(zyi_0 < 3U)
            {
            }
            else
            {

#line 175
                break;
            }

#line 175
            zyj_0 = 0U;
            for(;;)
            {

#line 176
                if(zyj_0 < 3U)
                {
                }
                else
                {

#line 176
                    break;
                }

#line 176
                acch_0 = 0.0f;

#line 176
                accl_0 = 0.0f;

#line 176
                zyk_0 = 0U;


                for(;;)
                {

#line 179
                    if(zyk_0 < 3U)
                    {
                    }
                    else
                    {

#line 179
                        break;
                    }

#line 180
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

#line 179
                    uint32_t zyk_1 = zyk_0 + 1U;

#line 179
                    acch_0 = _S7;

#line 179
                    accl_0 = _S8;

#line 179
                    zyk_0 = zyk_1;

#line 179
                }

#line 191
                uint32_t zyij_0 = zyi_0 * 3U + zyj_0;
                zyh_0[zyij_0] = acch_0;
                zyl_0[zyij_0] = accl_0;

#line 176
                zyj_0 = zyj_0 + 1U;

#line 176
            }

#line 175
            zyi_0 = zyi_0 + 1U;

#line 175
        }

#line 175
        zyj_0 = 0U;

#line 196
        for(;;)
        {

#line 196
            if(zyj_0 < 9U)
            {
            }
            else
            {

#line 196
                break;
            }

#line 197
            th_0[zyj_0] = - zyh_0[zyj_0];
            tl_0[zyj_0] = - zyl_0[zyj_0];

#line 196
            zyj_0 = zyj_0 + 1U;

#line 196
        }

#line 196
        zyk_0 = 0U;



        for(;;)
        {

#line 200
            if(zyk_0 < 3U)
            {
            }
            else
            {

#line 200
                break;
            }

#line 201
            uint32_t tdi_0 = zyk_0 * 4U;
            float t3h_0;
            float t3l_0;
            df_add_0(th_0[tdi_0], tl_0[tdi_0], 3.0f, 0.0f, &t3h_0, &t3l_0);
            th_0[tdi_0] = t3h_0;
            tl_0[tdi_0] = t3l_0;

#line 200
            zyk_0 = zyk_0 + 1U;

#line 200
        }

#line 200
        tm_0 = 0U;

#line 208
        for(;;)
        {

#line 208
            if(tm_0 < 9U)
            {
            }
            else
            {

#line 208
                break;
            }

#line 209
            float tmh_0;
            float tml_0;
            df_mul_0(0.5f, 0.0f, th_0[tm_0], tl_0[tm_0], &tmh_0, &tml_0);
            th_0[tm_0] = tmh_0;
            tl_0[tm_0] = tml_0;

#line 208
            tm_0 = tm_0 + 1U;

#line 208
        }

#line 208
        uint32_t yti_0 = 0U;

#line 215
        for(;;)
        {

#line 215
            if(yti_0 < 3U)
            {
            }
            else
            {

#line 215
                break;
            }

#line 215
            ytj_0 = 0U;
            for(;;)
            {

#line 216
                if(ytj_0 < 3U)
                {
                }
                else
                {

#line 216
                    break;
                }

#line 216
                acch_0 = 0.0f;

#line 216
                accl_0 = 0.0f;

#line 216
                ytk_0 = 0U;


                for(;;)
                {

#line 219
                    if(ytk_0 < 3U)
                    {
                    }
                    else
                    {

#line 219
                        break;
                    }

#line 220
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

#line 219
                    uint32_t ytk_1 = ytk_0 + 1U;

#line 219
                    acch_0 = _S9;

#line 219
                    accl_0 = _S10;

#line 219
                    ytk_0 = ytk_1;

#line 219
                }

#line 231
                uint32_t ytij_0 = yti_0 * 3U + ytj_0;
                ynh_0[ytij_0] = acch_0;
                ynl_0[ytij_0] = accl_0;

#line 216
                ytj_0 = ytj_0 + 1U;

#line 216
            }

#line 215
            yti_0 = yti_0 + 1U;

#line 215
        }

#line 215
        ytj_0 = 0U;

#line 236
        for(;;)
        {

#line 236
            if(ytj_0 < 3U)
            {
            }
            else
            {

#line 236
                break;
            }

#line 236
            ytk_0 = 0U;
            for(;;)
            {

#line 237
                if(ytk_0 < 3U)
                {
                }
                else
                {

#line 237
                    break;
                }

#line 237
                acch_0 = 0.0f;

#line 237
                accl_0 = 0.0f;

#line 237
                uint32_t tzk_0 = 0U;


                for(;;)
                {

#line 240
                    if(tzk_0 < 3U)
                    {
                    }
                    else
                    {

#line 240
                        break;
                    }

#line 241
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

#line 240
                    uint32_t tzk_1 = tzk_0 + 1U;

#line 240
                    acch_0 = _S11;

#line 240
                    accl_0 = _S12;

#line 240
                    tzk_0 = tzk_1;

#line 240
                }

#line 252
                uint32_t tzij_0 = ytj_0 * 3U + ytk_0;
                znh_0[tzij_0] = acch_0;
                znl_0[tzij_0] = accl_0;

#line 237
                ytk_0 = ytk_0 + 1U;

#line 237
            }

#line 236
            ytj_0 = ytj_0 + 1U;

#line 236
        }

#line 236
        ytk_0 = 0U;

#line 257
        for(;;)
        {

#line 257
            if(ytk_0 < 9U)
            {
            }
            else
            {

#line 257
                break;
            }

#line 258
            yh_0[ytk_0] = ynh_0[ytk_0];
            yl_0[ytk_0] = ynl_0[ytk_0];
            zh_0[ytk_0] = znh_0[ytk_0];
            zl_0[ytk_0] = znl_0[ytk_0];

#line 257
            ytk_0 = ytk_0 + 1U;

#line 257
        }

#line 174
        k_0 = k_0 + 1U;

#line 174
    }

#line 264
    float ssh_0;
    float ssl_0;
    df_sqrt_0(sh_1, sl_1, &ssh_0, &ssl_0);
    FixedArray<float, 9>  mvh_0;
    FixedArray<float, 9>  mvl_0;

#line 268
    zyi_0 = 0U;
    for(;;)
    {

#line 269
        if(zyi_0 < 9U)
        {
        }
        else
        {

#line 269
            break;
        }

#line 270
        float mvph_0;
        float mvpl_0;
        df_mul_0(ssh_0, ssl_0, yh_0[zyi_0], yl_0[zyi_0], &mvph_0, &mvpl_0);
        mvh_0[zyi_0] = mvph_0;
        mvl_0[zyi_0] = mvpl_0;

#line 269
        zyi_0 = zyi_0 + 1U;

#line 269
    }

#line 276
    FixedArray<float, 9>  rrh_0;
    FixedArray<float, 9>  rrl_0;

#line 277
    zyj_0 = 0U;
    for(;;)
    {

#line 278
        if(zyj_0 < 3U)
        {
        }
        else
        {

#line 278
            break;
        }

#line 278
        zyk_0 = 0U;
        for(;;)
        {

#line 279
            if(zyk_0 < 3U)
            {
            }
            else
            {

#line 279
                break;
            }

#line 279
            acch_0 = 0.0f;

#line 279
            accl_0 = 0.0f;

#line 279
            tm_0 = 0U;


            for(;;)
            {

#line 282
                if(tm_0 < 3U)
                {
                }
                else
                {

#line 282
                    break;
                }

#line 283
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

#line 282
                uint32_t rmk_0 = tm_0 + 1U;

#line 282
                acch_0 = _S13;

#line 282
                accl_0 = _S14;

#line 282
                tm_0 = rmk_0;

#line 282
            }

#line 294
            uint32_t rmij_0 = zyj_0 * 3U + zyk_0;
            rrh_0[rmij_0] = acch_0;
            rrl_0[rmij_0] = accl_0;

#line 279
            zyk_0 = zyk_0 + 1U;

#line 279
        }

#line 278
        zyj_0 = zyj_0 + 1U;

#line 278
    }

#line 278
    zyk_0 = 0U;

#line 299
    for(;;)
    {

#line 299
        if(zyk_0 < 9U)
        {
        }
        else
        {

#line 299
            break;
        }

#line 300
        uint32_t _S15 = 2U * zyk_0;

#line 300
        *(&((&kernelContext_0)->globalParams_0->out_hilo_0)[_S15]) = rrh_0[zyk_0];
        *(&((&kernelContext_0)->globalParams_0->out_hilo_0)[_S15 + 1U]) = rrl_0[zyk_0];

#line 299
        zyk_0 = zyk_0 + 1U;

#line 299
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
