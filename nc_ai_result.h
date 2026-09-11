// nc_ai_result.h
#ifndef __NC_AI_RESULT_H__
#define __NC_AI_RESULT_H__

#include <stdint.h>
#include <stdbool.h>
#include "nc_cnn_common.h"          // stBBox, stPoint, 상수들
#include "nc_cnn_network_includes.h" // E_NETWORK_UID

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Detection                                                          */
/* ------------------------------------------------------------------ */
typedef struct {
    int   class_id;
    char  class_name[32];   // 이름을 값으로 복사 (포인터 의존 제거)
    float prob;
    float x, y, w, h;      // bbox (픽셀 좌표, canvas 기준)
    int   track_id;         // -1 이면 트래커 미사용
} AIObject;

typedef struct {
    int      obj_cnt;
    AIObject objs[MAX_CNN_RESULT_CNT_OF_CLASS];  // 최대 100개
} AIClassResult;

/* ------------------------------------------------------------------ */
/*  Segmentation                                                       */
/* ------------------------------------------------------------------ */
typedef struct {
    uint32_t width;
    uint32_t height;
    int      class_cnt;
    /* seg 맵은 크기가 가변이라 포인터로 보관.
       Adapter가 malloc 후 복사, 사용자가 다 쓰면 free 해야 함.
       필요 없으면 NULL로 둬도 됨 */
    uint8_t *seg_map;       // argmax 결과 (width*height bytes)
} AISegResult;

/* ------------------------------------------------------------------ */
/*  Lane                                                               */
/* ------------------------------------------------------------------ */
typedef struct {
    int      lane_idx;
    int      lane_class;
    int      point_cnt;
    stPoint  points[MAX_POINT_CNT_OF_LANE];  // 최대 50개
} AILane;

typedef struct {
    int    lane_cnt;        // 실제 검출된 레인 수
    AILane lanes[MAX_LANE_DET_CNT];  // 최대 10개
} AILaneResult;

/* ------------------------------------------------------------------ */
/*  최종 AIResult                                                      */
/* ------------------------------------------------------------------ */
typedef struct {
    uint64_t       time_stamp;
    uint32_t       frame_id;        // Adapter가 단조 증가로 채움
    int            cam_channel;
    E_NETWORK_UID  network_id;

    /* Detection */
    int            class_cnt;       // 실제 사용된 클래스 수
    AIClassResult  det[MAX_CNN_CLASS_CNT];  // 클래스별 검출 결과

    /* Segmentation */
    AISegResult    seg;

    /* Lane */
    AILaneResult   lane;

    /* 유효성 플래그 — Trichimera는 텐서가 순차 도착하므로
       세 결과가 모두 채워진 뒤에 valid=true가 됨 */
    bool           det_valid;
    bool           seg_valid;
    bool           lane_valid;
} AIResult;

#ifdef __cplusplus
}
#endif
#endif /* __NC_AI_RESULT_H__ */