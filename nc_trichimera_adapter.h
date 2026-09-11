// nc_trichimera_adapter.h
#ifndef __NC_TRICHIMERA_ADAPTER_H__
#define __NC_TRICHIMERA_ADAPTER_H__

#include <stdint.h>
#include <stdbool.h>
#include "nc_cnn_aiware_runtime.h"      // pp_result_buf, E_NETWORK_UID 등
#include "nc_cnn_config_parser.h"       // stNetwork_info, nc_cnn_get_network_info ← 이게 빠진 것
#include "nc_cnn_network_includes.h"    // E_NETWORK_UID, E_NETWORK_TASK
#include "nc_ai_result.h"               // AIResult

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Adapter 초기화. 앱 시작 시 한 번 호출.
 */
void nc_trichimera_adapter_init(void);

/**
 * @brief  pp_result_buf 하나를 받아 AIResult에 반영.
 *         render 루프에서 det/seg/lane 버퍼를 읽은 직후 각각 호출.
 *
 * @param cam_ch   카메라 채널 번호
 * @param buf      nc_tsfs_ff_get_readable_buffer로 얻은 pp_result_buf*
 *                 (NULL이면 아무것도 하지 않음)
 * @param net_info Trichimera의 stNetwork_info* (클래스 이름 복사에 사용)
 *                 detection 이외의 task에서는 NULL 가능
 */
void nc_trichimera_adapter_update(int cam_ch,
                                  const pp_result_buf *buf,
                                  const stNetwork_info *net_info);

/**
 * @brief  최신 AIResult를 out에 복사해 반환.
 *         세 결과(det/seg/lane) 중 하나라도 아직 도착하지 않았으면 false.
 *
 * @param cam_ch  카메라 채널 번호
 * @param out     결과를 받을 AIResult 포인터 (호출자가 할당)
 * @return true   out에 유효한 결과가 복사됨
 *         false  아직 결과 없음
 */
bool getTrichimeraResult(int cam_ch, AIResult *out);

#ifdef __cplusplus
}
#endif
#endif /* __NC_TRICHIMERA_ADAPTER_H__ */