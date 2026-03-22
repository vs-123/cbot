#include "cmds.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "cbot.h"
#include "channel.h"
#include "discord.h"
#include "util.h"
#include "ystar.h"

/*************/
/*  HELPERS  */
/*************/

void
args_init (struct cmd_args_t *args, const char *content)
{
   const char *space = strchr (content, ' ');
   args->ptr         = space ? space + 1 : "";
}

int
args_next_number (struct cmd_args_t *args, uint64_t *out)
{
   while (*args->ptr && !isdigit ((unsigned char)*args->ptr))
      {
         args->ptr++;
      }

   if (*args->ptr == '\0')
      {
         return 0;
      }

   char *endptr;
   *out = strtol (args->ptr, &endptr, 10);

   if (args->ptr == endptr)
      {
         return 0;
      }

   args->ptr = endptr;
   return 1;
}

int
args_next_snowflake (struct cmd_args_t *args, uint64_t *out)
{
   while (*args->ptr && !isdigit ((unsigned char)*args->ptr))
      {
         args->ptr++;
      }

   if (!*args->ptr)
      {
         return 0;
      }

   char *end;
   *out = strtoull (args->ptr, &end, 10);
   if (args->ptr == end)
      {
         return 0;
      }

   args->ptr = end;
   return 1;
}

int
args_next_mention (struct cmd_args_t *args, uint64_t *out)
{
   while (*args->ptr && isspace (*args->ptr))
      {
         args->ptr++;
      }

   if (strncmp (args->ptr, "<@", 2) != 0)
      {
         return 0;
      }

   const char *start = args->ptr + 2;
   if (*start == '!')
      {
         start++;
      }

   char *end;
   *out = strtoull (start, &end, 10);

   if (*end != '>')
      {
         return 0;
      }

   args->ptr = end + 1;
   return 1;
}

/**********/
/*  CMDS  */
/**********/

void
cmd_die (struct cbot_t *cbot, const struct discord_message *event,
         const char *cmd)
{
   struct discord_ret_message ret = { .sync = DISCORD_SYNC_FLAG };

   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = "Goodbye, master! X_X" },
       &ret);

   cbot_log("SHUTTING DOWN...");
   cbot_save_bank_users (cbot);
   curl_global_cleanup ();
   discord_shutdown (cbot->client);
}

void
cmd_help (struct cbot_t *cbot, const struct discord_message *event,
          const char *cmd_name)
{
   char final_msg[4096];
   char tmp_msg[512];

   bool is_master = (event->author->id == cbot->master_id);
   sprintf (final_msg,
            "List of commands for <@!%llu>:\n",
            event->author->id);

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
cmd_id (struct cbot_t *cbot, const struct discord_message *event,
        const char *cmd)
{
   struct cmd_args_t args;
   args_init (&args, event->content);
   char msg[1024];
   u64snowflake target_id;

   if (!args_next_mention (&args, &target_id)
       && !args_next_snowflake (&args, &target_id))
      {
         snprintf (msg, sizeof (msg), "Your user id is **%llu**\n",
                   event->author->id);
      }
   else
      {
         snprintf (msg, sizeof (msg), "<@!%llu>'s user id is **%llu**\n",
                   target_id, target_id);
      }

   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){
           .content = msg,
           .allowed_mentions
           = &(struct discord_allowed_mention){ .parse = 0 } },
       NULL);
}

void
cmd_rand (struct cbot_t *cbot, const struct discord_message *event,
          const char *cmd)
{
   char usage[128];
   snprintf (usage, sizeof (usage), "**[USAGE]**\n```\n%s <min> <max>\n```\n",
             cmd);

   struct cmd_args_t args;
   args_init (&args, event->content);
   uint64_t min;
   uint64_t max;

   if (!args_next_number (&args, &min) || !args_next_number (&args, &max))
      {
         goto print_usage;
      }

   if (min == max)
      {
         discord_create_message (cbot->client, event->channel_id,
                                 &(struct discord_create_message){
                                     .content = "What did you expect? (¬_¬)" },
                                 NULL);
         return;
      }

   if (min > max)
      {
         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){
                 .content = "`<min>` should be less than `<max>` >:(" },
             NULL);
         return;
      }

   if (min >= UINT32_MAX || max >= UINT32_MAX)
      {
         discord_create_message (cbot->client, event->channel_id,
                                 &(struct discord_create_message){
                                     .content = "Try with smaller number(s)" },
                                 NULL);
         return;
      }

   cbot_log ("rand -- min : %zu, max : %zu", min, max);

   uint32_t num = ystar_between (&cbot->seed, min, max);

   char response[512];
   const char *fmt = "## %u\n";
   snprintf (response, sizeof (response), fmt, num);

   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = response }, NULL);

   return;

print_usage:
   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = (char *)usage }, NULL);
}

void
cmd_seed (struct cbot_t *cbot, const struct discord_message *event,
          const char *cmd)
{
   char response[1024];
   snprintf (response, sizeof (response), "Current seed -- **%llu**",
             cbot->seed);

   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = response }, NULL);
}
