#ifndef CBOT_H
#define CBOT_H

#include "concord/gencodecs/discord_codecs.h"
#include "concord/include/discord.h"

struct cbot_t;
struct cmd_t;

struct cbot_t
{
   struct discord *client;
   char *prefix;

   const u64snowflake master_id;
   u64snowflake bot_id;

   struct cmd_t *cmds;
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

#endif /* CBOT_H */
