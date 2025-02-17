/*****************************************************************************
 * sdl_image.c: sdl decoder module making use of libsdl_image.
 *****************************************************************************
 * Copyright (C) 2005 the VideoLAN team
 * $Id: sdl_image.c 17236 2006-10-21 19:11:38Z hartman $
 *
 * Authors: Christophe Massiot <massiot@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#include <vlc/vlc.h>
#include <vlc/decoder.h>

#include SDL_IMAGE_INCLUDE_FILE

/*****************************************************************************
 * decoder_sys_t : sdl decoder descriptor
 *****************************************************************************/
struct decoder_sys_t
{
    const char *psz_sdl_type;
};

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int  OpenDecoder   ( vlc_object_t * );
static void CloseDecoder  ( vlc_object_t * );

static picture_t *DecodeBlock  ( decoder_t *, block_t ** );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
vlc_module_begin();
    set_category( CAT_INPUT );
    set_subcategory( SUBCAT_INPUT_VCODEC );
    set_description( _("SDL_image video decoder") );
    set_capability( "decoder", 60 );
    set_callbacks( OpenDecoder, CloseDecoder );
    add_shortcut( "sdl_image" );
vlc_module_end();

static const struct supported_fmt_t
{
    vlc_fourcc_t i_fourcc;
    char *psz_sdl_type;
} p_supported_fmt[] =
{
    { VLC_FOURCC('t','g','a',' '), "TGA" },
    { VLC_FOURCC('b','m','p',' '), "BMP" },
    { VLC_FOURCC('p','n','m',' '), "PNM" },
    { VLC_FOURCC('x','p','m',' '), "XPM" },
    { VLC_FOURCC('x','c','f',' '), "XCF" },
    { VLC_FOURCC('p','c','x',' '), "PCX" },
    { VLC_FOURCC('g','i','f',' '), "GIF" },
    { VLC_FOURCC('j','p','e','g'), "JPG" },
    { VLC_FOURCC('t','i','f','f'), "TIF" },
    { VLC_FOURCC('l','b','m',' '), "LBM" },
    { VLC_FOURCC('p','n','g',' '), "PNG" }
};

/*****************************************************************************
 * OpenDecoder: probe the decoder and return score
 *****************************************************************************/
static int OpenDecoder( vlc_object_t *p_this )
{
    decoder_t *p_dec = (decoder_t *)p_this;
    decoder_sys_t *p_sys;
    int i;

    /* Find codec. */
    for ( i = 0;
          i < (int)(sizeof(p_supported_fmt)/sizeof(struct supported_fmt_t));
          i++ )
    {
        if ( p_supported_fmt[i].i_fourcc == p_dec->fmt_in.i_codec )
            break;
    }
    if ( i == (int)(sizeof(p_supported_fmt)/sizeof(struct supported_fmt_t)) )
    {
        return VLC_EGENERIC;
    }

    /* Allocate the memory needed to store the decoder's structure */
    if( ( p_dec->p_sys = p_sys =
          (decoder_sys_t *)malloc(sizeof(decoder_sys_t)) ) == NULL )
    {
        msg_Err( p_dec, "out of memory" );
        return VLC_EGENERIC;
    }
    p_sys->psz_sdl_type = p_supported_fmt[i].psz_sdl_type;

    /* Set output properties - this is a decoy and isn't used anywhere */
    p_dec->fmt_out.i_cat = VIDEO_ES;
    p_dec->fmt_out.i_codec = VLC_FOURCC('R','V','3','2');

    /* Set callbacks */
    p_dec->pf_decode_video = DecodeBlock;

    return VLC_SUCCESS;
}

/****************************************************************************
 * DecodeBlock: the whole thing
 ****************************************************************************
 * This function must be fed with a complete compressed frame.
 ****************************************************************************/
static picture_t *DecodeBlock( decoder_t *p_dec, block_t **pp_block )
{
    decoder_sys_t *p_sys = p_dec->p_sys;
    block_t *p_block;
    picture_t *p_pic = NULL;
    SDL_Surface *p_surface;
    SDL_RWops *p_rw;

    if( pp_block == NULL || *pp_block == NULL ) return NULL;
    p_block = *pp_block;

    p_rw = SDL_RWFromConstMem( p_block->p_buffer, p_block->i_buffer );

    /* Decode picture. */
    p_surface = IMG_LoadTyped_RW( p_rw, 1, (char*)p_sys->psz_sdl_type );
    if ( p_surface == NULL )
    {
        msg_Warn( p_dec, "SDL_image couldn't load the image (%s)",
                  IMG_GetError() );
        goto error;
    }

    switch ( p_surface->format->BitsPerPixel )
    {
    case 16:
        p_dec->fmt_out.i_codec = VLC_FOURCC('R','V','1','6');
        break;
    case 8:
    case 24:
        p_dec->fmt_out.i_codec = VLC_FOURCC('R','V','2','4');
        break;
    case 32:
        p_dec->fmt_out.i_codec = VLC_FOURCC('R','V','3','2');
        break;
    default:
        msg_Warn( p_dec, "unknown bits/pixel format (%d)",
                  p_surface->format->BitsPerPixel );
        goto error;
    }
    p_dec->fmt_out.video.i_width = p_surface->w;
    p_dec->fmt_out.video.i_height = p_surface->h;
    p_dec->fmt_out.video.i_aspect = VOUT_ASPECT_FACTOR * p_surface->w
                                     / p_surface->h;

    /* Get a new picture. */
    p_pic = p_dec->pf_vout_buffer_new( p_dec );
    if ( p_pic == NULL ) goto error;

    switch ( p_surface->format->BitsPerPixel )
    {
        case 8:
        {
            int i, j;
            uint8_t *p_src, *p_dst;
            uint8_t r, g, b;
            for ( i = 0; i < p_surface->h; i++ )
            {
                p_src = p_surface->pixels + i * p_surface->pitch;
                p_dst = p_pic->p[0].p_pixels + i * p_pic->p[0].i_pitch;
                for ( j = 0; j < p_surface->w; j++ )
                {
                    SDL_GetRGB( *(p_src++), p_surface->format,
                                &r, &g, &b );
                    *(p_dst++) = r;
                    *(p_dst++) = g;
                    *(p_dst++) = b;
                }
            }
            break;
        }
        case 16:
        {
            int i;
            uint8_t *p_src = p_surface->pixels;
            uint8_t *p_dst = p_pic->p[0].p_pixels;
            int i_pitch = p_pic->p[0].i_pitch < p_surface->pitch ?
                p_pic->p[0].i_pitch : p_surface->pitch;

            for ( i = 0; i < p_surface->h; i++ )
            {
                p_dec->p_vlc->pf_memcpy( p_dst, p_src, i_pitch );
                p_src += p_surface->pitch;
                p_dst += p_pic->p[0].i_pitch;
            }
            break;
        }
        case 24:
        {
            int i, j;
            uint8_t *p_src, *p_dst;
            uint8_t r, g, b;
            for ( i = 0; i < p_surface->h; i++ )
            {
                p_src = p_surface->pixels + i * p_surface->pitch;
                p_dst = p_pic->p[0].p_pixels + i * p_pic->p[0].i_pitch;
                for ( j = 0; j < p_surface->w; j++ )
                {
                    SDL_GetRGB( *(uint32_t*)p_src, p_surface->format,
                                &r, &g, &b );
                    *(p_dst++) = r;
                    *(p_dst++) = g;
                    *(p_dst++) = b;
                    p_src += 3;
                }
            }
            break;
        }
        case 32:
        {
            int i, j;
            uint8_t *p_src, *p_dst;
            uint8_t r, g, b, a;
            for ( i = 0; i < p_surface->h; i++ )
            {
                p_src = p_surface->pixels + i * p_surface->pitch;
                p_dst = p_pic->p[0].p_pixels + i * p_pic->p[0].i_pitch;
                for ( j = 0; j < p_surface->w; j++ )
                {
                    SDL_GetRGBA( *(uint32_t*)p_src, p_surface->format,
                                &r, &g, &b, &a );
                    *(p_dst++) = b;
                    *(p_dst++) = g;
                    *(p_dst++) = r;
                    *(p_dst++) = a;
                    p_src += 4;
                }
            }
            break;
        }
    }

    SDL_FreeSurface( p_surface );
    block_Release( p_block ); *pp_block = NULL;
    return p_pic;

error:
    if ( p_surface != NULL ) SDL_FreeSurface( p_surface );
    block_Release( p_block ); *pp_block = NULL;
    return NULL;
}

/*****************************************************************************
 * CloseDecoder: sdl decoder destruction
 *****************************************************************************/
static void CloseDecoder( vlc_object_t *p_this )
{
    decoder_t *p_dec = (decoder_t *)p_this;
    decoder_sys_t *p_sys = p_dec->p_sys;

    free( p_sys );
}
