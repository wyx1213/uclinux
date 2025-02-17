/*
    Copyright (C) 2006 Michael Niedermayer <michaelni@gmx.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#include "config.h"

#include "mp_msg.h"
#include "cpudetect.h"

#include "img_format.h"
#include "mp_image.h"
#include "vf.h"

#define HAVE_AV_CONFIG_H
#include "libavcodec/avcodec.h"
#include "libavcodec/eval.h"

struct vf_priv_s {
    AVEvalExpr * e[3];
    int framenum;
    mp_image_t *mpi;
};

static int config(struct vf_instance_s* vf,
        int width, int height, int d_width, int d_height,
        unsigned int flags, unsigned int outfmt){
    return vf_next_config(vf,width,height,d_width,d_height,flags,outfmt);
}

static inline double getpix(struct vf_instance_s* vf, double x, double y, int plane){
    int xi, yi;
    mp_image_t *mpi= vf->priv->mpi;
    int stride= mpi->stride[plane];
    uint8_t *src=  mpi->planes[plane];
    xi=x= FFMIN(FFMAX(x, 0), (mpi->w >> (plane ? mpi->chroma_x_shift : 0))-1);
    yi=y= FFMIN(FFMAX(y, 0), (mpi->h >> (plane ? mpi->chroma_y_shift : 0))-1);

    x-=xi;
    y-=yi;

    return
     (1-y)*((1-x)*src[xi +  yi    * stride] + x*src[xi + 1 +  yi    * stride])
    +   y *((1-x)*src[xi + (yi+1) * stride] + x*src[xi + 1 + (yi+1) * stride]);
}

//FIXME cubic interpolate
//FIXME keep the last few frames
static double lum(struct vf_instance_s* vf, double x, double y){
    return getpix(vf, x, y, 0);
}

static double cb(struct vf_instance_s* vf, double x, double y){
    return getpix(vf, x, y, 1);
}

static double cr(struct vf_instance_s* vf, double x, double y){
    return getpix(vf, x, y, 2);
}

static int put_image(struct vf_instance_s* vf, mp_image_t *mpi, double pts){
    mp_image_t *dmpi;
    int x,y, plane;

    if(!(mpi->flags&MP_IMGFLAG_DIRECT)){
        // no DR, so get a new image! hope we'll get DR buffer:
        vf->dmpi=vf_get_image(vf->next,mpi->imgfmt, MP_IMGTYPE_TEMP,
                              MP_IMGFLAG_ACCEPT_STRIDE|MP_IMGFLAG_PREFER_ALIGNED_STRIDE,
                              mpi->w,mpi->h);
    }

    dmpi= vf->dmpi;
    vf->priv->mpi= mpi;

    vf_clone_mpi_attributes(dmpi, mpi);

    for(plane=0; plane<3; plane++){
        int w= mpi->w >> (plane ? mpi->chroma_x_shift : 0);
        int h= mpi->h >> (plane ? mpi->chroma_y_shift : 0);
        uint8_t *dst  = dmpi->planes[plane];
        int dst_stride= dmpi->stride[plane];
        double const_values[]={
            M_PI,
            M_E,
            0,
            0,
            w,
            h,
            vf->priv->framenum,
            w/(double)mpi->w,
            h/(double)mpi->h,
            0
        };
        if (!vf->priv->e[plane]) continue;
        for(y=0; y<h; y++){
            const_values[3]=y;
            for(x=0; x<w; x++){
                const_values[2]=x;
                dst[x+y* dst_stride]= ff_parse_eval(vf->priv->e[plane], const_values, vf);
            }
        }
    }

    vf->priv->framenum++;

    return vf_next_put_image(vf,dmpi, pts);
}

static void uninit(struct vf_instance_s* vf){
    if(!vf->priv) return;

    av_free(vf->priv);
    vf->priv=NULL;
}

//===========================================================================//
static int open(vf_instance_t *vf, char* args){
    char eq[3][2000] = { { 0 }, { 0 }, { 0 } };
    int plane;

    vf->config=config;
    vf->put_image=put_image;
//    vf->get_image=get_image;
    vf->uninit=uninit;
    vf->priv=av_malloc(sizeof(struct vf_priv_s));
    memset(vf->priv, 0, sizeof(struct vf_priv_s));

    if (args) sscanf(args, "%1999[^:]:%1999[^:]:%1999[^:]", eq[0], eq[1], eq[2]);

    if (!eq[1][0]) strncpy(eq[1], eq[0], sizeof(eq[0])-1);
    if (!eq[2][0]) strncpy(eq[2], eq[1], sizeof(eq[0])-1);

    for(plane=0; plane<3; plane++){
        static const char *const_names[]={
            "PI",
            "E",
            "X",
            "Y",
            "W",
            "H",
            "N",
            "SW",
            "SH",
            NULL
        };
        static const char *func2_names[]={
            "lum",
            "cb",
            "cr",
            "p",
            NULL
        };
        double (*func2[])(void *, double, double)={
            lum,
            cb,
            cr,
            plane==0 ? lum : (plane==1 ? cb : cr),
            NULL
        };
        char * a;
        vf->priv->e[plane] = ff_parse(eq[plane], const_names, NULL, NULL, func2, func2_names, &a);

        if (!vf->priv->e[plane]) {
            mp_msg(MSGT_VFILTER, MSGL_ERR, "geq: error loading equation `%s': %s\n", eq[plane], a);
        }
    }

    return 1;
}

vf_info_t vf_info_geq = {
    "generic equation filter",
    "geq",
    "Michael Niedermayer",
    "",
    open,
    NULL
};
