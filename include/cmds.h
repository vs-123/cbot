#ifndef CMDS_H
#define CMDS_H

#include "discord.h"
#include "cbot.h"

/*************/
/*  HELPERS  */
/*************/

struct cmd_args_t
{
   const char *ptr;
};

void args_init          (struct cmd_args_t *args, const char *content);
int args_next_number    (struct cmd_args_t *args, uint64_t *out);
int args_next_snowflake (struct cmd_args_t *args, uint64_t *out);
int args_next_mention   (struct cmd_args_t *args, uint64_t *out);
int args_next_amount    (struct cmd_args_t *args, unsigned int *out);

/**********/
/*  CMDS  */
/**********/

void cmd_bf   (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);
void cmd_die  (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);
void cmd_help (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);
void cmd_id   (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);
void cmd_rand (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);
void cmd_seed (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);

/***************/
/*  BANK CMDS  */
/***************/

void bank_cmd_add       (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);
void bank_cmd_bal       (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);
void bank_cmd_lb        (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);
void bank_cmd_gamble    (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);
void bank_cmd_oreg      (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);
void bank_cmd_reg       (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);
void bank_cmd_sub       (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);
void bank_cmd_transfer  (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);
void bank_cmd_work      (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);

#endif /* CMDS_H */
