#ifndef CBOT_H
#define CBOT_H

#include "discord.h"
#include "bank.h"

struct cbot_t;
struct cmd_t;

struct cbot_t
{
   struct discord *client;
   char *prefix;
   short prefix_len;

   struct bank_users_t bank_users;
   char *bank_save_filename;

   const u64snowflake master_id;
   u64snowflake bot_id;

   struct cmd_t *cmds;
   uint64_t seed;
};

void cbot_on_ready (struct cbot_t *cbot, struct discord *client, const struct discord_ready *event);
void cbot_on_interaction (struct cbot_t *cbot, const struct discord_interaction *event);
void cbot_on_message (struct cbot_t *cbot, const struct discord_message *event);

typedef void (*cmd_handler_t) (struct cbot_t *cbot, const struct discord_message *event, const char *cmd);

struct cmd_t
{
   char *name;
   char *desc;
   cmd_handler_t run;
   bool owner_only;
};

#ifndef NULL_CMD
#define NULL_CMD                                                                                                                \
   (struct cmd_t) { "NULL", "NULL", NULL, true }
#endif /* NULL_CMD */

#endif /* CBOT_H */
