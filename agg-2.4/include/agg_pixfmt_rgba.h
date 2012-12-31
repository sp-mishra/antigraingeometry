//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.4
// Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------
//
// Adaptation for high precision colors has been sponsored by 
// Liberty Technology Systems, Inc., visit http://lib-sys.com
//
// Liberty Technology Systems, Inc. is the provider of
// PostScript and PDF technology for software developers.
// 
//----------------------------------------------------------------------------

#ifndef AGG_PIXFMT_RGBA_INCLUDED
#define AGG_PIXFMT_RGBA_INCLUDED

#include <string.h>
#include <math.h>
#include "agg_pixfmt_base.h"
#include "agg_rendering_buffer.h"

namespace agg
{
    template<class T> inline T sd_min(T a, T b) { return (a < b) ? a : b; }
    template<class T> inline T sd_max(T a, T b) { return (a > b) ? a : b; }

    inline rgba & clip(rgba & c)
    {
        if (c.a > 1) c.a = 1; else if (c.a < 0) c.a = 0;
        if (c.r > c.a) c.r = c.a; else if (c.r < 0) c.r = 0;
        if (c.g > c.a) c.g = c.a; else if (c.g < 0) c.g = 0;
        if (c.b > c.a) c.b = c.a; else if (c.b < 0) c.b = 0;
        return c;
    }

    //=========================================================multiplier_rgba
    template<class ColorT, class Order> 
    struct multiplier_rgba : blender_base<ColorT, Order>
    {
        //--------------------------------------------------------------------
        static AGG_INLINE void premultiply(value_type* p)
        {
            value_type a = p[Order::A];
            p[Order::R] = multiply(p[Order::R], a);
            p[Order::G] = multiply(p[Order::G], a);
            p[Order::B] = multiply(p[Order::B], a);
        }


        //--------------------------------------------------------------------
        static AGG_INLINE void demultiply(value_type* p)
        {
            value_type a = p[Order::A];
            p[Order::R] = blender_base::demultiply(p[Order::R], a);
            p[Order::G] = blender_base::demultiply(p[Order::G], a);
            p[Order::B] = blender_base::demultiply(p[Order::B], a);
        }
    };

    //=====================================================apply_gamma_dir_rgba
    template<class ColorT, class Order, class GammaLut> 
    class apply_gamma_dir_rgba
    {
    public:
        typedef typename ColorT::value_type value_type;

        apply_gamma_dir_rgba(const GammaLut& gamma) : m_gamma(gamma) {}

        AGG_INLINE void operator () (value_type* p)
        {
            p[Order::R] = m_gamma.dir(p[Order::R]);
            p[Order::G] = m_gamma.dir(p[Order::G]);
            p[Order::B] = m_gamma.dir(p[Order::B]);
        }

    private:
        const GammaLut& m_gamma;
    };

    //=====================================================apply_gamma_inv_rgba
    template<class ColorT, class Order, class GammaLut> class apply_gamma_inv_rgba
    {
    public:
        typedef typename ColorT::value_type value_type;

        apply_gamma_inv_rgba(const GammaLut& gamma) : m_gamma(gamma) {}

        AGG_INLINE void operator () (value_type* p)
        {
            p[Order::R] = m_gamma.inv(p[Order::R]);
            p[Order::G] = m_gamma.inv(p[Order::G]);
            p[Order::B] = m_gamma.inv(p[Order::B]);
        }

    private:
        const GammaLut& m_gamma;
    };


    //=============================================================blender_rgba
    // Blends "plain" (i.e. non-premultiplied) colors into a premultiplied buffer.
    template<class ColorT, class Order> 
    struct blender_rgba : blender_base<ColorT, Order>
    {
        // Blend pixels using the non-premultiplied form of Alvy-Ray Smith's
        // compositing function. Since the render buffer is in fact premultiplied
        // we omit the initial premultiplication and final demultiplication.

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type cr, value_type cg, value_type cb, value_type alpha, cover_type cover)
        {
            alpha = color_type::mult_cover(alpha, cover);
            p[Order::R] = lerp(p[Order::R], cr, alpha);
            p[Order::G] = lerp(p[Order::G], cg, alpha);
            p[Order::B] = lerp(p[Order::B], cb, alpha);
            p[Order::A] = prelerp(p[Order::A], alpha, alpha);
        }
        
        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type cr, value_type cg, value_type cb, value_type alpha)
        {
            p[Order::R] = lerp(p[Order::R], cr, alpha);
            p[Order::G] = lerp(p[Order::G], cg, alpha);
            p[Order::B] = lerp(p[Order::B], cb, alpha);
            p[Order::A] = prelerp(p[Order::A], alpha, alpha);
        }

        //--------------------------------------------------------------------
        static AGG_INLINE void make_pix(value_type* p, 
            value_type cr, value_type cg, value_type cb, value_type alpha)
        {
            p[Order::R] = multiply(cr, alpha);
            p[Order::G] = multiply(cg, alpha);
            p[Order::B] = multiply(cb, alpha);
            p[Order::A] = alpha;
        }

        //--------------------------------------------------------------------
        static AGG_INLINE void set_plain_color(value_type* p, color_type c)
        {
            c.premultiply();
            p[Order::R] = c.r;
            p[Order::G] = c.g;
            p[Order::B] = c.b;
            p[Order::A] = c.a;
        }

        //--------------------------------------------------------------------
        static AGG_INLINE color_type get_plain_color(const value_type* p)
        {
            return color_type(
                p[Order::R],
                p[Order::G],
                p[Order::B],
                p[Order::A]).demultiply();
        }
    };


    //========================================================blender_rgba_pre
    // Blends premultiplied colors into a premultiplied buffer.
    template<class ColorT, class Order> 
    struct blender_rgba_pre : blender_base<ColorT, Order>
    {
        // Blend pixels using the premultiplied form of Alvy-Ray Smith's
        // compositing function. 

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type cr, value_type cg, value_type cb, value_type alpha, cover_type cover)
        {
            blend_pix(p, 
                mult_cover(cr, cover), 
                mult_cover(cg, cover), 
                mult_cover(cb, cover), 
                mult_cover(alpha, cover));
        }
        
        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type cr, value_type cg, value_type cb, value_type alpha)
        {
            p[Order::R] = prelerp(p[Order::R], cr, alpha);
            p[Order::G] = prelerp(p[Order::G], cg, alpha);
            p[Order::B] = prelerp(p[Order::B], cb, alpha);
            p[Order::A] = prelerp(p[Order::A], alpha, alpha);
        }

        //--------------------------------------------------------------------
        static AGG_INLINE void set_plain_color(value_type* p, color_type c)
        {
            c.premultiply();
            p[Order::R] = c.r;
            p[Order::G] = c.g;
            p[Order::B] = c.b;
            p[Order::A] = c.a;
        }

        //--------------------------------------------------------------------
        static AGG_INLINE color_type get_plain_color(const value_type* p)
        {
            return color_type(
                p[Order::R],
                p[Order::G],
                p[Order::B],
                p[Order::A]).demultiply();
        }
    };

    //======================================================blender_rgba_plain
    // Blends "plain" (non-premultiplied) colors into a plain (non-premultiplied) buffer.
    template<class ColorT, class Order> 
    struct blender_rgba_plain : blender_base<ColorT, Order>
    {
        // Blend pixels using the non-premultiplied form of Alvy-Ray Smith's
        // compositing function. 

        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type cr, value_type cg, value_type cb, value_type alpha, cover_type cover)
        {
            blend_pix(p, cr, cg, cb, mult_cover(alpha, cover));
        }
        
        //--------------------------------------------------------------------
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type cr, value_type cg, value_type cb, value_type alpha)
        {
            if (!is_empty(alpha))
            {
                calc_type a = p[Order::A];
                calc_type r = multiply(p[Order::R], a);
                calc_type g =multiply(p[Order::G], a);
                calc_type b =multiply(p[Order::B], a);
                p[Order::R] = lerp(r, cr, alpha);
                p[Order::G] = lerp(g, cg, alpha);
                p[Order::B] = lerp(b, cb, alpha);
                p[Order::A] = prelerp(a, alpha, alpha);
                multiplier_rgba<ColorT, Order>::demultiply(p);
            }
        }

        //--------------------------------------------------------------------
        static AGG_INLINE void set_plain_color(value_type* p, color_type c)
        {
            p[Order::R] = c.r;
            p[Order::G] = c.g;
            p[Order::B] = c.b;
            p[Order::A] = c.a;
        }

        //--------------------------------------------------------------------
        static AGG_INLINE color_type get_plain_color(const value_type* p)
        {
            return color_type(
                p[Order::R],
                p[Order::G],
                p[Order::B],
                p[Order::A]);
        }
    };

    // SVG compositing operations.
    // For specifications, see http://www.w3.org/TR/SVGCompositing/

    //=========================================================comp_op_rgba_clear
    template<class ColorT, class Order> 
    struct comp_op_rgba_clear : blender_base<ColorT, Order>
    {
        // Dca' = 0
        // Da'  = 0
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            if (cover >= cover_full)
            {
                p[0] = p[1] = p[2] = p[3] = empty_value(); 
            }
            else if (cover > cover_none)
            {
                set(p, get(p, cover_full - cover));
            }
        }
    };

    //===========================================================comp_op_rgba_src
    template<class ColorT, class Order> 
    struct comp_op_rgba_src : blender_base<ColorT, Order>
    {
        // Dca' = Sca
        // Da'  = Sa
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            if (cover >= cover_full)
            {
                set(p, r, g, b, a);
            }
            else
            {
                rgba s = get(r, g, b, a, cover);
                rgba d = get(p, cover_full - cover);
                d.r += s.r;
                d.g += s.g;
                d.b += s.b;
                d.a += s.a;
                set(p, d);
            }
        }
    };

    //===========================================================comp_op_rgba_dst
    template<class ColorT, class Order> 
    struct comp_op_rgba_dst : blender_base<ColorT, Order>
    {
        // Dca' = Dca.Sa + Dca.(1 - Sa) = Dca
        // Da'  = Da.Sa + Da.(1 - Sa) = Da
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            // Well, that was easy!
        }
    };

    //======================================================comp_op_rgba_src_over
    template<class ColorT, class Order> 
    struct comp_op_rgba_src_over : blender_base<ColorT, Order>
    {
        // Dca' = Sca + Dca.(1 - Sa) = Dca + Sca - Dca.Sa
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
#if 1
            blender_rgba_pre<ColorT, Order>::blend_pix(p, r, g, b, a, cover);
#else
            rgba s = get(r, g, b, a, cover);
            rgba d = get(p);
            d.r += s.r - d.r * s.a;
            d.g += s.g - d.g * s.a;
            d.b += s.b - d.b * s.a;
            d.a += s.a - d.a * s.a;
            set(p, d);
#endif
        }
    };

    //======================================================comp_op_rgba_dst_over
    template<class ColorT, class Order> 
    struct comp_op_rgba_dst_over : blender_base<ColorT, Order>
    {
        // Dca' = Dca + Sca.(1 - Da)
        // Da'  = Sa + Da - Sa.Da = Da + Sa.(1 - Da)
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            rgba d = get(p);
            double d1a = 1 - d.a;
            d.r += s.r * d1a;
            d.g += s.g * d1a;
            d.b += s.b * d1a;
            d.a += s.a * d1a;
            set(p, d);
        }
    };

    //======================================================comp_op_rgba_src_in
    template<class ColorT, class Order> 
    struct comp_op_rgba_src_in : blender_base<ColorT, Order>
    {
        // Dca' = Sca.Da
        // Da'  = Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            double da = to_double(p[Order::A]);
            if (da > 0)
            {
                rgba s = get(r, g, b, a, cover);
                rgba d = get(p, cover_full - cover);
                d.r += s.r * da;
                d.g += s.g * da;
                d.b += s.b * da;
                d.a += s.a * da;
                set(p, d);
            }
        }
    };

    //======================================================comp_op_rgba_dst_in
    template<class ColorT, class Order> 
    struct comp_op_rgba_dst_in : blender_base<ColorT, Order>
    {
        // Dca' = Dca.Sa
        // Da'  = Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            double sa = to_double(a);
            rgba d = get(p, cover_full - cover);
            rgba d2 = get(p, cover);
            d.r += d2.r * sa;
            d.g += d2.g * sa;
            d.b += d2.b * sa;
            d.a += d2.a * sa;
            set(p, d);
        }
    };

    //======================================================comp_op_rgba_src_out
    template<class ColorT, class Order> 
    struct comp_op_rgba_src_out : blender_base<ColorT, Order>
    {
        // Dca' = Sca.(1 - Da)
        // Da'  = Sa.(1 - Da) 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            rgba d = get(p, cover_full - cover);
            double d1a = 1 - to_double(p[Order::A]);
            d.r += s.r * d1a;
            d.g += s.g * d1a;
            d.b += s.b * d1a;
            d.a += s.a * d1a;
            set(p, d);
        }
    };

    //======================================================comp_op_rgba_dst_out
    template<class ColorT, class Order> 
    struct comp_op_rgba_dst_out : blender_base<ColorT, Order>
    {
        // Dca' = Dca.(1 - Sa) 
        // Da'  = Da.(1 - Sa) 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba d = get(p, cover_full - cover);
            rgba dc = get(p, cover);
            double s1a = 1 - to_double(a);
            d.r += dc.r * s1a;
            d.g += dc.g * s1a;
            d.b += dc.b * s1a;
            d.a += dc.a * s1a;
            set(p, d);
        }
    };

    //=====================================================comp_op_rgba_src_atop
    template<class ColorT, class Order> 
    struct comp_op_rgba_src_atop : blender_base<ColorT, Order>
    {
        // Dca' = Sca.Da + Dca.(1 - Sa)
        // Da'  = Da
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            rgba d = get(p);
            double s1a = 1 - s.a;
            d.r = s.r * d.a + d.r * s1a;
            d.g = s.g * d.a + d.g * s1a;
            d.b = s.b * d.a + d.g * s1a;
            set(p, d);
        }
    };

    //=====================================================comp_op_rgba_dst_atop
    template<class ColorT, class Order> 
    struct comp_op_rgba_dst_atop : blender_base<ColorT, Order>
    {
        // Dca' = Dca.Sa + Sca.(1 - Da)
        // Da'  = Sa 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba sc = get(r, g, b, a, cover);
            rgba dc = get(p, cover);
            rgba d = get(p, cover_full - cover);
            double sa = to_double(a);
            double d1a = 1 - to_double(p[Order::A]);
            d.r += dc.r * sa + sc.r * d1a;
            d.g += dc.g * sa + sc.g * d1a;
            d.b += dc.b * sa + sc.b * d1a;
            d.a += sc.a;
            set(p, d);
        }
    };

    //=========================================================comp_op_rgba_xor
    template<class ColorT, class Order> 
    struct comp_op_rgba_xor : blender_base<ColorT, Order>
    {
        // Dca' = Sca.(1 - Da) + Dca.(1 - Sa)
        // Da'  = Sa + Da - 2.Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            rgba d = get(p);
            double s1a = 1 - s.a;
            double d1a = 1 - to_double(p[Order::A]);
            d.r = s.r * d1a + d.r * s1a;
            d.g = s.g * d1a + d.g * s1a;
            d.b = s.b * d1a + d.b * s1a;
            d.a = s.a + d.a - 2 * s.a * d.a;
            set(p, d);
        }
    };

    //=========================================================comp_op_rgba_plus
    template<class ColorT, class Order> 
    struct comp_op_rgba_plus : blender_base<ColorT, Order>
    {
        // Dca' = Sca + Dca
        // Da'  = Sa + Da 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            if (s.a > 0)
            {
                rgba d = get(p);
                d.a = sd_min(d.a + s.a, 1.0);
                d.r = sd_min(d.r + s.r, d.a);
                d.g = sd_min(d.g + s.g, d.a);
                d.b = sd_min(d.b + s.b, d.a);
                set(p, clip(d));
            }
        }
    };

    //========================================================comp_op_rgba_minus
    // Note: not included in SVG spec.
    template<class ColorT, class Order> 
    struct comp_op_rgba_minus : blender_base<ColorT, Order>
    {
        // Dca' = Dca - Sca
        // Da' = 1 - (1 - Sa).(1 - Da) = Da + Sa - Sa.Da
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            if (s.a > 0)
            {
                rgba d = get(p);
                d.a += s.a - s.a * d.a;
                d.r = sd_max(d.r - s.r, 0.0);
                d.g = sd_max(d.g - s.g, 0.0);
                d.b = sd_max(d.b - s.b, 0.0);
                set(p, clip(d));
            }
        }
    };

    //=====================================================comp_op_rgba_multiply
    template<class ColorT, class Order> 
    struct comp_op_rgba_multiply : blender_base<ColorT, Order>
    {
        // Dca' = Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            if (s.a > 0)
            {
                rgba d = get(p);
                double s1a = 1 - s.a;
                double d1a = 1 - d.a;
                d.r = s.r * d.r + s.r * d1a + d.r * s1a;
                d.g = s.g * d.g + s.g * d1a + d.g * s1a;
                d.b = s.b * d.b + s.b * d1a + d.b * s1a;
                d.a += s.a - s.a * d.a;
                set(p, clip(d));
            }
        }
    };

    //=====================================================comp_op_rgba_screen
    template<class ColorT, class Order> 
    struct comp_op_rgba_screen : blender_base<ColorT, Order>
    {
        // Dca' = Sca + Dca - Sca.Dca
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            if (s.a > 0)
            {
                rgba d = get(p);
                d.r += s.r - s.r * d.r;
                d.g += s.g - s.g * d.g;
                d.b += s.b - s.b * d.b;
                d.a += s.a - s.a * d.a;
                set(p, clip(d));
            }
        }
    };

    //=====================================================comp_op_rgba_overlay
    template<class ColorT, class Order> 
    struct comp_op_rgba_overlay : blender_base<ColorT, Order>
    {
        // if 2.Dca <= Da
        //   Dca' = 2.Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
        // otherwise
        //   Dca' = Sa.Da - 2.(Da - Dca).(Sa - Sca) + Sca.(1 - Da) + Dca.(1 - Sa)
        // 
        // Da' = Sa + Da - Sa.Da
        static AGG_INLINE double calc(double dca, double sca, double da, double sa, double sada, double d1a, double s1a)
        {
            return (2 * dca <= da) ? 
                2 * sca * dca + sca * d1a + dca * s1a : 
                sada - 2 * (da - dca) * (sa - sca) + sca * d1a + dca * s1a;
        }

        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            if (s.a > 0)
            {
                rgba d = get(p);
                double d1a = 1 - d.a;
                double s1a = 1 - s.a;
                double sada = s.a * d.a;
                d.r = calc(d.r, s.r, d.a, s.a, sada, d1a, s1a);
                d.g = calc(d.g, s.g, d.a, s.a, sada, d1a, s1a);
                d.b = calc(d.b, s.b, d.a, s.a, sada, d1a, s1a);
                d.a += s.a - s.a * d.a;
                set(p, clip(d));
            }
        }
    };

    //=====================================================comp_op_rgba_darken
    template<class ColorT, class Order> 
    struct comp_op_rgba_darken : blender_base<ColorT, Order>
    {
        // Dca' = min(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            if (s.a > 0)
            {
                rgba d = get(p);
                double d1a = 1 - d.a;
                double s1a = 1 - s.a;
                d.r = sd_min(s.r * d.a, d.r * s.a) + s.r * d1a + d.r * s1a;
                d.g = sd_min(s.g * d.a, d.g * s.a) + s.g * d1a + d.g * s1a;
                d.b = sd_min(s.b * d.a, d.b * s.a) + s.b * d1a + d.b * s1a;
                d.a += s.a - s.a * d.a;
                set(p, clip(d));
            }
        }
    };

    //=====================================================comp_op_rgba_lighten
    template<class ColorT, class Order> 
    struct comp_op_rgba_lighten : blender_base<ColorT, Order>
    {
        // Dca' = max(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            if (s.a > 0)
            {
                rgba d = get(p);
                double d1a = 1 - d.a;
                double s1a = 1 - s.a;
                d.r = sd_max(s.r * d.a, d.r * s.a) + s.r * d1a + d.r * s1a;
                d.g = sd_max(s.g * d.a, d.g * s.a) + s.g * d1a + d.g * s1a;
                d.b = sd_max(s.b * d.a, d.b * s.a) + s.b * d1a + d.b * s1a;
                d.a += s.a - s.a * d.a;
                set(p, clip(d));
            }
        }
    };

    //=====================================================comp_op_rgba_color_dodge
    template<class ColorT, class Order> 
    struct comp_op_rgba_color_dodge : blender_base<ColorT, Order>
    {
        // if Sca == Sa and Dca == 0
        //     Dca' = Sca.(1 - Da) + Dca.(1 - Sa) = Sca.(1 - Da)
        // otherwise if Sca == Sa
        //     Dca' = Sa.Da + Sca.(1 - Da) + Dca.(1 - Sa)
        // otherwise if Sca < Sa
        //     Dca' = Sa.Da.min(1, Dca/Da.Sa/(Sa - Sca)) + Sca.(1 - Da) + Dca.(1 - Sa)
        //
        // Da'  = Sa + Da - Sa.Da
        static AGG_INLINE double calc(double dca, double sca, double da, double sa, double sada, double d1a, double s1a)
        {
            if (sca < sa) return sada * sd_min(1.0, (dca / da) * sa / (sa - sca)) + sca * d1a + dca * s1a;
            if (dca > 0) return sada + sca * d1a + dca * s1a;
            return sca * d1a;
        }

        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            if (s.a > 0)
            {
                rgba d = get(p);
                if (d.a > 0)
                {
                    double sada = s.a * d.a;
                    double s1a = 1 - s.a;
                    double d1a = 1 - d.a;
                    d.r = calc(d.r, s.r, d.a, s.a, sada, d1a, s1a);
                    d.g = calc(d.g, s.g, d.a, s.a, sada, d1a, s1a);
                    d.b = calc(d.b, s.b, d.a, s.a, sada, d1a, s1a);
                    d.a += s.a - s.a * d.a;
                    set(p, clip(d));
                }
                else set(p, s);
            }
        }
    };

    //=====================================================comp_op_rgba_color_burn
    template<class ColorT, class Order> 
    struct comp_op_rgba_color_burn : blender_base<ColorT, Order>
    {
        // if Sca == 0 and Dca == Da
        //   Dca' = Sa.Da + Dca.(1 - Sa)
        // otherwise if Sca == 0
        //   Dca' = Dca.(1 - Sa)
        // otherwise if Sca > 0
        //   Dca' =  Sa.Da.(1 - min(1, (1 - Dca/Da).Sa/Sca)) + Sca.(1 - Da) + Dca.(1 - Sa)
        static AGG_INLINE double calc(double dca, double sca, double da, double sa, double sada, double d1a, double s1a)
        {
            if (sca > 0) return sada * (1 - sd_min(1.0, (1 - dca / da) * sa / sca)) + sca * d1a + dca * s1a;
            if (dca > da) return sada + dca * s1a;
            return dca * s1a;
        }

        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            if (s.a > 0)
            {
                rgba d = get(p);
                if (d.a > 0)
                {
                    double sada = s.a * d.a;
                    double s1a = 1 - s.a;
                    double d1a = 1 - d.a;
                    d.r = calc(d.r, s.r, d.a, s.a, sada, d1a, s1a);
                    d.g = calc(d.g, s.g, d.a, s.a, sada, d1a, s1a);
                    d.b = calc(d.b, s.b, d.a, s.a, sada, d1a, s1a);
                    d.a += s.a - sada;
                    set(p, clip(d));
                }
                else set(p, s);
            }
        }
    };

    //=====================================================comp_op_rgba_hard_light
    template<class ColorT, class Order> 
    struct comp_op_rgba_hard_light : blender_base<ColorT, Order>
    {
        // if 2.Sca < Sa
        //    Dca' = 2.Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
        // otherwise
        //    Dca' = Sa.Da - 2.(Da - Dca).(Sa - Sca) + Sca.(1 - Da) + Dca.(1 - Sa)
        // 
        // Da'  = Sa + Da - Sa.Da
        static AGG_INLINE double calc(double dca, double sca, double da, double sa, double sada, double d1a, double s1a)
        {
            return (2 * sca < sa) ? 
                2 * sca * dca + sca * d1a + dca * s1a : 
                sada - 2 * (da - dca) * (sa - sca) + sca * d1a + dca * s1a;
        }

        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            if (s.a > 0)
            {
                rgba d = get(p);
                double d1a = 1 - d.a;
                double s1a = 1 - s.a;
                double sada = s.a * d.a;
                d.r = calc(d.r, s.r, d.a, s.a, sada, d1a, s1a);
                d.g = calc(d.g, s.g, d.a, s.a, sada, d1a, s1a);
                d.b = calc(d.b, s.b, d.a, s.a, sada, d1a, s1a);
                d.a += s.a - sada;
                set(p, clip(d));
            }
        }
    };

    //=====================================================comp_op_rgba_soft_light
    template<class ColorT, class Order> 
    struct comp_op_rgba_soft_light : blender_base<ColorT, Order>
    {
        // if 2.Sca <= Sa
        //   Dca' = Dca.Sa - (Sa.Da - 2.Sca.Da).Dca.Sa.(Sa.Da - Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
        // otherwise if 2.Sca > Sa and 4.Dca <= Da
        //   Dca' = Dca.Sa + (2.Sca.Da - Sa.Da).((((16.Dsa.Sa - 12).Dsa.Sa + 4).Dsa.Da) - Dsa.Da) + Sca.(1 - Da) + Dca.(1 - Sa)
        // otherwise if 2.Sca > Sa and 4.Dca > Da
        //   Dca' = Dca.Sa + (2.Sca.Da - Sa.Da).((Dca.Sa)^0.5 - Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
        // 
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE double calc(double dca, double sca, double da, double sa, double sada, double d1a, double s1a)
        {
            double dcasa = dca * sa;
            if (2 * sca <= sa) return dcasa - (sada - 2 * sca * da) * dcasa * (sada - dcasa) + sca * d1a + dca * s1a;
            if (4 * dca <= da) return dcasa + (2 * sca * da - sada) * ((((16 * dcasa - 12) * dcasa + 4) * dca * da) - dca * da) + sca * d1a + dca * s1a;
            return dcasa + (2 * sca * da - sada) * (sqrt(dcasa) - dcasa) + sca * d1a + dca * s1a;
        }

        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            if (s.a > 0)
            {
                rgba d = get(p);
                if (d.a > 0)
                {
                    double sada = s.a * d.a;
                    double s1a = 1 - s.a;
                    double d1a = 1 - d.a;
                    d.r = calc(d.r, s.r, d.a, s.a, sada, d1a, s1a);
                    d.g = calc(d.g, s.g, d.a, s.a, sada, d1a, s1a);
                    d.b = calc(d.b, s.b, d.a, s.a, sada, d1a, s1a);
                    d.a += s.a - sada;
                    set(p, clip(d));
                }
                else set(p, s);
            }
        }
    };

    //=====================================================comp_op_rgba_difference
    template<class ColorT, class Order> 
    struct comp_op_rgba_difference : blender_base<ColorT, Order>
    {
        // Dca' = Sca + Dca - 2.min(Sca.Da, Dca.Sa)
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            if (s.a > 0)
            {
                rgba d = get(p);
                d.r += s.r - 2 * sd_min(s.r * d.a, d.r * s.a);
                d.g += s.g - 2 * sd_min(s.g * d.a, d.g * s.a);
                d.b += s.b - 2 * sd_min(s.b * d.a, d.b * s.a);
                d.a += s.a - s.a * d.a;
                set(p, clip(d));
            }
        }
    };

    //=====================================================comp_op_rgba_exclusion
    template<class ColorT, class Order> 
    struct comp_op_rgba_exclusion : blender_base<ColorT, Order>
    {
        // Dca' = (Sca.Da + Dca.Sa - 2.Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            rgba s = get(r, g, b, a, cover);
            if (s.a > 0)
            {
                rgba d = get(p);
                double d1a = 1 - d.a;
                double s1a = 1 - s.a;
                d.r = (s.r * d.a + d.r * s.a - 2 * s.r * d.r) + s.r * d1a + d.r * s1a;
                d.g = (s.g * d.a + d.g * s.a - 2 * s.g * d.g) + s.g * d1a + d.g * s1a;
                d.b = (s.b * d.a + d.b * s.a - 2 * s.b * d.b) + s.b * d1a + d.b * s1a;
                d.a += s.a - s.a * d.a;
                set(p, clip(d));
            }
        }
    };

#if 0
    //=====================================================comp_op_rgba_contrast
    template<class ColorT, class Order> struct comp_op_rgba_contrast
    {
        typedef ColorT color_type;
        typedef Order order_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        typedef typename color_type::long_type long_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };


        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if (cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            long_type dr = p[Order::R];
            long_type dg = p[Order::G];
            long_type db = p[Order::B];
            int       da = p[Order::A];
            long_type d2a = da >> 1;
            unsigned s2a = sa >> 1;

            int r = (int)((((dr - d2a) * int((sr - s2a)*2 + base_mask)) >> base_shift) + d2a); 
            int g = (int)((((dg - d2a) * int((sg - s2a)*2 + base_mask)) >> base_shift) + d2a); 
            int b = (int)((((db - d2a) * int((sb - s2a)*2 + base_mask)) >> base_shift) + d2a); 

            r = (r < 0) ? 0 : r;
            g = (g < 0) ? 0 : g;
            b = (b < 0) ? 0 : b;

            p[Order::R] = (value_type)((r > da) ? da : r);
            p[Order::G] = (value_type)((g > da) ? da : g);
            p[Order::B] = (value_type)((b > da) ? da : b);
        }
    };

    //=====================================================comp_op_rgba_invert
    template<class ColorT, class Order> struct comp_op_rgba_invert
    {
        typedef ColorT color_type;
        typedef Order order_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        typedef typename color_type::long_type long_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = (Da - Dca) * Sa + Dca.(1 - Sa)
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            sa = (sa * cover + 255) >> 8;
            if (sa)
            {
                calc_type da = p[Order::A];
                calc_type dr = ((da - p[Order::R]) * sa + base_mask) >> base_shift;
                calc_type dg = ((da - p[Order::G]) * sa + base_mask) >> base_shift;
                calc_type db = ((da - p[Order::B]) * sa + base_mask) >> base_shift;
                calc_type s1a = base_mask - sa;
                p[Order::R] = (value_type)(dr + ((p[Order::R] * s1a + base_mask) >> base_shift));
                p[Order::G] = (value_type)(dg + ((p[Order::G] * s1a + base_mask) >> base_shift));
                p[Order::B] = (value_type)(db + ((p[Order::B] * s1a + base_mask) >> base_shift));
                p[Order::A] = (value_type)(sa + da - ((sa * da + base_mask) >> base_shift));
            }
        }
    };

    //=================================================comp_op_rgba_invert_rgb
    template<class ColorT, class Order> struct comp_op_rgba_invert_rgb
    {
        typedef ColorT color_type;
        typedef Order order_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        typedef typename color_type::long_type long_type;
        enum base_scale_e
        { 
            base_shift = color_type::base_shift,
            base_mask  = color_type::base_mask
        };

        // Dca' = (Da - Dca) * Sca + Dca.(1 - Sa)
        // Da'  = Sa + Da - Sa.Da 
        static AGG_INLINE void blend_pix(value_type* p, 
                                         unsigned sr, unsigned sg, unsigned sb, 
                                         unsigned sa, unsigned cover)
        {
            if (cover < 255)
            {
                sr = (sr * cover + 255) >> 8;
                sg = (sg * cover + 255) >> 8;
                sb = (sb * cover + 255) >> 8;
                sa = (sa * cover + 255) >> 8;
            }
            if (sa)
            {
                calc_type da = p[Order::A];
                calc_type dr = ((da - p[Order::R]) * sr + base_mask) >> base_shift;
                calc_type dg = ((da - p[Order::G]) * sg + base_mask) >> base_shift;
                calc_type db = ((da - p[Order::B]) * sb + base_mask) >> base_shift;
                calc_type s1a = base_mask - sa;
                p[Order::R] = (value_type)(dr + ((p[Order::R] * s1a + base_mask) >> base_shift));
                p[Order::G] = (value_type)(dg + ((p[Order::G] * s1a + base_mask) >> base_shift));
                p[Order::B] = (value_type)(db + ((p[Order::B] * s1a + base_mask) >> base_shift));
                p[Order::A] = (value_type)(sa + da - ((sa * da + base_mask) >> base_shift));
            }
        }
    };
#endif


    //======================================================comp_op_table_rgba
    template<class ColorT, class Order> struct comp_op_table_rgba
    {
        typedef typename ColorT::value_type value_type;
        typedef typename ColorT::calc_type calc_type;
        typedef void (*comp_op_func_type)(value_type* p, 
                                          value_type cr, 
                                          value_type cg, 
                                          value_type cb,
                                          value_type ca,
                                          cover_type cover);
        static comp_op_func_type g_comp_op_func[];
    };

    //==========================================================g_comp_op_func
    template<class ColorT, class Order> 
    typename comp_op_table_rgba<ColorT, Order>::comp_op_func_type
    comp_op_table_rgba<ColorT, Order>::g_comp_op_func[] = 
    {
        comp_op_rgba_clear      <ColorT,Order>::blend_pix,
        comp_op_rgba_src        <ColorT,Order>::blend_pix,
        comp_op_rgba_dst        <ColorT,Order>::blend_pix,
        comp_op_rgba_src_over   <ColorT,Order>::blend_pix,
        comp_op_rgba_dst_over   <ColorT,Order>::blend_pix,
        comp_op_rgba_src_in     <ColorT,Order>::blend_pix,
        comp_op_rgba_dst_in     <ColorT,Order>::blend_pix,
        comp_op_rgba_src_out    <ColorT,Order>::blend_pix,
        comp_op_rgba_dst_out    <ColorT,Order>::blend_pix,
        comp_op_rgba_src_atop   <ColorT,Order>::blend_pix,
        comp_op_rgba_dst_atop   <ColorT,Order>::blend_pix,
        comp_op_rgba_xor        <ColorT,Order>::blend_pix,
        comp_op_rgba_plus       <ColorT,Order>::blend_pix,
        //comp_op_rgba_minus      <ColorT,Order>::blend_pix,
        comp_op_rgba_multiply   <ColorT,Order>::blend_pix,
        comp_op_rgba_screen     <ColorT,Order>::blend_pix,
        comp_op_rgba_overlay    <ColorT,Order>::blend_pix,
        comp_op_rgba_darken     <ColorT,Order>::blend_pix,
        comp_op_rgba_lighten    <ColorT,Order>::blend_pix,
        comp_op_rgba_color_dodge<ColorT,Order>::blend_pix,
        comp_op_rgba_color_burn <ColorT,Order>::blend_pix,
        comp_op_rgba_hard_light <ColorT,Order>::blend_pix,
        comp_op_rgba_soft_light <ColorT,Order>::blend_pix,
        comp_op_rgba_difference <ColorT,Order>::blend_pix,
        comp_op_rgba_exclusion  <ColorT,Order>::blend_pix,
        //comp_op_rgba_contrast   <ColorT,Order>::blend_pix,
        //comp_op_rgba_invert     <ColorT,Order>::blend_pix,
        //comp_op_rgba_invert_rgb <ColorT,Order>::blend_pix,
        0
    };


    //==============================================================comp_op_e
    enum comp_op_e
    {
        comp_op_clear,         //----comp_op_clear
        comp_op_src,           //----comp_op_src
        comp_op_dst,           //----comp_op_dst
        comp_op_src_over,      //----comp_op_src_over
        comp_op_dst_over,      //----comp_op_dst_over
        comp_op_src_in,        //----comp_op_src_in
        comp_op_dst_in,        //----comp_op_dst_in
        comp_op_src_out,       //----comp_op_src_out
        comp_op_dst_out,       //----comp_op_dst_out
        comp_op_src_atop,      //----comp_op_src_atop
        comp_op_dst_atop,      //----comp_op_dst_atop
        comp_op_xor,           //----comp_op_xor
        comp_op_plus,          //----comp_op_plus
        //comp_op_minus,         //----comp_op_minus
        comp_op_multiply,      //----comp_op_multiply
        comp_op_screen,        //----comp_op_screen
        comp_op_overlay,       //----comp_op_overlay
        comp_op_darken,        //----comp_op_darken
        comp_op_lighten,       //----comp_op_lighten
        comp_op_color_dodge,   //----comp_op_color_dodge
        comp_op_color_burn,    //----comp_op_color_burn
        comp_op_hard_light,    //----comp_op_hard_light
        comp_op_soft_light,    //----comp_op_soft_light
        comp_op_difference,    //----comp_op_difference
        comp_op_exclusion,     //----comp_op_exclusion
        //comp_op_contrast,      //----comp_op_contrast
        //comp_op_invert,        //----comp_op_invert
        //comp_op_invert_rgb,    //----comp_op_invert_rgb

        end_of_comp_op_e
    };







    //====================================================comp_op_adaptor_rgba
    template<class ColorT, class Order> 
    struct comp_op_adaptor_rgba : blender_base<ColorT, Order>
    {
        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            comp_op_table_rgba<ColorT, Order>::g_comp_op_func[op]
                (p, multiply(r, a), multiply(g, a), multiply(b, a), a, cover);
        }
    };

    //=========================================comp_op_adaptor_clip_to_dst_rgba
    template<class ColorT, class Order> 
    struct comp_op_adaptor_clip_to_dst_rgba : blender_base<ColorT, Order>
    {
        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            r = multiply(r, a);
            g = multiply(g, a);
            b = multiply(b, a);
            value_type da = p[Order::A];
            comp_op_table_rgba<ColorT, Order>::g_comp_op_func[op](p, 
                multiply(r, da), multiply(g, da), multiply(b, da), multiply(a, da), cover);
        }
    };

    //================================================comp_op_adaptor_rgba_pre
    template<class ColorT, class Order> 
    struct comp_op_adaptor_rgba_pre : blender_base<ColorT, Order>
    {
        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            comp_op_table_rgba<ColorT, Order>::g_comp_op_func[op](p, r, g, b, a, cover);
        }
    };

    //=====================================comp_op_adaptor_clip_to_dst_rgba_pre
    template<class ColorT, class Order> 
    struct comp_op_adaptor_clip_to_dst_rgba_pre : blender_base<ColorT, Order>
    {
        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            value_type da = p[Order::A];
            comp_op_table_rgba<ColorT, Order>::g_comp_op_func[op](p, 
                multiply(r, da), multiply(g, da), multiply(b, da), multiply(a, da), cover);
        }
    };

    template<class Base> 
    struct blender_base2 : 
        blender_base<
            typename Base::color_type, 
            typename Base::order_type>
    {
    };

    //=======================================================comp_adaptor_rgba
    template<class BlenderPre> 
    struct comp_adaptor_rgba : blender_base2<BlenderPre>
    {
        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
            value_type sr, value_type sg, value_type sb, value_type sa, cover_type cover)
        {
            BlenderPre::blend_pix(p, multiply(cr, ca), multiply(cg, ca), multiply(cb, ca), ca, cover);
        }
    };

    //==========================================comp_adaptor_clip_to_dst_rgba
    template<class BlenderPre> 
    struct comp_adaptor_clip_to_dst_rgba : blender_base2<BlenderPre>
    {
        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            r = multiply(r, a);
            g = multiply(g, a);
            b = multiply(b, a);
            value_type da = p[order_type::A];
            BlenderPre::blend_pix(p, multiply(r, da), multiply(g, da), multiply(b, da), multiply(a, da), cover);
        }
    };

    //======================================comp_adaptor_clip_to_dst_rgba_pre
    template<class BlenderPre> 
    struct comp_adaptor_clip_to_dst_rgba_pre : blender_base2<BlenderPre>
    {
        static AGG_INLINE void blend_pix(unsigned op, value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            unsigned da = p[order_type::A];
            BlenderPre::blend_pix(p, multiply(r, da), multiply(g, da), multiply(b, da), multiply(a, da), cover);
        }
    };






    //===============================================copy_or_blend_rgba_wrapper
    template<class Blender> 
    struct copy_or_blend_rgba_wrapper : blender_base2<Blender>
    {
        //--------------------------------------------------------------------
        static AGG_INLINE void copy_or_blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a)
        {
            if (!is_empty(a))
            {
                if (is_full(a))
                {
                    p[order_type::R] = r;
                    p[order_type::G] = g;
                    p[order_type::B] = b;
                    p[order_type::A] = full_value();
                }
                else
                {
                    Blender::blend_pix(p, r, g, b, a);
                }
            }
        }

        //--------------------------------------------------------------------
        static AGG_INLINE void copy_or_blend_pix(value_type* p, 
            value_type r, value_type g, value_type b, value_type a, cover_type cover)
        {
            if (cover == cover_mask)
            {
                copy_or_blend_pix(p, r, g, b, a);
            }
            else
            {
                if (!is_empty(a))
                {
                    Blender::blend_pix(p, r, g, b, a, cover);
                }
            }
        }
    };



    
    //=================================================pixfmt_alpha_blend_rgba
    template<class Blender, class RenBuf> 
    class pixfmt_alpha_blend_rgba
    {
    public:
        typedef pixfmt_rgba_tag pixfmt_category;
        typedef RenBuf   rbuf_type;
        typedef typename rbuf_type::row_data row_data;
        typedef Blender  blender_type;
        typedef typename blender_type::color_type color_type;
        typedef typename blender_type::order_type order_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        typedef copy_or_blend_rgba_wrapper<blender_type> cob_type;
        enum 
        {
            num_components = 4,
            pix_step = 4,
            pix_width = sizeof(value_type) * pix_step,
        };
        struct pixel_type
        {
            value_type c[num_components];
        };

        //--------------------------------------------------------------------
        pixfmt_alpha_blend_rgba() : m_rbuf(0) {}
        explicit pixfmt_alpha_blend_rgba(rbuf_type& rb) : m_rbuf(&rb) {}
        void attach(rbuf_type& rb) { m_rbuf = &rb; }

        //--------------------------------------------------------------------
        template<class PixFmt>
        bool attach(PixFmt& pixf, int x1, int y1, int x2, int y2)
        {
            rect_i r(x1, y1, x2, y2);
            if (r.clip(rect_i(0, 0, pixf.width()-1, pixf.height()-1)))
            {
                int stride = pixf.stride();
                m_rbuf->attach(pixf.pix_ptr(r.x1, stride < 0 ? r.y2 : r.y1), 
                               (r.x2 - r.x1) + 1,
                               (r.y2 - r.y1) + 1,
                               stride);
                return true;
            }
            return false;
        }

        //--------------------------------------------------------------------
        AGG_INLINE unsigned width()  const { return m_rbuf->width();  }
        AGG_INLINE unsigned height() const { return m_rbuf->height(); }
        AGG_INLINE int      stride() const { return m_rbuf->stride(); }

        //--------------------------------------------------------------------
        AGG_INLINE       int8u* row_ptr(int y)       { return m_rbuf->row_ptr(y); }
        AGG_INLINE const int8u* row_ptr(int y) const { return m_rbuf->row_ptr(y); }
        AGG_INLINE row_data     row(int y)     const { return m_rbuf->row(y); }

        //--------------------------------------------------------------------
        AGG_INLINE int8u* pix_ptr(int x, int y)
        {
            return m_rbuf->row_ptr(y) + x * pix_width;
        }

        AGG_INLINE const int8u* pix_ptr(int x, int y) const
        {
            return m_rbuf->row_ptr(y) + x * pix_width;
        }

        // Return pointer to pixel value, or null if row not allocated.
        AGG_INLINE const value_type* pix_value_ptr(int x, int y) const 
        {
            return (const value_type*)m_rbuf->row_ptr(y) + x * pix_step;
        }

        // Return pointer to pixel value, forcing row to be allocated.
        AGG_INLINE value_type* pix_value_ptr(int x, int y, unsigned len) 
        {
            return (value_type*)m_rbuf->row_ptr(x, y, len) + x * pix_step;
        }

        //--------------------------------------------------------------------
        AGG_INLINE static void set_plain_color(value_type* p, color_type c)
        {
            blender_type::set_plain_color(p, c);
        }

        //--------------------------------------------------------------------
        AGG_INLINE static color_type get_plain_color(const value_type* p)
        {
            return blender_type::get_plain_color(p);
        }

        //--------------------------------------------------------------------
        AGG_INLINE static void make_pix(value_type* p, const color_type& c)
        {
            p[order_type::R] = c.r;
            p[order_type::G] = c.g;
            p[order_type::B] = c.b;
            p[order_type::A] = c.a;
        }

        //--------------------------------------------------------------------
        AGG_INLINE static color_type make_color(const value_type* p)
        {
            return color_type(
                p[order_type::R],
                p[order_type::G],
                p[order_type::B],
                p[order_type::A]);
        }

        //--------------------------------------------------------------------
        AGG_INLINE color_type pixel(int x, int y) const
        {
            if (const value_type* p = pix_value_ptr(x, y))
            {
                return make_color(p);
            }
            return color_type::no_color();
        }

        //--------------------------------------------------------------------
        AGG_INLINE void copy_pixel(int x, int y, const color_type& c)
        {
            make_pix(pix_value_ptr(x, y, 1), c);
        }

        //--------------------------------------------------------------------
        AGG_INLINE void blend_pixel(int x, int y, const color_type& c, int8u cover)
        {
            blender_type::blend_pix(pix_value_ptr(x, y, 1), c.r, c.g, c.b, c.a, cover);
        }

        //--------------------------------------------------------------------
        AGG_INLINE void copy_hline(int x, int y, 
                                   unsigned len, 
                                   const color_type& c)
        {
            pixel_type v;
            make_pix(v.c, c);
            value_type* p = pix_value_ptr(x, y, len);
            do
            {
                *(pixel_type*)p = v;
                p += pix_step;
            }
            while (--len);
        }


        //--------------------------------------------------------------------
        AGG_INLINE void copy_vline(int x, int y,
                                   unsigned len, 
                                   const color_type& c)
        {
            pixel_type v;
            make_pix(v.c, c);
            do
            {
                value_type* p = pix_value_ptr(x, y++, 1);
                *(pixel_type*)p = v;
            }
            while (--len);
        }

        //--------------------------------------------------------------------
        void blend_hline(int x, int y,
                         unsigned len, 
                         const color_type& c,
                         int8u cover)
        {
            if (!c.is_transparent())
            {
                value_type* p = pix_value_ptr(x, y, len);
                if (c.is_opaque() && cover == cover_mask)
                {
                    pixel_type v;
                    make_pix(v.c, c);
                    do
                    {
                        *(pixel_type*)p = v;
                        p += pix_step;
                    }
                    while (--len);
                }
                else
                {
                    if (cover == cover_mask)
                    {
                        do
                        {
                            blender_type::blend_pix(p, c.r, c.g, c.b, c.a);
                            p += pix_step;
                        }
                        while (--len);
                    }
                    else
                    {
                        do
                        {
                            blender_type::blend_pix(p, c.r, c.g, c.b, c.a, cover);
                            p += pix_step;
                        }
                        while (--len);
                    }
                }
            }
        }


        //--------------------------------------------------------------------
        void blend_vline(int x, int y,
                         unsigned len, 
                         const color_type& c,
                         int8u cover)
        {
            if (!c.is_transparent())
            {
                value_type* p;
                if (c.is_opaque() && cover == cover_mask)
                {
                    pixel_type v;
                    make_pix(v.c, c);
                    do
                    {
                        p = pix_value_ptr(x, y++, 1);
                        *(pixel_type*)p = v;
                    }
                    while (--len);
                }
                else
                {
                    if (cover == cover_mask)
                    {
                        do
                        {
                            p = pix_value_ptr(x, y++, 1);
                            blender_type::blend_pix(p, c.r, c.g, c.b, c.a);
                        }
                        while (--len);
                    }
                    else
                    {
                        do
                        {
                            p = pix_value_ptr(x, y++, 1);
                            blender_type::blend_pix(p, c.r, c.g, c.b, c.a, cover);
                        }
                        while (--len);
                    }
                }
            }
        }


        //--------------------------------------------------------------------
        void blend_solid_hspan(int x, int y,
                               unsigned len, 
                               const color_type& c,
                               const int8u* covers)
        {
            if (!c.is_transparent())
            {
                value_type* p = pix_value_ptr(x, y, len);
                do 
                {
                    if (c.is_opaque() && *covers == cover_mask)
                    {
                        make_pix(p, c);
                    }
                    else
                    {
                        blender_type::blend_pix(p, c.r, c.g, c.b, c.a, *covers);
                    }
                    p += pix_step;
                    ++covers;
                }
                while (--len);
            }
        }


        //--------------------------------------------------------------------
        void blend_solid_vspan(int x, int y,
                               unsigned len, 
                               const color_type& c,
                               const int8u* covers)
        {
            if (!c.is_transparent())
            {
                do 
                {
                    value_type* p = pix_value_ptr(x, y++, 1);
                    if (c.is_opaque() && *covers == cover_mask)
                    {
                        make_pix(p, c);
                    }
                    else
                    {
                        blender_type::blend_pix(p, c.r, c.g, c.b, c.a, *covers);
                    }
                    ++covers;
                }
                while (--len);
            }
        }

        //--------------------------------------------------------------------
        void copy_color_hspan(int x, int y,
                              unsigned len, 
                              const color_type* colors)
        {
            value_type* p = pix_value_ptr(x, y, len);
            do 
            {
                make_pix(p, *colors);
                ++colors;
                p += pix_step;
            }
            while (--len);
        }


        //--------------------------------------------------------------------
        void copy_color_vspan(int x, int y,
                              unsigned len, 
                              const color_type* colors)
        {
            do 
            {
                value_type* p = pix_value_ptr(x, y++, 1);
                make_pix(p, *colors);
                ++colors;
            }
            while (--len);
        }

        //--------------------------------------------------------------------
        void blend_color_hspan(int x, int y,
                               unsigned len, 
                               const color_type* colors,
                               const int8u* covers,
                               int8u cover)
        {
            value_type* p = pix_value_ptr(x, y, len);
            if (covers)
            {
                do 
                {
                    cob_type::copy_or_blend_pix(p, 
                                                colors->r, 
                                                colors->g, 
                                                colors->b, 
                                                colors->a, 
                                                *covers++);
                    p += pix_step;
                    ++colors;
                }
                while (--len);
            }
            else
            {
                if (cover == cover_mask)
                {
                    do 
                    {
                        cob_type::copy_or_blend_pix(p, 
                                                    colors->r, 
                                                    colors->g, 
                                                    colors->b, 
                                                    colors->a);
                        p += pix_step;
                        ++colors;
                    }
                    while (--len);
                }
                else
                {
                    do 
                    {
                        cob_type::copy_or_blend_pix(p, 
                                                    colors->r, 
                                                    colors->g, 
                                                    colors->b, 
                                                    colors->a, 
                                                    cover);
                        p += pix_step;
                        ++colors;
                    }
                    while (--len);
                }
            }
        }



        //--------------------------------------------------------------------
        void blend_color_vspan(int x, int y,
                               unsigned len, 
                               const color_type* colors,
                               const int8u* covers,
                               int8u cover)
        {
            value_type* p;
            if (covers)
            {
                do 
                {
                    p = pix_value_ptr(x, y++, 1);
                    cob_type::copy_or_blend_pix(p, 
                                                colors->r, 
                                                colors->g, 
                                                colors->b, 
                                                colors->a,
                                                *covers++);
                    ++colors;
                }
                while (--len);
            }
            else
            {
                if (cover == cover_mask)
                {
                    do 
                    {
                        p = pix_value_ptr(x, y++, 1);
                        cob_type::copy_or_blend_pix(p, 
                                                    colors->r, 
                                                    colors->g, 
                                                    colors->b, 
                                                    colors->a);
                        ++colors;
                    }
                    while (--len);
                }
                else
                {
                    do 
                    {
                        p = pix_value_ptr(x, y++, 1);
                        cob_type::copy_or_blend_pix(p, 
                                                    colors->r, 
                                                    colors->g, 
                                                    colors->b, 
                                                    colors->a, 
                                                    cover);
                        ++colors;
                    }
                    while (--len);
                }
            }
        }

        //--------------------------------------------------------------------
        template<class Function> void for_each_pixel(Function f)
        {
            for (unsigned y = 0; y < height(); ++y)
            {
                row_data r = m_rbuf->row(y);
                if (r.ptr)
                {
                    unsigned len = r.x2 - r.x1 + 1;
                    value_type* p = pix_value_ptr(r.x1, y, len);
                    do
                    {
                        f(p);
                        p += pix_step;
                    }
                    while (--len);
                }
            }
        }

        //--------------------------------------------------------------------
        void premultiply()
        {
            for_each_pixel(multiplier_rgba<color_type, order_type>::premultiply);
        }

        //--------------------------------------------------------------------
        void demultiply()
        {
            for_each_pixel(multiplier_rgba<color_type, order_type>::demultiply);
        }

        //--------------------------------------------------------------------
        template<class GammaLut> void apply_gamma_dir(const GammaLut& g)
        {
            for_each_pixel(apply_gamma_dir_rgba<color_type, order_type, GammaLut>(g));
        }

        //--------------------------------------------------------------------
        template<class GammaLut> void apply_gamma_inv(const GammaLut& g)
        {
            for_each_pixel(apply_gamma_inv_rgba<color_type, order_type, GammaLut>(g));
        }

        //--------------------------------------------------------------------
        template<class RenBuf2> void copy_from(const RenBuf2& from, 
                                               int xdst, int ydst,
                                               int xsrc, int ysrc,
                                               unsigned len)
        {
            if (const int8u* p = from.row_ptr(ysrc))
            {
                memmove(m_rbuf->row_ptr(xdst, ydst, len) + xdst * pix_width, 
                        p + xsrc * pix_width, 
                        len * pix_width);
            }
        }

        //--------------------------------------------------------------------
        // Blend from another RGBA surface.
        template<class SrcPixelFormatRenderer>
        void blend_from(const SrcPixelFormatRenderer& from, 
                        int xdst, int ydst,
                        int xsrc, int ysrc,
                        unsigned len,
                        int8u cover)
        {
            typedef typename SrcPixelFormatRenderer::order_type src_order;

            if (const value_type* psrc = from.pix_value_ptr(xsrc, ysrc))
            {
                value_type* pdst = pix_value_ptr(xdst, ydst, len);
                int srcinc = SrcPixelFormatRenderer::pix_step;
                int dstinc = pix_step;

                if (xdst > xsrc)
                {
                    psrc += (len-1) * srcinc;
                    pdst += (len-1) * dstinc;
                    srcinc = -srcinc;
                    dstinc = -dstinc;
                }

                if (cover == cover_mask)
                {
                    do 
                    {
                        cob_type::copy_or_blend_pix(
                            pdst, 
                            psrc[src_order::R],
                            psrc[src_order::G],
                            psrc[src_order::B],
                            psrc[src_order::A]);
                        psrc += srcinc;
                        pdst += dstinc;
                    }
                    while (--len);
                }
                else
                {
                    do 
                    {
                        cob_type::copy_or_blend_pix(
                            pdst, 
                            psrc[src_order::R],
                            psrc[src_order::G],
                            psrc[src_order::B],
                            psrc[src_order::A],
                            cover);
                        psrc += srcinc;
                        pdst += dstinc;
                    }
                    while (--len);
                }
            }
        }

        //--------------------------------------------------------------------
        // Combine single color with grayscale surface and blend.
        template<class SrcPixelFormatRenderer>
        void blend_from_color(const SrcPixelFormatRenderer& from, 
                              const color_type& color,
                              int xdst, int ydst,
                              int xsrc, int ysrc,
                              unsigned len,
                              int8u cover)
        {
            typedef typename SrcPixelFormatRenderer::value_type src_value_type;
            typedef typename SrcPixelFormatRenderer::color_type src_color_type;

            if (const src_value_type* psrc = from.pix_value_ptr(xsrc, ysrc))
            {
                value_type* pdst = pix_value_ptr(xdst, ydst, len);

                do 
                {
                    cob_type::copy_or_blend_pix(
                        pdst, color.r, color.g, color.b, color.a,
                        src_color_type::scale_cover(cover, *psrc));
                    psrc += SrcPixelFormatRenderer::pix_step;
                    pdst += pix_step;
                }
                while (--len);
            }
        }

        //--------------------------------------------------------------------
        // Combine color table with grayscale surface and blend.
        template<class SrcPixelFormatRenderer>
        void blend_from_lut(const SrcPixelFormatRenderer& from, 
                            const color_type* color_lut,
                            int xdst, int ydst,
                            int xsrc, int ysrc,
                            unsigned len,
                            int8u cover)
        {
            typedef typename SrcPixelFormatRenderer::value_type src_value_type;

            if (const src_value_type* psrc = from.pix_value_ptr(xsrc, ysrc))
            {
                value_type* pdst = pix_value_ptr(xdst, ydst, len);

                if (cover == cover_mask)
                {
                    do 
                    {
                        const color_type& color = color_lut[*psrc];
                        cob_type::copy_or_blend_pix(pdst, color.r, color.g, color.b, color.a);
                        psrc += SrcPixelFormatRenderer::pix_step;
                        pdst += pix_step;
                    }
                    while (--len);
                }
                else
                {
                    do 
                    {
                        const color_type& color = color_lut[*psrc];
                        cob_type::copy_or_blend_pix(pdst, color.r, color.g, color.b, color.a, cover);
                        psrc += SrcPixelFormatRenderer::pix_step;
                        pdst += pix_step;
                    }
                    while (--len);
                }
            }
        }

    private:
        rbuf_type* m_rbuf;
    };

    //================================================pixfmt_custom_blend_rgba
    template<class Blender, class RenBuf> class pixfmt_custom_blend_rgba
    {
    public:
        typedef pixfmt_rgba_tag pixfmt_category;
        typedef RenBuf   rbuf_type;
        typedef typename rbuf_type::row_data row_data;
        typedef Blender  blender_type;
        typedef typename blender_type::color_type color_type;
        typedef typename blender_type::order_type order_type;
        typedef typename color_type::value_type value_type;
        typedef typename color_type::calc_type calc_type;
        enum 
        {
            num_components = 4,
            pix_step = 4,
            pix_width  = sizeof(value_type) * pix_step,
        };
        struct pixel_type
        {
            value_type c[num_components];
        };


        //--------------------------------------------------------------------
        pixfmt_custom_blend_rgba() : m_rbuf(0), m_comp_op(3) {}
        explicit pixfmt_custom_blend_rgba(rbuf_type& rb, unsigned comp_op=3) : 
            m_rbuf(&rb),
            m_comp_op(comp_op)
        {}
        void attach(rbuf_type& rb) { m_rbuf = &rb; }

        //--------------------------------------------------------------------
        template<class PixFmt>
        bool attach(PixFmt& pixf, int x1, int y1, int x2, int y2)
        {
            rect_i r(x1, y1, x2, y2);
            if (r.clip(rect_i(0, 0, pixf.width()-1, pixf.height()-1)))
            {
                int stride = pixf.stride();
                m_rbuf->attach(pixf.pix_ptr(r.x1, stride < 0 ? r.y2 : r.y1), 
                               (r.x2 - r.x1) + 1,
                               (r.y2 - r.y1) + 1,
                               stride);
                return true;
            }
            return false;
        }

        //--------------------------------------------------------------------
        AGG_INLINE unsigned width()  const { return m_rbuf->width();  }
        AGG_INLINE unsigned height() const { return m_rbuf->height(); }
        AGG_INLINE int      stride() const { return m_rbuf->stride(); }

        //--------------------------------------------------------------------
        AGG_INLINE       int8u* row_ptr(int y)       { return m_rbuf->row_ptr(y); }
        AGG_INLINE const int8u* row_ptr(int y) const { return m_rbuf->row_ptr(y); }
        AGG_INLINE row_data     row(int y)     const { return m_rbuf->row(y); }

        //--------------------------------------------------------------------
        AGG_INLINE int8u* pix_ptr(int x, int y)
        {
            return m_rbuf->row_ptr(y) + x * pix_width;
        }

        AGG_INLINE const int8u* pix_ptr(int x, int y) const
        {
            return m_rbuf->row_ptr(y) + x * pix_width;
        }

        // Return pointer to pixel value, or null if row not allocated.
        AGG_INLINE const value_type* pix_value_ptr(int x, int y) const 
        {
            return (const value_type*)m_rbuf->row_ptr(x, y) + x * pix_step;
        }

        // Return pointer to pixel value, forcing row to be allocated.
        AGG_INLINE value_type* pix_value_ptr(int x, int y, unsigned len) 
        {
            return (value_type*)m_rbuf->row_ptr(x, y, len) + x * pix_step;
        }

        //--------------------------------------------------------------------
        void comp_op(unsigned op) { m_comp_op = op; }
        unsigned comp_op() const  { return m_comp_op; }

        //--------------------------------------------------------------------
        AGG_INLINE static void set_plain_color(value_type* p, color_type c)
        {
            blender_type::set_plain_color(p, c);
        }

        //--------------------------------------------------------------------
        AGG_INLINE static color_type get_plain_color(const value_type* p)
        {
            return blender_type::get_plain_color(p);
        }

        //--------------------------------------------------------------------
        AGG_INLINE static void make_pix(value_type* p, const color_type& c)
        {
            p[order_type::R] = c.r;
            p[order_type::G] = c.g;
            p[order_type::B] = c.b;
            p[order_type::A] = c.a;
        }

        //--------------------------------------------------------------------
        AGG_INLINE static color_type make_color(const value_type* p)
        {
            return color_type(
                p[order_type::R],
                p[order_type::G],
                p[order_type::B],
                p[order_type::A]);
        }

        //--------------------------------------------------------------------
        AGG_INLINE color_type pixel(int x, int y) const
        {
            if (const value_type* p = pix_value_ptr(x, y))
            {
                return make_color(p);
            }
            return color_type::no_color();
        }

        //--------------------------------------------------------------------
        AGG_INLINE void copy_pixel(int x, int y, const color_type& c)
        {
            make_pix(pix_value_ptr(x, y, 1), c);
        }

        //--------------------------------------------------------------------
        AGG_INLINE void blend_pixel(int x, int y, const color_type& c, int8u cover)
        {
            blender_type::blend_pix(pix_value_ptr(x, y, 1), c.r, c.g, c.b, c.a, cover);
        }

        //--------------------------------------------------------------------
        void copy_hline(int x, int y, unsigned len, const color_type& c)
        {
            value_type* p = pix_value_ptr(x, y, len);
            do
            {
                blender_type::blend_pix(m_comp_op, p, c.r, c.g, c.b, c.a, cover_mask);
                p += pix_step;
            }
            while (--len);
        }

        //--------------------------------------------------------------------
        void copy_vline(int x, int y, unsigned len, const color_type& c)
        {
            do
            {
                blender_type::blend_pix(
                    m_comp_op, 
                    pix_value_ptr(x, y++, 1),
                    c.r, c.g, c.b, c.a, cover_mask);
            }
            while (--len);
        }

        //--------------------------------------------------------------------
        void blend_hline(int x, int y, unsigned len, 
                         const color_type& c, int8u cover)
        {

            value_type* p = pix_value_ptr(x, y, len);
            do
            {
                blender_type::blend_pix(m_comp_op, p, c.r, c.g, c.b, c.a, cover);
                p += pix_step;
            }
            while (--len);
        }

        //--------------------------------------------------------------------
        void blend_vline(int x, int y, unsigned len, 
                         const color_type& c, int8u cover)
        {

            do
            {
                blender_type::blend_pix(
                    m_comp_op, 
                    pix_value_ptr(x, y++, 1), 
                    c.r, c.g, c.b, c.a, 
                    cover);
            }
            while (--len);
        }

        //--------------------------------------------------------------------
        void blend_solid_hspan(int x, int y, unsigned len, 
                               const color_type& c, const int8u* covers)
        {
            value_type* p = pix_value_ptr(x, y, len);
            do 
            {
                blender_type::blend_pix(m_comp_op, 
                                        p, c.r, c.g, c.b, c.a, 
                                        *covers++);
                p += pix_step;
            }
            while (--len);
        }

        //--------------------------------------------------------------------
        void blend_solid_vspan(int x, int y, unsigned len, 
                               const color_type& c, const int8u* covers)
        {
            do 
            {
                blender_type::blend_pix(
                    m_comp_op, 
                    pix_value_ptr(x, y++, 1), 
                    c.r, c.g, c.b, c.a, 
                    *covers++);
            }
            while (--len);
        }

        //--------------------------------------------------------------------
        void copy_color_hspan(int x, int y,
                              unsigned len, 
                              const color_type* colors)
        {

            value_type* p = pix_value_ptr(x, y, len);
            do 
            {
                p[order_type::R] = colors->r;
                p[order_type::G] = colors->g;
                p[order_type::B] = colors->b;
                p[order_type::A] = colors->a;
                ++colors;
                p += pix_step;
            }
            while (--len);
        }

        //--------------------------------------------------------------------
        void copy_color_vspan(int x, int y,
                              unsigned len, 
                              const color_type* colors)
        {
            do 
            {
                value_type* p = pix_value_ptr(x, y++, 1);
                p[order_type::R] = colors->r;
                p[order_type::G] = colors->g;
                p[order_type::B] = colors->b;
                p[order_type::A] = colors->a;
                ++colors;
            }
            while (--len);
        }

        //--------------------------------------------------------------------
        void blend_color_hspan(int x, int y, unsigned len, 
                               const color_type* colors, 
                               const int8u* covers,
                               int8u cover)
        {
            value_type* p = pix_value_ptr(x, y, len);
            do 
            {
                blender_type::blend_pix(m_comp_op, 
                                        p, 
                                        colors->r, 
                                        colors->g, 
                                        colors->b, 
                                        colors->a, 
                                        covers ? *covers++ : cover);
                p += pix_step;
                ++colors;
            }
            while (--len);
        }

        //--------------------------------------------------------------------
        void blend_color_vspan(int x, int y, unsigned len, 
                               const color_type* colors, 
                               const int8u* covers,
                               int8u cover)
        {
            do 
            {
                blender_type::blend_pix(
                    m_comp_op, 
                    pix_value_ptr(x, y++, 1), 
                    colors->r,
                    colors->g,
                    colors->b,
                    colors->a,
                    covers ? *covers++ : cover);
                ++colors;
            }
            while (--len);

        }

        //--------------------------------------------------------------------
        template<class Function> void for_each_pixel(Function f)
        {
            unsigned y;
            for (y = 0; y < height(); ++y)
            {
                row_data r = m_rbuf->row(y);
                if (r.ptr)
                {
                    unsigned len = r.x2 - r.x1 + 1;
                    value_type* p = pix_value_ptr(r.x1, y, len);
                    do
                    {
                        f(p);
                        p += pix_step;
                    }
                    while (--len);
                }
            }
        }

        //--------------------------------------------------------------------
        void premultiply()
        {
            for_each_pixel(multiplier_rgba<color_type, order_type>::premultiply);
        }

        //--------------------------------------------------------------------
        void demultiply()
        {
            for_each_pixel(multiplier_rgba<color_type, order_type>::demultiply);
        }

        //--------------------------------------------------------------------
        template<class GammaLut> void apply_gamma_dir(const GammaLut& g)
        {
            for_each_pixel(apply_gamma_dir_rgba<color_type, order_type, GammaLut>(g));
        }

        //--------------------------------------------------------------------
        template<class GammaLut> void apply_gamma_inv(const GammaLut& g)
        {
            for_each_pixel(apply_gamma_inv_rgba<color_type, order_type, GammaLut>(g));
        }

        //--------------------------------------------------------------------
        template<class RenBuf2> void copy_from(const RenBuf2& from, 
                                               int xdst, int ydst,
                                               int xsrc, int ysrc,
                                               unsigned len)
        {
            if (const int8u* p = from.row_ptr(ysrc))
            {
                memmove(m_rbuf->row_ptr(xdst, ydst, len) + xdst * pix_width, 
                        p + xsrc * pix_width, 
                        len * pix_width);
            }
        }

        //--------------------------------------------------------------------
        // Blend from another RGBA surface.
        template<class SrcPixelFormatRenderer> 
        void blend_from(const SrcPixelFormatRenderer& from, 
                        int xdst, int ydst,
                        int xsrc, int ysrc,
                        unsigned len,
                        int8u cover)
        {
            typedef typename SrcPixelFormatRenderer::value_type src_value_type;
            typedef typename SrcPixelFormatRenderer::order_type src_order;

            if (const src_value_type* psrc = from.pix_value_ptr(xsrc, ysrc))
            {
                value_type* pdst = pix_value_ptr(xdst, ydst, len);

                // The surfaces might be the same, so copy backwards if necessary.
                int srcinc = SrcPixelFormatRenderer::pix_step;
                int dstinc = pix_step;

                if (xdst > xsrc)
                {
                    psrc += (len-1) * srcinc;
                    pdst += (len-1) * dstinc;
                    srcinc = -srcinc;
                    dstinc = -dstinc;
                }

                do 
                {
                    blender_type::blend_pix(
                        m_comp_op, pdst, 
                        psrc[src_order::R],
                        psrc[src_order::G],
                        psrc[src_order::B],
                        psrc[src_order::A],
                        cover);
                    psrc += srcinc;
                    pdst += dstinc;
                }
                while (--len);
            }
        }

        //--------------------------------------------------------------------
        // Blend from single color, using grayscale surface as alpha channel.
        template<class SrcPixelFormatRenderer>
        void blend_from_color(const SrcPixelFormatRenderer& from, 
                              const color_type& color,
                              int xdst, int ydst,
                              int xsrc, int ysrc,
                              unsigned len,
                              int8u cover)
        {
            typedef typename SrcPixelFormatRenderer::value_type src_value_type;
            typedef typename SrcPixelFormatRenderer::color_type src_color_type;

            if (const src_value_type* psrc = from.pix_value_ptr(xsrc, ysrc))
            {
                value_type* pdst = pix_value_ptr(xdst, ydst, len);
                int srcinc = SrcPixelFormatRenderer::pix_step;
                int dstinc = pix_step;

                do 
                {
                    blender_type::blend_pix(
                        m_comp_op, pdst, 
                        color.r, color.g, color.b, color.a,
                        src_color_type::scale_cover(cover, *psrc));
                    psrc += srcinc;
                    pdst += dstinc;
                }
                while (--len);
            }
        }

        //--------------------------------------------------------------------
        // Blend from color table, using grayscale surface as indexes into table.
        // Obviously, this only works for integer value types.
        template<class SrcPixelFormatRenderer>
        void blend_from_lut(const SrcPixelFormatRenderer& from, 
                            const color_type* color_lut,
                            int xdst, int ydst,
                            int xsrc, int ysrc,
                            unsigned len,
                            int8u cover)
        {
            typedef typename SrcPixelFormatRenderer::value_type src_value_type;

            if (const src_value_type* psrc = from.pix_value_ptr(xsrc, ysrc))
            {
                value_type* pdst = pix_value_ptr(xdst, ydst, len);
                int srcinc = SrcPixelFormatRenderer::pix_step;
                int dstinc = pix_step;

                do 
                {
                    const color_type& color = color_lut[*psrc];
                    blender_type::blend_pix(m_comp_op, pdst, 
                        color.r, color.g, color.b, color.a, cover);
                    psrc += srcinc;
                    pdst += dstinc;
                }
                while (--len);
            }
        }

    private:
        rbuf_type* m_rbuf;
        unsigned m_comp_op;
    };


    //-----------------------------------------------------------------------
    typedef blender_rgba<rgba8, order_rgba> blender_rgba32; //----blender_rgba32
    typedef blender_rgba<rgba8, order_argb> blender_argb32; //----blender_argb32
    typedef blender_rgba<rgba8, order_abgr> blender_abgr32; //----blender_abgr32
    typedef blender_rgba<rgba8, order_bgra> blender_bgra32; //----blender_bgra32

    typedef blender_rgba_pre<rgba8, order_rgba> blender_rgba32_pre; //----blender_rgba32_pre
    typedef blender_rgba_pre<rgba8, order_argb> blender_argb32_pre; //----blender_argb32_pre
    typedef blender_rgba_pre<rgba8, order_abgr> blender_abgr32_pre; //----blender_abgr32_pre
    typedef blender_rgba_pre<rgba8, order_bgra> blender_bgra32_pre; //----blender_bgra32_pre

    typedef blender_rgba_plain<rgba8, order_rgba> blender_rgba32_plain; //----blender_rgba32_plain
    typedef blender_rgba_plain<rgba8, order_argb> blender_argb32_plain; //----blender_argb32_plain
    typedef blender_rgba_plain<rgba8, order_abgr> blender_abgr32_plain; //----blender_abgr32_plain
    typedef blender_rgba_plain<rgba8, order_bgra> blender_bgra32_plain; //----blender_bgra32_plain

    typedef blender_rgba<rgba16, order_rgba> blender_rgba64; //----blender_rgba64
    typedef blender_rgba<rgba16, order_argb> blender_argb64; //----blender_argb64
    typedef blender_rgba<rgba16, order_abgr> blender_abgr64; //----blender_abgr64
    typedef blender_rgba<rgba16, order_bgra> blender_bgra64; //----blender_bgra64

    typedef blender_rgba_pre<rgba16, order_rgba> blender_rgba64_pre; //----blender_rgba64_pre
    typedef blender_rgba_pre<rgba16, order_argb> blender_argb64_pre; //----blender_argb64_pre
    typedef blender_rgba_pre<rgba16, order_abgr> blender_abgr64_pre; //----blender_abgr64_pre
    typedef blender_rgba_pre<rgba16, order_bgra> blender_bgra64_pre; //----blender_bgra64_pre

    typedef blender_rgba<rgba32, order_rgba> blender_rgba128;
    typedef blender_rgba<rgba32, order_argb> blender_argb128;
    typedef blender_rgba<rgba32, order_abgr> blender_abgr128;
    typedef blender_rgba<rgba32, order_bgra> blender_bgra128;

    typedef blender_rgba_pre<rgba32, order_rgba> blender_rgba128_pre;
    typedef blender_rgba_pre<rgba32, order_argb> blender_argb128_pre;
    typedef blender_rgba_pre<rgba32, order_abgr> blender_abgr128_pre;
    typedef blender_rgba_pre<rgba32, order_bgra> blender_bgra128_pre;

    typedef blender_rgba_plain<rgba32, order_rgba> blender_rgba128_plain;
    typedef blender_rgba_plain<rgba32, order_argb> blender_argb128_plain;
    typedef blender_rgba_plain<rgba32, order_abgr> blender_abgr128_plain;
    typedef blender_rgba_plain<rgba32, order_bgra> blender_bgra128_plain;


    //-----------------------------------------------------------------------
    typedef pixfmt_alpha_blend_rgba<blender_rgba32, rendering_buffer> pixfmt_rgba32; //----pixfmt_rgba32
    typedef pixfmt_alpha_blend_rgba<blender_argb32, rendering_buffer> pixfmt_argb32; //----pixfmt_argb32
    typedef pixfmt_alpha_blend_rgba<blender_abgr32, rendering_buffer> pixfmt_abgr32; //----pixfmt_abgr32
    typedef pixfmt_alpha_blend_rgba<blender_bgra32, rendering_buffer> pixfmt_bgra32; //----pixfmt_bgra32

    typedef pixfmt_alpha_blend_rgba<blender_rgba32_pre, rendering_buffer> pixfmt_rgba32_pre; //----pixfmt_rgba32_pre
    typedef pixfmt_alpha_blend_rgba<blender_argb32_pre, rendering_buffer> pixfmt_argb32_pre; //----pixfmt_argb32_pre
    typedef pixfmt_alpha_blend_rgba<blender_abgr32_pre, rendering_buffer> pixfmt_abgr32_pre; //----pixfmt_abgr32_pre
    typedef pixfmt_alpha_blend_rgba<blender_bgra32_pre, rendering_buffer> pixfmt_bgra32_pre; //----pixfmt_bgra32_pre

    typedef pixfmt_alpha_blend_rgba<blender_rgba32_plain, rendering_buffer> pixfmt_rgba32_plain; //----pixfmt_rgba32_plain
    typedef pixfmt_alpha_blend_rgba<blender_argb32_plain, rendering_buffer> pixfmt_argb32_plain; //----pixfmt_argb32_plain
    typedef pixfmt_alpha_blend_rgba<blender_abgr32_plain, rendering_buffer> pixfmt_abgr32_plain; //----pixfmt_abgr32_plain
    typedef pixfmt_alpha_blend_rgba<blender_bgra32_plain, rendering_buffer> pixfmt_bgra32_plain; //----pixfmt_bgra32_plain

    typedef pixfmt_alpha_blend_rgba<blender_rgba64, rendering_buffer> pixfmt_rgba64; //----pixfmt_rgba64
    typedef pixfmt_alpha_blend_rgba<blender_argb64, rendering_buffer> pixfmt_argb64; //----pixfmt_argb64
    typedef pixfmt_alpha_blend_rgba<blender_abgr64, rendering_buffer> pixfmt_abgr64; //----pixfmt_abgr64
    typedef pixfmt_alpha_blend_rgba<blender_bgra64, rendering_buffer> pixfmt_bgra64; //----pixfmt_bgra64

    typedef pixfmt_alpha_blend_rgba<blender_rgba64_pre, rendering_buffer> pixfmt_rgba64_pre; //----pixfmt_rgba64_pre
    typedef pixfmt_alpha_blend_rgba<blender_argb64_pre, rendering_buffer> pixfmt_argb64_pre; //----pixfmt_argb64_pre
    typedef pixfmt_alpha_blend_rgba<blender_abgr64_pre, rendering_buffer> pixfmt_abgr64_pre; //----pixfmt_abgr64_pre
    typedef pixfmt_alpha_blend_rgba<blender_bgra64_pre, rendering_buffer> pixfmt_bgra64_pre; //----pixfmt_bgra64_pre

    typedef pixfmt_alpha_blend_rgba<blender_rgba128, rendering_buffer> pixfmt_rgba128;
    typedef pixfmt_alpha_blend_rgba<blender_argb128, rendering_buffer> pixfmt_argb128;
    typedef pixfmt_alpha_blend_rgba<blender_abgr128, rendering_buffer> pixfmt_abgr128;
    typedef pixfmt_alpha_blend_rgba<blender_bgra128, rendering_buffer> pixfmt_bgra128;

    typedef pixfmt_alpha_blend_rgba<blender_rgba128_pre, rendering_buffer> pixfmt_rgba128_pre;
    typedef pixfmt_alpha_blend_rgba<blender_argb128_pre, rendering_buffer> pixfmt_argb128_pre;
    typedef pixfmt_alpha_blend_rgba<blender_abgr128_pre, rendering_buffer> pixfmt_abgr128_pre;
    typedef pixfmt_alpha_blend_rgba<blender_bgra128_pre, rendering_buffer> pixfmt_bgra128_pre;

    typedef pixfmt_alpha_blend_rgba<blender_rgba128_plain, rendering_buffer> pixfmt_rgba128_plain;
    typedef pixfmt_alpha_blend_rgba<blender_argb128_plain, rendering_buffer> pixfmt_argb128_plain;
    typedef pixfmt_alpha_blend_rgba<blender_abgr128_plain, rendering_buffer> pixfmt_abgr128_plain;
    typedef pixfmt_alpha_blend_rgba<blender_bgra128_plain, rendering_buffer> pixfmt_bgra128_plain;

}

#endif

