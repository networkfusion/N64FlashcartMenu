/**
 * @file e664_vseries_state.h
 * @brief ED64 vseries state
 * @ingroup flashcart
 */

#ifndef FLASHCART_ED64_V_STATE_H__
#define FLASHCART_ED64_V_STATE_H__

/** @brief ED64 Vseries Pseudo Writeback Structure */
typedef struct {
/** @brief The path to the last loaded ROM save path */
    char *last_save_path;
/** @brief The last used save type */
    int last_save_type;
/** @brief The reset button was used */
    bool is_expecting_save_writeback;
} ed64_vseries_pseudo_writeback_t;

void ed64_vseries_state_load (ed64_vseries_pseudo_writeback_t *state);
void ed64_vseries_state_save (ed64_vseries_pseudo_writeback_t *state);

#endif
