// nc_trichimera_adapter.c
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "nc_trichimera_adapter.h"
#include "nc_cnn_config_parser.h"   // stNetwork_info

/* VIDEO_MAX_CH 는 wayland_npu_app.c 와 동일한 값 사용 */
#ifndef VIDEO_MAX_CH
#define VIDEO_MAX_CH 2
#endif

/* 채널별 내부 상태 */
static AIResult  g_ai_result[VIDEO_MAX_CH];
static pthread_mutex_t g_mutex[VIDEO_MAX_CH];
static uint32_t  g_frame_id[VIDEO_MAX_CH];

/* ------------------------------------------------------------------ */
void nc_trichimera_adapter_init(void)
{
    for (int i = 0; i < VIDEO_MAX_CH; i++) {
        pthread_mutex_init(&g_mutex[i], NULL);
        memset(&g_ai_result[i], 0, sizeof(AIResult));
        g_frame_id[i] = 0;
    }
}

/* ------------------------------------------------------------------ */
void nc_trichimera_adapter_update(int cam_ch,
                                  const pp_result_buf *buf,
                                  const stNetwork_info *net_info)
{
    if (!buf) return;
    if (cam_ch < 0 || cam_ch >= VIDEO_MAX_CH) return;

    pthread_mutex_lock(&g_mutex[cam_ch]);

    AIResult *dst = &g_ai_result[cam_ch];

    /* 공통 필드 — 가장 마지막에 도착한 버퍼의 타임스탬프로 덮어씀 */
    dst->time_stamp   = buf->time_stamp;
    dst->cam_channel  = buf->cam_channel;
    dst->network_id   = buf->net_id;

    const stCnnPostprocessingResults *src = &buf->cnn_result;

    switch (buf->net_task) {

    /* ---- DETECTION ---- */
    case DETECTION: {
        int class_cnt = buf->draw_info.max_class_cnt;
        dst->class_cnt = class_cnt;

        for (int c = 0; c < class_cnt && c < MAX_CNN_CLASS_CNT; c++) {
            AIClassResult *dcls = &dst->det[c];
            const stClassObjs *scls = &src->class_objs[c];

            dcls->obj_cnt = scls->obj_cnt;
            for (int o = 0; o < scls->obj_cnt && o < MAX_CNN_RESULT_CNT_OF_CLASS; o++) {
                AIObject *dobj = &dcls->objs[o];
                const stObjInfo *sobj = &scls->objs[o];

                dobj->class_id  = scls->class_id;
                dobj->prob      = sobj->prob;
                dobj->x         = sobj->bbox.x;
                dobj->y         = sobj->bbox.y;
                dobj->w         = sobj->bbox.w;
                dobj->h         = sobj->bbox.h;
                dobj->track_id  = sobj->track_id;

                /* 클래스 이름 복사 — net_info가 있을 때만 */
                if (net_info && net_info->class_name && net_info->class_name[c]) {
                    snprintf(dobj->class_name, sizeof(dobj->class_name),
                             "%s", net_info->class_name[c]);
                } else {
                    snprintf(dobj->class_name, sizeof(dobj->class_name),
                             "class_%d", c);
                }
            }
        }
        dst->det_valid = true;
        break;
    }

    /* ---- SEGMENTATION ---- */
    case SEGMENTATION: {
        dst->seg.width     = buf->seg_info.width;
        dst->seg.height    = buf->seg_info.height;
        dst->seg.class_cnt = buf->seg_info.max_class_cnt;

        /* seg_map: 크기가 width*height bytes */
        uint32_t map_size = dst->seg.width * dst->seg.height;
        if (map_size > 0 && src->seg != NULL) {
            if (dst->seg.seg_map) free(dst->seg.seg_map);
            dst->seg.seg_map = (uint8_t *)malloc(map_size);
            if (dst->seg.seg_map) {
                memcpy(dst->seg.seg_map, src->seg, map_size);
            }
        }
        dst->seg_valid = true;
        break;
    }

    /* ---- LANE ---- */
    case LANE: {
        int max_lane = buf->lane_draw_info.max_lane_num;
        int detected = 0;

        for (int l = 0; l < max_lane && l < MAX_LANE_DET_CNT; l++) {
            const stLaneDet *sl = &src->lane_det[l];
            if (sl->point_cnt == 0) continue;

            AILane *dl = &dst->lane.lanes[detected];
            dl->lane_idx   = l;
            dl->lane_class = sl->lane_class;
            dl->point_cnt  = sl->point_cnt;
            memcpy(dl->points, sl->point,
                   sl->point_cnt * sizeof(stPoint));
            detected++;
        }
        dst->lane.lane_cnt = detected;
        dst->lane_valid    = true;

        /* 레인이 마지막으로 도착 → frame_id 증가 */
        dst->frame_id = ++g_frame_id[cam_ch];
        break;
    }

    default:
        break;
    }

    pthread_mutex_unlock(&g_mutex[cam_ch]);
}

/* ------------------------------------------------------------------ */
bool getTrichimeraResult(int cam_ch, AIResult *out)
{
    if (!out) return false;
    if (cam_ch < 0 || cam_ch >= VIDEO_MAX_CH) return false;

    pthread_mutex_lock(&g_mutex[cam_ch]);

    AIResult *src = &g_ai_result[cam_ch];
    bool ready = src->det_valid && src->seg_valid && src->lane_valid;

    if (ready) {
        /* seg_map은 포인터라 별도 처리 */
        *out = *src;
        if (src->seg.seg_map && src->seg.width > 0 && src->seg.height > 0) {
            uint32_t map_size = src->seg.width * src->seg.height;
            out->seg.seg_map = (uint8_t *)malloc(map_size);
            if (out->seg.seg_map) {
                memcpy(out->seg.seg_map, src->seg.seg_map, map_size);
            }
        } else {
            out->seg.seg_map = NULL;
        }
    }

    pthread_mutex_unlock(&g_mutex[cam_ch]);
    return ready;
}