/*
 * AVI demuxer
 * Copyright (c) 2001 Fabrice Bellard.
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "avformat.h"
#include "avi.h"
#include "dv.h"
#include "riff.h"

#undef NDEBUG
#include <assert.h>

//#define DEBUG
//#define DEBUG_SEEK

typedef struct AVIStream {
    int64_t frame_offset; /* current frame (video) or byte (audio) counter
                         (used to compute the pts) */
    int remaining;
    int packet_size;

    int scale;
    int rate;
    int sample_size; /* size of one sample (or packet) (in the rate/scale sense) in bytes */

    int64_t cum_len; /* temporary storage (used during seek) */

    int prefix;                       ///< normally 'd'<<8 + 'c' or 'w'<<8 + 'b'
    int prefix_count;
} AVIStream;

typedef struct {
    int64_t  riff_end;
    int64_t  movi_end;
    int64_t  fsize;
    offset_t movi_list;
    int index_loaded;
    int is_odml;
    int non_interleaved;
    int stream_index;
    DVDemuxContext* dv_demux;
} AVIContext;

static const char avi_headers[][8] = {
    { 'R', 'I', 'F', 'F',    'A', 'V', 'I', ' ' },
    { 'R', 'I', 'F', 'F',    'A', 'V', 'I', 'X' },
    { 'R', 'I', 'F', 'F',    'A', 'V', 'I', 0x19},
    { 'O', 'N', '2', ' ',    'O', 'N', '2', 'f' },
    { 'R', 'I', 'F', 'F',    'A', 'M', 'V', ' ' },
    { 0 }
};

static int avi_load_index(AVFormatContext *s);
static int guess_ni_flag(AVFormatContext *s);

#ifdef DEBUG
static void print_tag(const char *str, unsigned int tag, int size)
{
    printf("%s: tag=%c%c%c%c size=0x%x\n",
           str, tag & 0xff,
           (tag >> 8) & 0xff,
           (tag >> 16) & 0xff,
           (tag >> 24) & 0xff,
           size);
}
#endif

static int get_riff(AVIContext *avi, ByteIOContext *pb)
{
    char header[8];
    int i;

    /* check RIFF header */
    get_buffer(pb, header, 4);
    avi->riff_end = get_le32(pb);   /* RIFF chunk size */
    avi->riff_end += url_ftell(pb); /* RIFF chunk end */
    get_buffer(pb, header+4, 4);

    for(i=0; avi_headers[i][0]; i++)
        if(!memcmp(header, avi_headers[i], 8))
            break;
    if(!avi_headers[i][0])
        return -1;

    if(header[7] == 0x19)
        av_log(NULL, AV_LOG_INFO, "file has been generated with a totally broken muxer\n");

    return 0;
}

static int read_braindead_odml_indx(AVFormatContext *s, int frame_num){
    AVIContext *avi = s->priv_data;
    ByteIOContext *pb = s->pb;
    int longs_pre_entry= get_le16(pb);
    int index_sub_type = get_byte(pb);
    int index_type     = get_byte(pb);
    int entries_in_use = get_le32(pb);
    int chunk_id       = get_le32(pb);
    int64_t base       = get_le64(pb);
    int stream_id= 10*((chunk_id&0xFF) - '0') + (((chunk_id>>8)&0xFF) - '0');
    AVStream *st;
    AVIStream *ast;
    int i;
    int64_t last_pos= -1;
    int64_t filesize= url_fsize(s->pb);

#ifdef DEBUG_SEEK
    av_log(s, AV_LOG_ERROR, "longs_pre_entry:%d index_type:%d entries_in_use:%d chunk_id:%X base:%16"PRIX64"\n",
        longs_pre_entry,index_type, entries_in_use, chunk_id, base);
#endif

    if(stream_id > s->nb_streams || stream_id < 0)
        return -1;
    st= s->streams[stream_id];
    ast = st->priv_data;

    if(index_sub_type)
        return -1;

    get_le32(pb);

    if(index_type && longs_pre_entry != 2)
        return -1;
    if(index_type>1)
        return -1;

    if(filesize > 0 && base >= filesize){
        av_log(s, AV_LOG_ERROR, "ODML index invalid\n");
        if(base>>32 == (base & 0xFFFFFFFF) && (base & 0xFFFFFFFF) < filesize && filesize <= 0xFFFFFFFF)
            base &= 0xFFFFFFFF;
        else
            return -1;
    }

    for(i=0; i<entries_in_use; i++){
        if(index_type){
            int64_t pos= get_le32(pb) + base - 8;
            int len    = get_le32(pb);
            int key= len >= 0;
            len &= 0x7FFFFFFF;

#ifdef DEBUG_SEEK
            av_log(s, AV_LOG_ERROR, "pos:%"PRId64", len:%X\n", pos, len);
#endif
            if(last_pos == pos || pos == base - 8)
                avi->non_interleaved= 1;
            else
                av_add_index_entry(st, pos, ast->cum_len / FFMAX(1, ast->sample_size), len, 0, key ? AVINDEX_KEYFRAME : 0);

            if(ast->sample_size)
                ast->cum_len += len;
            else
                ast->cum_len ++;
            last_pos= pos;
        }else{
            int64_t offset, pos;
            int duration;
            offset = get_le64(pb);
            get_le32(pb);       /* size */
            duration = get_le32(pb);
            pos = url_ftell(pb);

            url_fseek(pb, offset+8, SEEK_SET);
            read_braindead_odml_indx(s, frame_num);
            frame_num += duration;

            url_fseek(pb, pos, SEEK_SET);
        }
    }
    avi->index_loaded=1;
    return 0;
}

static void clean_index(AVFormatContext *s){
    int i;
    int64_t j;

    for(i=0; i<s->nb_streams; i++){
        AVStream *st = s->streams[i];
        AVIStream *ast = st->priv_data;
        int n= st->nb_index_entries;
        int max= ast->sample_size;
        int64_t pos, size, ts;

        if(n != 1 || ast->sample_size==0)
            continue;

        while(max < 1024) max+=max;

        pos= st->index_entries[0].pos;
        size= st->index_entries[0].size;
        ts= st->index_entries[0].timestamp;

        for(j=0; j<size; j+=max){
            av_add_index_entry(st, pos+j, ts + j/ast->sample_size, FFMIN(max, size-j), 0, AVINDEX_KEYFRAME);
        }
    }
}

static int avi_read_tag(ByteIOContext *pb, char *buf, int maxlen,  unsigned int size)
{
    offset_t i = url_ftell(pb);
    size += (size & 1);
    get_strz(pb, buf, maxlen);
    url_fseek(pb, i+size, SEEK_SET);
    return 0;
}

static int avi_read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    AVIContext *avi = s->priv_data;
    ByteIOContext *pb = s->pb;
    uint32_t tag, tag1, handler;
    int codec_type, stream_index, frame_period, bit_rate;
    unsigned int size, nb_frames;
    int i;
    AVStream *st;
    AVIStream *ast = NULL;
    char str_track[4];
    int avih_width=0, avih_height=0;
    int amv_file_format=0;

    avi->stream_index= -1;

    if (get_riff(avi, pb) < 0)
        return -1;

    avi->fsize = url_fsize(pb);
    if(avi->fsize<=0)
        avi->fsize= avi->riff_end;

    /* first list tag */
    stream_index = -1;
    codec_type = -1;
    frame_period = 0;
    for(;;) {
        if (url_feof(pb))
            goto fail;
        tag = get_le32(pb);
        size = get_le32(pb);
#ifdef DEBUG
        print_tag("tag", tag, size);
#endif

        switch(tag) {
        case MKTAG('L', 'I', 'S', 'T'):
            /* ignored, except when start of video packets */
            tag1 = get_le32(pb);
#ifdef DEBUG
            print_tag("list", tag1, 0);
#endif
            if (tag1 == MKTAG('m', 'o', 'v', 'i')) {
                avi->movi_list = url_ftell(pb) - 4;
                if(size) avi->movi_end = avi->movi_list + size + (size & 1);
                else     avi->movi_end = url_fsize(pb);
#ifdef DEBUG
                printf("movi end=%"PRIx64"\n", avi->movi_end);
#endif
                goto end_of_header;
            }
            break;
        case MKTAG('d', 'm', 'l', 'h'):
            avi->is_odml = 1;
            url_fskip(pb, size + (size & 1));
            break;
        case MKTAG('a', 'm', 'v', 'h'):
            amv_file_format=1;
        case MKTAG('a', 'v', 'i', 'h'):
            /* avi header */
            /* using frame_period is bad idea */
            frame_period = get_le32(pb);
            bit_rate = get_le32(pb) * 8;
            get_le32(pb);
            avi->non_interleaved |= get_le32(pb) & AVIF_MUSTUSEINDEX;

            url_fskip(pb, 2 * 4);
            get_le32(pb);
            get_le32(pb);
            avih_width=get_le32(pb);
            avih_height=get_le32(pb);

            url_fskip(pb, size - 10 * 4);
            break;
        case MKTAG('s', 't', 'r', 'h'):
            /* stream header */

            tag1 = get_le32(pb);
            handler = get_le32(pb); /* codec tag */

            if(tag1 == MKTAG('p', 'a', 'd', 's')){
                url_fskip(pb, size - 8);
                break;
            }else{
                stream_index++;
                st = av_new_stream(s, stream_index);
                if (!st)
                    goto fail;

                ast = av_mallocz(sizeof(AVIStream));
                if (!ast)
                    goto fail;
                st->priv_data = ast;
            }
            if(amv_file_format)
                tag1 = stream_index ? MKTAG('a','u','d','s') : MKTAG('v','i','d','s');

#ifdef DEBUG
            print_tag("strh", tag1, -1);
#endif
            if(tag1 == MKTAG('i', 'a', 'v', 's') || tag1 == MKTAG('i', 'v', 'a', 's')){
                int64_t dv_dur;

                /*
                 * After some consideration -- I don't think we
                 * have to support anything but DV in a type1 AVIs.
                 */
                if (s->nb_streams != 1)
                    goto fail;

                if (handler != MKTAG('d', 'v', 's', 'd') &&
                    handler != MKTAG('d', 'v', 'h', 'd') &&
                    handler != MKTAG('d', 'v', 's', 'l'))
                   goto fail;

                ast = s->streams[0]->priv_data;
                av_freep(&s->streams[0]->codec->extradata);
                av_freep(&s->streams[0]);
                s->nb_streams = 0;
                if (ENABLE_DV_DEMUXER) {
                    avi->dv_demux = dv_init_demux(s);
                    if (!avi->dv_demux)
                        goto fail;
                }
                s->streams[0]->priv_data = ast;
                url_fskip(pb, 3 * 4);
                ast->scale = get_le32(pb);
                ast->rate = get_le32(pb);
                url_fskip(pb, 4);  /* start time */

                dv_dur = get_le32(pb);
                if (ast->scale > 0 && ast->rate > 0 && dv_dur > 0) {
                    dv_dur *= AV_TIME_BASE;
                    s->duration = av_rescale(dv_dur, ast->scale, ast->rate);
                }
                /*
                 * else, leave duration alone; timing estimation in utils.c
                 *      will make a guess based on bit rate.
                 */

                stream_index = s->nb_streams - 1;
                url_fskip(pb, size - 9*4);
                break;
            }

            assert(stream_index < s->nb_streams);
            st->codec->stream_codec_tag= handler;

            get_le32(pb); /* flags */
            get_le16(pb); /* priority */
            get_le16(pb); /* language */
            get_le32(pb); /* initial frame */
            ast->scale = get_le32(pb);
            ast->rate = get_le32(pb);
            if(ast->scale && ast->rate){
            }else if(frame_period){
                ast->rate = 1000000;
                ast->scale = frame_period;
            }else{
                ast->rate = 25;
                ast->scale = 1;
            }
            av_set_pts_info(st, 64, ast->scale, ast->rate);

            ast->cum_len=get_le32(pb); /* start */
            nb_frames = get_le32(pb);

            st->start_time = 0;
            st->duration = nb_frames;
            get_le32(pb); /* buffer size */
            get_le32(pb); /* quality */
            ast->sample_size = get_le32(pb); /* sample ssize */
            ast->cum_len *= FFMAX(1, ast->sample_size);
//            av_log(NULL, AV_LOG_DEBUG, "%d %d %d %d\n", ast->rate, ast->scale, ast->start, ast->sample_size);

            switch(tag1) {
            case MKTAG('v', 'i', 'd', 's'):
                codec_type = CODEC_TYPE_VIDEO;

                ast->sample_size = 0;
                break;
            case MKTAG('a', 'u', 'd', 's'):
                codec_type = CODEC_TYPE_AUDIO;
                break;
            case MKTAG('t', 'x', 't', 's'):
                //FIXME
                codec_type = CODEC_TYPE_DATA; //CODEC_TYPE_SUB ?  FIXME
                break;
            default:
                av_log(s, AV_LOG_ERROR, "unknown stream type %X\n", tag1);
                goto fail;
            }
            ast->frame_offset= ast->cum_len;
            url_fskip(pb, size - 12 * 4);
            break;
        case MKTAG('s', 't', 'r', 'f'):
            /* stream header */
            if (stream_index >= (unsigned)s->nb_streams || avi->dv_demux) {
                url_fskip(pb, size);
            } else {
                st = s->streams[stream_index];
                switch(codec_type) {
                case CODEC_TYPE_VIDEO:
                    if(amv_file_format){
                        st->codec->width=avih_width;
                        st->codec->height=avih_height;
                        st->codec->codec_type = CODEC_TYPE_VIDEO;
                        st->codec->codec_id = CODEC_ID_AMV;
                        url_fskip(pb, size);
                        break;
                    }
                    get_le32(pb); /* size */
                    st->codec->width = get_le32(pb);
                    st->codec->height = get_le32(pb);
                    get_le16(pb); /* panes */
                    st->codec->bits_per_sample= get_le16(pb); /* depth */
                    tag1 = get_le32(pb);
                    get_le32(pb); /* ImageSize */
                    get_le32(pb); /* XPelsPerMeter */
                    get_le32(pb); /* YPelsPerMeter */
                    get_le32(pb); /* ClrUsed */
                    get_le32(pb); /* ClrImportant */

                    if (tag1 == MKTAG('D', 'X', 'S', 'B')) {
                        st->codec->codec_type = CODEC_TYPE_SUBTITLE;
                        st->codec->codec_tag = tag1;
                        st->codec->codec_id = CODEC_ID_XSUB;
                        break;
                    }

                    if(size > 10*4 && size<(1<<30)){
                        st->codec->extradata_size= size - 10*4;
                        st->codec->extradata= av_malloc(st->codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
                        get_buffer(pb, st->codec->extradata, st->codec->extradata_size);
                    }

                    if(st->codec->extradata_size & 1) //FIXME check if the encoder really did this correctly
                        get_byte(pb);

                    /* Extract palette from extradata if bpp <= 8 */
                    /* This code assumes that extradata contains only palette */
                    /* This is true for all paletted codecs implemented in ffmpeg */
                    if (st->codec->extradata_size && (st->codec->bits_per_sample <= 8)) {
                        st->codec->palctrl = av_mallocz(sizeof(AVPaletteControl));
#ifdef WORDS_BIGENDIAN
                        for (i = 0; i < FFMIN(st->codec->extradata_size, AVPALETTE_SIZE)/4; i++)
                            st->codec->palctrl->palette[i] = bswap_32(((uint32_t*)st->codec->extradata)[i]);
#else
                        memcpy(st->codec->palctrl->palette, st->codec->extradata,
                               FFMIN(st->codec->extradata_size, AVPALETTE_SIZE));
#endif
                        st->codec->palctrl->palette_changed = 1;
                    }

#ifdef DEBUG
                    print_tag("video", tag1, 0);
#endif
                    st->codec->codec_type = CODEC_TYPE_VIDEO;
                    st->codec->codec_tag = tag1;
                    st->codec->codec_id = codec_get_id(codec_bmp_tags, tag1);
                    st->need_parsing = AVSTREAM_PARSE_HEADERS; // this is needed to get the pict type which is needed for generating correct pts
//                    url_fskip(pb, size - 5 * 4);
                    break;
                case CODEC_TYPE_AUDIO:
                    get_wav_header(pb, st->codec, size);
                    if(ast->sample_size && st->codec->block_align && ast->sample_size % st->codec->block_align)
                        av_log(s, AV_LOG_DEBUG, "invalid sample size or block align detected\n");
                    if (size%2) /* 2-aligned (fix for Stargate SG-1 - 3x18 - Shades of Grey.avi) */
                        url_fskip(pb, 1);
                    /* Force parsing as several audio frames can be in
                     * one packet and timestamps refer to packet start*/
                    st->need_parsing = AVSTREAM_PARSE_TIMESTAMPS;
                    /* ADTS header is in extradata, AAC without header must be stored as exact frames, parser not needed and it will fail */
                    if (st->codec->codec_id == CODEC_ID_AAC && st->codec->extradata_size)
                        st->need_parsing = AVSTREAM_PARSE_NONE;
                    /* AVI files with Xan DPCM audio (wrongly) declare PCM
                     * audio in the header but have Axan as stream_code_tag. */
                    if (st->codec->stream_codec_tag == ff_get_fourcc("Axan")){
                        st->codec->codec_id  = CODEC_ID_XAN_DPCM;
                        st->codec->codec_tag = 0;
                    }
                    if (amv_file_format)
                        st->codec->codec_id  = CODEC_ID_ADPCM_IMA_AMV;
                    break;
                default:
                    st->codec->codec_type = CODEC_TYPE_DATA;
                    st->codec->codec_id= CODEC_ID_NONE;
                    st->codec->codec_tag= 0;
                    url_fskip(pb, size);
                    break;
                }
            }
            break;
        case MKTAG('i', 'n', 'd', 'x'):
            i= url_ftell(pb);
            if(!url_is_streamed(pb) && !(s->flags & AVFMT_FLAG_IGNIDX)){
                read_braindead_odml_indx(s, 0);
            }
            url_fseek(pb, i+size, SEEK_SET);
            break;
        case MKTAG('I', 'N', 'A', 'M'):
            avi_read_tag(pb, s->title, sizeof(s->title), size);
            break;
        case MKTAG('I', 'A', 'R', 'T'):
            avi_read_tag(pb, s->author, sizeof(s->author), size);
            break;
        case MKTAG('I', 'C', 'O', 'P'):
            avi_read_tag(pb, s->copyright, sizeof(s->copyright), size);
            break;
        case MKTAG('I', 'C', 'M', 'T'):
            avi_read_tag(pb, s->comment, sizeof(s->comment), size);
            break;
        case MKTAG('I', 'G', 'N', 'R'):
            avi_read_tag(pb, s->genre, sizeof(s->genre), size);
            break;
        case MKTAG('I', 'P', 'R', 'D'):
            avi_read_tag(pb, s->album, sizeof(s->album), size);
            break;
        case MKTAG('I', 'P', 'R', 'T'):
            avi_read_tag(pb, str_track, sizeof(str_track), size);
            sscanf(str_track, "%d", &s->track);
            break;
        default:
            if(size > 1000000){
                av_log(s, AV_LOG_ERROR, "well something went wrong during header parsing, "
                                        "ill ignore it and try to continue anyway\n");
                avi->movi_list = url_ftell(pb) - 4;
                avi->movi_end  = url_fsize(pb);
                goto end_of_header;
            }
            /* skip tag */
            size += (size & 1);
            url_fskip(pb, size);
            break;
        }
    }
 end_of_header:
    /* check stream number */
    if (stream_index != s->nb_streams - 1) {
    fail:
        for(i=0;i<s->nb_streams;i++) {
            av_freep(&s->streams[i]->codec->extradata);
            av_freep(&s->streams[i]);
        }
        return -1;
    }

    if(!avi->index_loaded && !url_is_streamed(pb))
        avi_load_index(s);
    avi->index_loaded = 1;
    avi->non_interleaved |= guess_ni_flag(s);
    if(avi->non_interleaved)
        clean_index(s);

    return 0;
}

static int avi_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    AVIContext *avi = s->priv_data;
    ByteIOContext *pb = s->pb;
    int n, d[8], size;
    offset_t i, sync;
    void* dstr;

    if (ENABLE_DV_DEMUXER && avi->dv_demux) {
        size = dv_get_packet(avi->dv_demux, pkt);
        if (size >= 0)
            return size;
    }

    if(avi->non_interleaved){
        int best_stream_index = 0;
        AVStream *best_st= NULL;
        AVIStream *best_ast;
        int64_t best_ts= INT64_MAX;
        int i;

        for(i=0; i<s->nb_streams; i++){
            AVStream *st = s->streams[i];
            AVIStream *ast = st->priv_data;
            int64_t ts= ast->frame_offset;

            if(ast->sample_size)
                ts /= ast->sample_size;
            ts= av_rescale(ts, AV_TIME_BASE * (int64_t)st->time_base.num, st->time_base.den);

//            av_log(NULL, AV_LOG_DEBUG, "%"PRId64" %d/%d %"PRId64"\n", ts, st->time_base.num, st->time_base.den, ast->frame_offset);
            if(ts < best_ts){
                best_ts= ts;
                best_st= st;
                best_stream_index= i;
            }
        }
        best_ast = best_st->priv_data;
        best_ts= av_rescale(best_ts, best_st->time_base.den, AV_TIME_BASE * (int64_t)best_st->time_base.num); //FIXME a little ugly
        if(best_ast->remaining)
            i= av_index_search_timestamp(best_st, best_ts, AVSEEK_FLAG_ANY | AVSEEK_FLAG_BACKWARD);
        else
            i= av_index_search_timestamp(best_st, best_ts, AVSEEK_FLAG_ANY);

//        av_log(NULL, AV_LOG_DEBUG, "%d\n", i);
        if(i>=0){
            int64_t pos= best_st->index_entries[i].pos;
            pos += best_ast->packet_size - best_ast->remaining;
            url_fseek(s->pb, pos + 8, SEEK_SET);
//        av_log(NULL, AV_LOG_DEBUG, "pos=%"PRId64"\n", pos);

            assert(best_ast->remaining <= best_ast->packet_size);

            avi->stream_index= best_stream_index;
            if(!best_ast->remaining)
                best_ast->packet_size=
                best_ast->remaining= best_st->index_entries[i].size;
        }
    }

resync:
    if(avi->stream_index >= 0){
        AVStream *st= s->streams[ avi->stream_index ];
        AVIStream *ast= st->priv_data;
        int size;

        if(ast->sample_size <= 1) // minorityreport.AVI block_align=1024 sample_size=1 IMA-ADPCM
            size= INT_MAX;
        else if(ast->sample_size < 32)
            size= 64*ast->sample_size;
        else
            size= ast->sample_size;

        if(size > ast->remaining)
            size= ast->remaining;
        av_get_packet(pb, pkt, size);

        if (ENABLE_DV_DEMUXER && avi->dv_demux) {
            dstr = pkt->destruct;
            size = dv_produce_packet(avi->dv_demux, pkt,
                                    pkt->data, pkt->size);
            pkt->destruct = dstr;
            pkt->flags |= PKT_FLAG_KEY;
        } else {
            /* XXX: how to handle B frames in avi ? */
            pkt->dts = ast->frame_offset;
//                pkt->dts += ast->start;
            if(ast->sample_size)
                pkt->dts /= ast->sample_size;
//av_log(NULL, AV_LOG_DEBUG, "dts:%"PRId64" offset:%"PRId64" %d/%d smpl_siz:%d base:%d st:%d size:%d\n", pkt->dts, ast->frame_offset, ast->scale, ast->rate, ast->sample_size, AV_TIME_BASE, avi->stream_index, size);
            pkt->stream_index = avi->stream_index;

            if (st->codec->codec_type == CODEC_TYPE_VIDEO) {
                AVIndexEntry *e;
                int index;
                assert(st->index_entries);

                index= av_index_search_timestamp(st, pkt->dts, 0);
                e= &st->index_entries[index];

                if(index >= 0 && e->timestamp == ast->frame_offset){
                    if (e->flags & AVINDEX_KEYFRAME)
                        pkt->flags |= PKT_FLAG_KEY;
                }
            } else {
                pkt->flags |= PKT_FLAG_KEY;
            }
            if(ast->sample_size)
                ast->frame_offset += pkt->size;
            else
                ast->frame_offset++;
        }
        ast->remaining -= size;
        if(!ast->remaining){
            avi->stream_index= -1;
            ast->packet_size= 0;
        }

        return size;
    }

    memset(d, -1, sizeof(int)*8);
    for(i=sync=url_ftell(pb); !url_feof(pb); i++) {
        int j;

        for(j=0; j<7; j++)
            d[j]= d[j+1];
        d[7]= get_byte(pb);

        size= d[4] + (d[5]<<8) + (d[6]<<16) + (d[7]<<24);

        if(    d[2] >= '0' && d[2] <= '9'
            && d[3] >= '0' && d[3] <= '9'){
            n= (d[2] - '0') * 10 + (d[3] - '0');
        }else{
            n= 100; //invalid stream id
        }
//av_log(NULL, AV_LOG_DEBUG, "%X %X %X %X %X %X %X %X %"PRId64" %d %d\n", d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], i, size, n);
        if(i + size > avi->fsize || d[0]<0)
            continue;

        //parse ix##
        if(  (d[0] == 'i' && d[1] == 'x' && n < s->nb_streams)
        //parse JUNK
           ||(d[0] == 'J' && d[1] == 'U' && d[2] == 'N' && d[3] == 'K')
           ||(d[0] == 'i' && d[1] == 'd' && d[2] == 'x' && d[3] == '1')){
            url_fskip(pb, size);
//av_log(NULL, AV_LOG_DEBUG, "SKIP\n");
            goto resync;
        }

        if(    d[0] >= '0' && d[0] <= '9'
            && d[1] >= '0' && d[1] <= '9'){
            n= (d[0] - '0') * 10 + (d[1] - '0');
        }else{
            n= 100; //invalid stream id
        }

        //parse ##dc/##wb
        if(n < s->nb_streams){
          AVStream *st;
          AVIStream *ast;
          st = s->streams[n];
          ast = st->priv_data;

          if(   (st->discard >= AVDISCARD_DEFAULT && size==0)
             /*|| (st->discard >= AVDISCARD_NONKEY && !(pkt->flags & PKT_FLAG_KEY))*/ //FIXME needs a little reordering
             || st->discard >= AVDISCARD_ALL){
                if(ast->sample_size) ast->frame_offset += pkt->size;
                else                 ast->frame_offset++;
                url_fskip(pb, size);
                goto resync;
          }

          if(   ((ast->prefix_count<5 || sync+9 > i) && d[2]<128 && d[3]<128) ||
                d[2]*256+d[3] == ast->prefix /*||
                (d[2] == 'd' && d[3] == 'c') ||
                (d[2] == 'w' && d[3] == 'b')*/) {

//av_log(NULL, AV_LOG_DEBUG, "OK\n");
            if(d[2]*256+d[3] == ast->prefix)
                ast->prefix_count++;
            else{
                ast->prefix= d[2]*256+d[3];
                ast->prefix_count= 0;
            }

            avi->stream_index= n;
            ast->packet_size= size + 8;
            ast->remaining= size;

            {
                uint64_t pos= url_ftell(pb) - 8;
                if(!st->index_entries || !st->nb_index_entries || st->index_entries[st->nb_index_entries - 1].pos < pos){
                    av_add_index_entry(st, pos, ast->frame_offset / FFMAX(1, ast->sample_size), size, 0, AVINDEX_KEYFRAME);
                }
            }
            goto resync;
          }
        }
        /* palette changed chunk */
        if (   d[0] >= '0' && d[0] <= '9'
            && d[1] >= '0' && d[1] <= '9'
            && ((d[2] == 'p' && d[3] == 'c'))
            && n < s->nb_streams && i + size <= avi->fsize) {

            AVStream *st;
            int first, clr, flags, k, p;

            st = s->streams[n];

            first = get_byte(pb);
            clr = get_byte(pb);
            if(!clr) /* all 256 colors used */
                clr = 256;
            flags = get_le16(pb);
            p = 4;
            for (k = first; k < clr + first; k++) {
                int r, g, b;
                r = get_byte(pb);
                g = get_byte(pb);
                b = get_byte(pb);
                    get_byte(pb);
                st->codec->palctrl->palette[k] = b + (g << 8) + (r << 16);
            }
            st->codec->palctrl->palette_changed = 1;
            goto resync;
        }

    }

    return -1;
}

/* XXX: we make the implicit supposition that the position are sorted
   for each stream */
static int avi_read_idx1(AVFormatContext *s, int size)
{
    AVIContext *avi = s->priv_data;
    ByteIOContext *pb = s->pb;
    int nb_index_entries, i;
    AVStream *st;
    AVIStream *ast;
    unsigned int index, tag, flags, pos, len;
    unsigned last_pos= -1;

    nb_index_entries = size / 16;
    if (nb_index_entries <= 0)
        return -1;

    /* read the entries and sort them in each stream component */
    for(i = 0; i < nb_index_entries; i++) {
        tag = get_le32(pb);
        flags = get_le32(pb);
        pos = get_le32(pb);
        len = get_le32(pb);
#if defined(DEBUG_SEEK)
        av_log(NULL, AV_LOG_DEBUG, "%d: tag=0x%x flags=0x%x pos=0x%x len=%d/",
               i, tag, flags, pos, len);
#endif
        if(i==0 && pos > avi->movi_list)
            avi->movi_list= 0; //FIXME better check
        pos += avi->movi_list;

        index = ((tag & 0xff) - '0') * 10;
        index += ((tag >> 8) & 0xff) - '0';
        if (index >= s->nb_streams)
            continue;
        st = s->streams[index];
        ast = st->priv_data;

#if defined(DEBUG_SEEK)
        av_log(NULL, AV_LOG_DEBUG, "%d cum_len=%"PRId64"\n", len, ast->cum_len);
#endif
        if(last_pos == pos)
            avi->non_interleaved= 1;
        else
            av_add_index_entry(st, pos, ast->cum_len / FFMAX(1, ast->sample_size), len, 0, (flags&AVIIF_INDEX) ? AVINDEX_KEYFRAME : 0);
        if(ast->sample_size)
            ast->cum_len += len;
        else
            ast->cum_len ++;
        last_pos= pos;
    }
    return 0;
}

static int guess_ni_flag(AVFormatContext *s){
    int i;
    int64_t last_start=0;
    int64_t first_end= INT64_MAX;

    for(i=0; i<s->nb_streams; i++){
        AVStream *st = s->streams[i];
        int n= st->nb_index_entries;

        if(n <= 0)
            continue;

        if(st->index_entries[0].pos > last_start)
            last_start= st->index_entries[0].pos;
        if(st->index_entries[n-1].pos < first_end)
            first_end= st->index_entries[n-1].pos;
    }
    return last_start > first_end;
}

static int avi_load_index(AVFormatContext *s)
{
    AVIContext *avi = s->priv_data;
    ByteIOContext *pb = s->pb;
    uint32_t tag, size;
    offset_t pos= url_ftell(pb);

    url_fseek(pb, avi->movi_end, SEEK_SET);
#ifdef DEBUG_SEEK
    printf("movi_end=0x%"PRIx64"\n", avi->movi_end);
#endif
    for(;;) {
        if (url_feof(pb))
            break;
        tag = get_le32(pb);
        size = get_le32(pb);
#ifdef DEBUG_SEEK
        printf("tag=%c%c%c%c size=0x%x\n",
               tag & 0xff,
               (tag >> 8) & 0xff,
               (tag >> 16) & 0xff,
               (tag >> 24) & 0xff,
               size);
#endif
        switch(tag) {
        case MKTAG('i', 'd', 'x', '1'):
            if (avi_read_idx1(s, size) < 0)
                goto skip;
            else
                goto the_end;
            break;
        default:
        skip:
            size += (size & 1);
            url_fskip(pb, size);
            break;
        }
    }
 the_end:
    url_fseek(pb, pos, SEEK_SET);
    return 0;
}

static int avi_read_seek(AVFormatContext *s, int stream_index, int64_t timestamp, int flags)
{
    AVIContext *avi = s->priv_data;
    AVStream *st;
    int i, index;
    int64_t pos;

    if (!avi->index_loaded) {
        /* we only load the index on demand */
        avi_load_index(s);
        avi->index_loaded = 1;
    }
    assert(stream_index>= 0);

    st = s->streams[stream_index];
    index= av_index_search_timestamp(st, timestamp, flags);
    if(index<0)
        return -1;

    /* find the position */
    pos = st->index_entries[index].pos;
    timestamp = st->index_entries[index].timestamp;

//    av_log(NULL, AV_LOG_DEBUG, "XX %"PRId64" %d %"PRId64"\n", timestamp, index, st->index_entries[index].timestamp);

    if (ENABLE_DV_DEMUXER && avi->dv_demux) {
        /* One and only one real stream for DV in AVI, and it has video  */
        /* offsets. Calling with other stream indices should have failed */
        /* the av_index_search_timestamp call above.                     */
        assert(stream_index == 0);

        /* Feed the DV video stream version of the timestamp to the */
        /* DV demux so it can synth correct timestamps              */
        dv_offset_reset(avi->dv_demux, timestamp);

        url_fseek(s->pb, pos, SEEK_SET);
        avi->stream_index= -1;
        return 0;
    }

    for(i = 0; i < s->nb_streams; i++) {
        AVStream *st2 = s->streams[i];
        AVIStream *ast2 = st2->priv_data;

        ast2->packet_size=
        ast2->remaining= 0;

        if (st2->nb_index_entries <= 0)
            continue;

//        assert(st2->codec->block_align);
        assert(st2->time_base.den == ast2->rate);
        assert(st2->time_base.num == ast2->scale);
        index = av_index_search_timestamp(
                st2,
                av_rescale(timestamp, st2->time_base.den*(int64_t)st->time_base.num, st->time_base.den * (int64_t)st2->time_base.num),
                flags | AVSEEK_FLAG_BACKWARD);
        if(index<0)
            index=0;

        if(!avi->non_interleaved){
            while(index>0 && st2->index_entries[index].pos > pos)
                index--;
            while(index+1 < st2->nb_index_entries && st2->index_entries[index].pos < pos)
                index++;
        }

//        av_log(NULL, AV_LOG_DEBUG, "%"PRId64" %d %"PRId64"\n", timestamp, index, st2->index_entries[index].timestamp);
        /* extract the current frame number */
        ast2->frame_offset = st2->index_entries[index].timestamp;
        if(ast2->sample_size)
            ast2->frame_offset *=ast2->sample_size;
    }

    /* do the seek */
    url_fseek(s->pb, pos, SEEK_SET);
    avi->stream_index= -1;
    return 0;
}

static int avi_read_close(AVFormatContext *s)
{
    int i;
    AVIContext *avi = s->priv_data;

    for(i=0;i<s->nb_streams;i++) {
        AVStream *st = s->streams[i];
        AVIStream *ast = st->priv_data;
        av_free(ast);
        av_free(st->codec->palctrl);
    }

    if (avi->dv_demux)
        av_free(avi->dv_demux);

    return 0;
}

static int avi_probe(AVProbeData *p)
{
    int i;

    /* check file header */
    for(i=0; avi_headers[i][0]; i++)
        if(!memcmp(p->buf  , avi_headers[i]  , 4) &&
           !memcmp(p->buf+8, avi_headers[i]+4, 4))
            return AVPROBE_SCORE_MAX;

    return 0;
}

AVInputFormat avi_demuxer = {
    "avi",
    "avi format",
    sizeof(AVIContext),
    avi_probe,
    avi_read_header,
    avi_read_packet,
    avi_read_close,
    avi_read_seek,
};
