#include "cmds.h"

#include "discord.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

/*************/
/*  HELPERS  */
/*************/

int
args_next_amount (struct cmd_args_t *args, unsigned int *out)
{
   while (*args->ptr && isspace ((unsigned char)*args->ptr))
      {
         args->ptr++;
      }
   if (!*args->ptr)
      {
         return 0;
      }

   const char *start = args->ptr;
   char buffer[32];
   int i = 0;

   while (*args->ptr && !isspace ((unsigned char)*args->ptr) && i < 31)
      {
         if (!isdigit ((unsigned char)*args->ptr) && *args->ptr != '.')
            {
               break;
            }
         buffer[i++] = *args->ptr++;
      }
   buffer[i] = '\0';
   if (i == 0)
      {
         return 0;
      }

   char *dot = strchr (buffer, '.');
   if (!dot)
      {
         unsigned int val = (unsigned int)strtoul (buffer, NULL, 10);
         *out             = val * 100;
      }
   else
      {
         *dot            = '\0';
         char *int_part  = buffer;
         char *frac_part = dot + 1;

         unsigned int dollars = (unsigned int)strtoul (int_part, NULL, 10);
         unsigned int cents   = 0;

         size_t frac_len = strlen (frac_part);
         if (frac_len > 0)
            {
               char padded_frac[3] = "00";
               strncpy (padded_frac, frac_part, 2);
               cents = (unsigned int)strtoul (padded_frac, NULL, 10);
               if (frac_len == 1)
                  {
                     cents *= 10;
                  }
            }
         *out = (dollars * 100) + cents;
      }

   return 1;
}

void
bank_unregistered_msg (struct cbot_t *cbot,
                           const struct discord_message *event, const char *cmd)
{
   char response[1024];
   sprintf (response, "Hey <@!%llu>, you're not registered!",
            event->author->id);
   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = response }, NULL);
}

void
bank_not_enough_money (struct cbot_t *cbot, const struct discord_message *event,
                      const char *cmd)
{
   discord_create_message (cbot->client, event->channel_id,
                           &(struct discord_create_message){
                               .content = "You're too poor for that :3" },
                           NULL);
}

struct bank_user_t *
cbot_search_bank_user (struct cbot_t *cbot, u64snowflake user_id)
{
   for (size_t i = 0; i < cbot->bank_users.count; i++)
      {
         if (cbot->bank_users.elems[i].user_id == user_id)
            {
               return &cbot->bank_users.elems[i];
            }
      }
   return NULL;
}

/***************/
/*  BANK CMDS  */
/***************/

void
bank_cmd_add (struct cbot_t *cbot, const struct discord_message *event,
              const char *cmd)
{
   char usage[128];
   snprintf (usage, sizeof (usage),
             "**[USAGE]**\n```\n%s <snowflake> <amount>\n```\n", cmd);

   struct cmd_args_t args;
   args_init (&args, event->content);
   u64snowflake snowflake;
   unsigned int amount;

   if (!args_next_mention (&args, &snowflake)
       && !args_next_snowflake (&args, &snowflake))
      {
         goto print_usage;
      }

   if (!args_next_amount (&args, &amount))
      {
         goto print_usage;
      }

   struct bank_user_t *bank_user = cbot_search_bank_user (cbot, snowflake);

   if (bank_user == NULL)
      {
         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){
                 .content = "Master, that user is not registered!" },
             NULL);

         return;
      }

   bank_user->balance += amount;
   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = "Added!" }, NULL);

   return;
print_usage:
   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = (char *)usage }, NULL);
}

void
bank_cmd_bal (struct cbot_t *cbot, const struct discord_message *event,
              const char *cmd)
{
   struct cmd_args_t args;
   args_init (&args, event->content);
   u64snowflake snowflake;

   if (!args_next_mention (&args, &snowflake)
       && !args_next_snowflake (&args, &snowflake))
      {
         snowflake = event->author->id;
      }

   struct bank_user_t *bank_user = cbot_search_bank_user (cbot, snowflake);

   if (bank_user == NULL)
      {
         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){
                 .content = "That user is NOT registered!" },
             NULL);
         return;
      }

   char response[4096];
   sprintf (response, "<@%llu>'s balance is **%u.%02u** :3", snowflake,
            (bank_user->balance / 100), (bank_user->balance % 100));

   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = response }, NULL);
}

void
bank_cmd_oreg (struct cbot_t *cbot, const struct discord_message *event,
               const char *cmd)
{
   char usage[128];
   snprintf (usage, sizeof (usage), "**[USAGE]**\n```\n%s <snowflake>\n```\n",
             cmd);

   struct cmd_args_t args;
   args_init (&args, event->content);
   u64snowflake snowflake;

   if (!args_next_mention (&args, &snowflake)
       && !args_next_snowflake (&args, &snowflake))
      {
         goto print_usage;
      }

   struct bank_user_t *bank_user = cbot_search_bank_user (cbot, snowflake);
   if (bank_user != NULL)
      {
         char response[1024];
         sprintf (response, "Master, %llu is already registered!", snowflake);
         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){ .content = response }, NULL);
         return;
      }

   DAPPEND (cbot->bank_users,
            ((struct bank_user_t){ .user_id = snowflake, .balance = 0 }));
   char response[1024];
   sprintf (response, "%llu has been (forcibly) registered successfully!",
            snowflake);
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
bank_cmd_reg (struct cbot_t *cbot, const struct discord_message *event,
              const char *cmd)
{
   struct bank_user_t *bank_user
       = cbot_search_bank_user (cbot, event->author->id);

   if (bank_user != NULL)
      {
         char response[1024];
         sprintf (response, "Bruh <@!%llu>, you're already registered!",
                  event->author->id);
         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){ .content = response }, NULL);
         return;
      }

   DAPPEND (
       cbot->bank_users,
       ((struct bank_user_t){ .user_id = event->author->id, .balance = 0 }));

   char response[1024];
   sprintf (response, "You were registered successfully, <@!%llu>!",
            event->author->id);

   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = response }, NULL);
}

void
bank_cmd_transfer (struct cbot_t *cbot, const struct discord_message *event,
                   const char *cmd)
{
   char usage[128];
   snprintf (usage, sizeof (usage),
             "**[USAGE]**\n```\n%s <snowflake> <amount>\n```\n", cmd);

   struct cmd_args_t args;
   args_init (&args, event->content);
   u64snowflake target_id;
   unsigned int amount;

   if (!args_next_mention (&args, &target_id)
       && !args_next_snowflake (&args, &target_id))
      {
         goto print_usage;
      }

   if (!args_next_amount (&args, &amount) || amount == 0)
      {
         goto print_usage;
      }

   struct bank_user_t *sender = cbot_search_bank_user (cbot, event->author->id);
   struct bank_user_t *receiver = cbot_search_bank_user (cbot, target_id);

   if (!sender)
      {
         bank_unregistered_msg (cbot, event, cmd);
         return;
      }

   if (!receiver)
      {
         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){
                 .content = "Are you sure they're registered?" },
             NULL);
         return;
      }

   if (sender->balance < amount)
      {
         bank_not_enough_money (cbot, event, cmd);
         return;
      }

   sender->balance -= amount;
   receiver->balance += amount;

   char response[256];
   snprintf (response, sizeof (response), "Transferred %u.%02u to <@%llu> ^_^",
             amount / 100, amount % 100, (unsigned long long)target_id);

   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){
           .content          = response,
           .allowed_mentions = &(struct discord_allowed_mention){ 0 } },
       NULL);
   return;

print_usage:
   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = (char *)usage }, NULL);
}
