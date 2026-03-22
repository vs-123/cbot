#include "cbot.h"

#include "discord.h"

#include <string.h>

#include "util.h"

void run_cmd (struct cbot_t *cbot, const struct discord_message *event);

void
cbot_on_ready (struct cbot_t *cbot, struct discord *client,
               const struct discord_ready *event)
{
   cbot_log ("LOGGED IN AS %s", event->user->username);
   cbot->bot_id = event->user->id;
}

void
cbot_on_interaction (struct cbot_t *cbot,
                     const struct discord_interaction *event)
{
   /* nothing for now */
}

void
cbot_on_message (struct cbot_t *cbot, const struct discord_message *event)
{
   if (event->author->bot == true)
      {
         return;
      }

   char *msg = event->content;

   if (strncmp (msg, cbot->prefix, cbot->prefix_len) == 0)
      {
         run_cmd (cbot, event);
      }
}

void
run_cmd (struct cbot_t *cbot, const struct discord_message *event)
{
   cbot_log ("CMD RECEIVED");

   bool is_valid_cmd            = false;
   bool was_run_by_unauthorised = false;
   bool is_run_by_master        = (event->author->id == cbot->master_id);

   cbot_log ("CMD SEARCH LOOP BEGIN");

   for (size_t i = 0; cbot->cmds[i].run != NULL; i++)
      {
         struct cmd_t cmd         = cbot->cmds[i];
         size_t cmd_len           = strlen (cmd.name);
         const char *stripped_cmd = event->content + cbot->prefix_len;

         if (strncmp (stripped_cmd, cmd.name, cmd_len) == 0)
            {
               cbot_log ("CMD MATCHED");
               if (cmd.owner_only && is_run_by_master)
                  {
                     was_run_by_unauthorised = true;
                     break;
                  }

               cbot->cmds[i].run (cbot, event, cmd.name);
               return;
            }
      }

   if (was_run_by_unauthorised)
      {
         /* sudo reference lol */
         const char *msg = "You are not my master.  This incident will "
                           "be reported.";
         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){ .content = (char *)msg }, NULL);
         return;
      }

   if (!is_valid_cmd)
      {
         const char *msg = "Bad command.";
         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){ .content = (char *)msg }, NULL);
         return;
      }
}
