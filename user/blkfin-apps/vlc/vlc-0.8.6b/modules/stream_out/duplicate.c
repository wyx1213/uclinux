/*****************************************************************************
 * duplicate.c: duplicate stream output module
 *****************************************************************************
 * Copyright (C) 2003-2004 the VideoLAN team
 * $Id: duplicate.c 13905 2006-01-12 23:10:04Z dionoea $
 *
 * Author: Laurent Aimar <fenrir@via.ecp.fr>
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
#include <stdlib.h>
#include <string.h>

#include <vlc/vlc.h>
#include <vlc/sout.h>

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int      Open    ( vlc_object_t * );
static void     Close   ( vlc_object_t * );

vlc_module_begin();
    set_description( _("Duplicate stream output") );
    set_capability( "sout stream", 50 );
    add_shortcut( "duplicate" );
    add_shortcut( "dup" );
    set_category( CAT_SOUT );
    set_subcategory( SUBCAT_SOUT_STREAM );
    set_callbacks( Open, Close );
vlc_module_end();


/*****************************************************************************
 * Exported prototypes
 *****************************************************************************/
static sout_stream_id_t *Add ( sout_stream_t *, es_format_t * );
static int               Del ( sout_stream_t *, sout_stream_id_t * );
static int               Send( sout_stream_t *, sout_stream_id_t *,
                               block_t* );

struct sout_stream_sys_t
{
    int             i_nb_streams;
    sout_stream_t   **pp_streams;

    int             i_nb_select;
    char            **ppsz_select;
};

struct sout_stream_id_t
{
    int                 i_nb_ids;
    void                **pp_ids;
};

static vlc_bool_t ESSelected( es_format_t *fmt, char *psz_select );

/*****************************************************************************
 * Open:
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    sout_stream_t     *p_stream = (sout_stream_t*)p_this;
    sout_stream_sys_t *p_sys;
    sout_cfg_t        *p_cfg;

    msg_Dbg( p_stream, "creating 'duplicate'" );

    p_sys = malloc( sizeof( sout_stream_sys_t ) );

    p_sys->i_nb_streams = 0;
    p_sys->pp_streams   = NULL;
    p_sys->i_nb_select  = 0;
    p_sys->ppsz_select  = NULL;

    for( p_cfg = p_stream->p_cfg; p_cfg != NULL; p_cfg = p_cfg->p_next )
    {
        if( !strncmp( p_cfg->psz_name, "dst", strlen( "dst" ) ) )
        {
            sout_stream_t *s;

            msg_Dbg( p_stream, " * adding `%s'", p_cfg->psz_value );
            s = sout_StreamNew( p_stream->p_sout, p_cfg->psz_value );

            if( s )
            {
                TAB_APPEND( p_sys->i_nb_streams, p_sys->pp_streams, s );
                TAB_APPEND( p_sys->i_nb_select,  p_sys->ppsz_select, NULL );
            }
        }
        else if( !strncmp( p_cfg->psz_name, "select", strlen( "select" ) ) )
        {
            char *psz = p_cfg->psz_value;
            if( p_sys->i_nb_select > 0 && psz && *psz )
            {
                msg_Dbg( p_stream, " * apply selection %s", psz );
                p_sys->ppsz_select[p_sys->i_nb_select - 1] = strdup( psz );
            }
        }
    }

    if( p_sys->i_nb_streams == 0 )
    {
        msg_Err( p_stream, "no destination given" );
        free( p_sys );

        return VLC_EGENERIC;
    }

    p_stream->pf_add    = Add;
    p_stream->pf_del    = Del;
    p_stream->pf_send   = Send;

    p_stream->p_sys     = p_sys;

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close:
 *****************************************************************************/
static void Close( vlc_object_t * p_this )
{
    sout_stream_t     *p_stream = (sout_stream_t*)p_this;
    sout_stream_sys_t *p_sys = p_stream->p_sys;

    int i;

    msg_Dbg( p_stream, "closing a duplication" );
    for( i = 0; i < p_sys->i_nb_streams; i++ )
    {
        sout_StreamDelete( p_sys->pp_streams[i] );
        if( p_sys->ppsz_select[i] )
        {
            free( p_sys->ppsz_select[i] );
        }
    }
    if( p_sys->pp_streams )
    {
        free( p_sys->pp_streams );
    }
    if( p_sys->ppsz_select )
    {
        free( p_sys->ppsz_select );
    }

    free( p_sys );
}

/*****************************************************************************
 * Add:
 *****************************************************************************/
static sout_stream_id_t * Add( sout_stream_t *p_stream, es_format_t *p_fmt )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;
    sout_stream_id_t  *id;
    int i_stream, i_valid_streams = 0;

    id = malloc( sizeof( sout_stream_id_t ) );
    id->i_nb_ids = 0;
    id->pp_ids   = NULL;

    msg_Dbg( p_stream, "duplicated a new stream codec=%4.4s (es=%d group=%d)",
             (char*)&p_fmt->i_codec, p_fmt->i_id, p_fmt->i_group );

    for( i_stream = 0; i_stream < p_sys->i_nb_streams; i_stream++ )
    {
        void *id_new = NULL;

        if( ESSelected( p_fmt, p_sys->ppsz_select[i_stream] ) )
        {
            sout_stream_t *out = p_sys->pp_streams[i_stream];

            id_new = (void*)out->pf_add( out, p_fmt );
            if( id_new )
            {
                msg_Dbg( p_stream, "    - added for output %d", i_stream );
                i_valid_streams++;
            }
            else
            {
                msg_Dbg( p_stream, "    - failed for output %d", i_stream );
            }
        }
        else
        {
            msg_Dbg( p_stream, "    - ignored for output %d", i_stream );
        }

        /* Append failed attempts as well to keep track of which pp_id
         * belongs to which duplicated stream */
        TAB_APPEND( id->i_nb_ids, id->pp_ids, id_new );
    }

    if( i_valid_streams <= 0 )
    {
        Del( p_stream, id );
        return NULL;
    }

    return id;
}

/*****************************************************************************
 * Del:
 *****************************************************************************/
static int Del( sout_stream_t *p_stream, sout_stream_id_t *id )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;
    int               i_stream;

    for( i_stream = 0; i_stream < p_sys->i_nb_streams; i_stream++ )
    {
        if( id->pp_ids[i_stream] )
        {
            sout_stream_t *out = p_sys->pp_streams[i_stream];
            out->pf_del( out, id->pp_ids[i_stream] );
        }
    }

    free( id->pp_ids );
    free( id );
    return VLC_SUCCESS;
}

/*****************************************************************************
 * Send:
 *****************************************************************************/
static int Send( sout_stream_t *p_stream, sout_stream_id_t *id,
                 block_t *p_buffer )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;
    sout_stream_t     *p_dup_stream;
    int               i_stream;

    /* Loop through the linked list of buffers */
    while( p_buffer )
    {
        block_t *p_next = p_buffer->p_next;

        p_buffer->p_next = NULL;

        for( i_stream = 0; i_stream < p_sys->i_nb_streams - 1; i_stream++ )
        {
            block_t *p_dup;
            p_dup_stream = p_sys->pp_streams[i_stream];

            if( id->pp_ids[i_stream] )
            {
                p_dup = block_Duplicate( p_buffer );

                p_dup_stream->pf_send( p_dup_stream, id->pp_ids[i_stream],
                                       p_dup );
            }
        }

        if( i_stream < p_sys->i_nb_streams && id->pp_ids[i_stream] )
        {
            p_dup_stream = p_sys->pp_streams[i_stream];
            p_dup_stream->pf_send( p_dup_stream, id->pp_ids[i_stream],
                                   p_buffer );
        }
        else
        {
            block_Release( p_buffer );
        }

        p_buffer = p_next;
    }
    return VLC_SUCCESS;
}

/*****************************************************************************
 * Divers
 *****************************************************************************/
static vlc_bool_t NumInRange( char *psz_range, int i_num )
{
    char *psz = strchr( psz_range, '-' );
    char *end;
    int  i_start, i_stop;

    if( psz )
    {
        i_start = strtol( psz_range, &end, 0 );
        if( end == psz_range ) i_start = i_num;

        i_stop  = strtol( psz,       &end, 0 );
        if( end == psz_range ) i_stop = i_num;
    }
    else
    {
        i_start = i_stop = strtol( psz_range, NULL, 0 );
    }

    return i_start <= i_num && i_num <= i_stop ? VLC_TRUE : VLC_FALSE;
}

static vlc_bool_t ESSelected( es_format_t *fmt, char *psz_select )
{
    char  *psz_dup;
    char  *psz;

    /* We have tree state variable : no tested (-1), failed(0), succeed(1) */
    int i_cat = -1;
    int i_es  = -1;
    int i_prgm= -1;

    /* If empty all es are selected */
    if( psz_select == NULL || *psz_select == '\0' )
    {
        return VLC_TRUE;
    }
    psz_dup = strdup( psz_select );
    psz     = psz_dup;

    /* If non empty, parse the selection:
     * We have selection[,selection[,..]] where following selection are recognized:
     *      (no(-))audio
     *      (no(-))spu
     *      (no(-))video
     *      (no(-))es=[start]-[end] or es=num
     *      (no(-))prgm=[start]-[end] or prgm=num (program works too)
     *      if a negative test failed we exit directly
     */
    while( psz && *psz )
    {
        char *p;

        /* Skip space */
        while( *psz == ' ' || *psz == '\t' ) psz++;

        /* Search end */
        p = strchr( psz, ',' );
        if( p == psz )
        {
            /* Empty */
            psz = p + 1;
            continue;
        }
        if( p )
        {
            *p++ = '\0';
        }

        if( !strncmp( psz, "no-audio", strlen( "no-audio" ) ) ||
            !strncmp( psz, "noaudio", strlen( "noaudio" ) ) )
        {
            if( i_cat == -1 )
            {
                i_cat = fmt->i_cat != AUDIO_ES ? 1 : 0;
            }
        }
        else if( !strncmp( psz, "no-video", strlen( "no-video" ) ) ||
                 !strncmp( psz, "novideo", strlen( "novideo" ) ) )
        {
            if( i_cat == -1 )
            {
                i_cat = fmt->i_cat != VIDEO_ES ? 1 : 0;
            }
        }
        else if( !strncmp( psz, "no-spu", strlen( "no-spu" ) ) ||
                 !strncmp( psz, "nospu", strlen( "nospu" ) ) )
        {
            if( i_cat == -1 )
            {
                i_cat = fmt->i_cat != SPU_ES ? 1 : 0;
            }
        }
        else if( !strncmp( psz, "audio", strlen( "audio" ) ) )
        {
            if( i_cat == -1 )
            {
                i_cat = fmt->i_cat == AUDIO_ES ? 1 : 0;
            }
        }
        else if( !strncmp( psz, "video", strlen( "video" ) ) )
        {
            if( i_cat == -1 )
            {
                i_cat = fmt->i_cat == VIDEO_ES ? 1 : 0;
            }
        }
        else if( !strncmp( psz, "spu", strlen( "spu" ) ) )
        {
            if( i_cat == -1 )
            {
                i_cat = fmt->i_cat == SPU_ES ? 1 : 0;
            }
        }
        else if( strchr( psz, '=' ) != NULL )
        {
            char *psz_arg = strchr( psz, '=' );
            *psz_arg++ = '\0';

            if( !strcmp( psz, "no-es" ) || !strcmp( psz, "noes" ) )
            {
                if( i_es == -1 )
                {
                    i_es = NumInRange( psz_arg, fmt->i_id ) ? 0 : -1;
                }
            }
            else if( !strcmp( psz, "es" ) )
            {
                if( i_es == -1 )
                {
                    i_es = NumInRange( psz_arg, fmt->i_id) ? 1 : -1;
                }
            }
            else if( !strcmp( psz, "no-prgm" ) || !strcmp( psz, "noprgm" ) ||
                      !strcmp( psz, "no-program" ) || !strcmp( psz, "noprogram" ) )
            {
                if( fmt->i_group >= 0 && i_prgm == -1 )
                {
                    i_prgm = NumInRange( psz_arg, fmt->i_group ) ? 0 : -1;
                }
            }
            else if( !strcmp( psz, "prgm" ) || !strcmp( psz, "program" ) )
            {
                if( fmt->i_group >= 0 && i_prgm == -1 )
                {
                    i_prgm = NumInRange( psz_arg, fmt->i_group ) ? 1 : -1;
                }
            }
        }
        else
        {
            fprintf( stderr, "unknown args (%s)\n", psz );
        }
        /* Next */
        psz = p;
    }

    free( psz_dup );

    if( i_cat == 1 || i_es == 1 || i_prgm == 1 )
    {
        return VLC_TRUE;
    }
    return VLC_FALSE;
}
