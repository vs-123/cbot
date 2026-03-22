#include "cmds.h"

#include <string.h>
#include <stdlib.h>

#include "cbot.h"
#include "discord.h"

void
cmd_help (struct cbot_t *cbot, const struct discord_message *event,
          const char *cmd_name)
{
   char final_msg[4096];
   char tmp_msg[512];

   bool is_master = (event->author->id == cbot->master_id);
   sprintf (final_msg, "List of commands for <@!%llu>:\n", event->author->id);

   struct cmd_t *cmd = NULL;
   for (size_t i = 0; (cmd = &cbot->cmds[i])->run != NULL; i++)
      {
         bool should_user_see_cmd
             = (is_master) || (!cmd->owner_only && !is_master);
         if (should_user_see_cmd)
            {
               const char *master_only_notice
                   = cmd->owner_only ? "-- **(master only)**" : "";
               sprintf (tmp_msg, "- `%s` -- %s %s\n", cmd->name, cmd->desc,
                        master_only_notice);
               strcat (final_msg, tmp_msg);
            }
      }

   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = final_msg }, NULL);
}

void
cmd_die (struct cbot_t *cbot, const struct discord_message *event,
         const char *cmd)
{
   struct discord_ret_message ret = { .sync = DISCORD_SYNC_FLAG };

   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = "Goodbye, master! X_X" }, &ret);

   curl_global_cleanup ();
   discord_shutdown (cbot->client);
}
