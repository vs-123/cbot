#include "cmds.h"

#include "discord.h"
#include "ystar.h"

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

/* FOR QSORT */
int
bank_users_cmp (const void *a, const void *b)
{
   const struct bank_user_t *user_a = (const struct bank_user_t *)a;
   const struct bank_user_t *user_b = (const struct bank_user_t *)b;

   if (user_b->balance > user_a->balance)
      {
         return 1;
      }
   if (user_b->balance < user_a->balance)
      {
         return -1;
      }
   return 0;
}

void
bank_unregistered_msg (struct cbot_t *cbot, const struct discord_message *event,
                       const char *cmd)
{
   char response[1024];
   sprintf (response, "Hey <@!%llu>, you're not registered!",
            event->author->id);
   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = response }, NULL);
}

void
not_enough_money (struct cbot_t *cbot, const struct discord_message *event,
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
bank_cmd_sub (struct cbot_t *cbot, const struct discord_message *event,
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

   struct bank_user_t *user = cbot_search_bank_user (cbot, snowflake);
   if (user == NULL)
      {
         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){
                 .content = "Master, user is not registered!!" },
             NULL);
         return;
      }

   user->balance = (amount >= user->balance) ? 0 : user->balance - amount;
   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = "Subtracted >:]" }, NULL);

   return;

print_usage:
   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = (char *)usage }, NULL);
}

void
bank_cmd_lb (struct cbot_t *cbot, const struct discord_message *event,
             const char *cmd)
{
   if (cbot->bank_users.count == 0)
      {
         discord_create_message (cbot->client, event->channel_id,
                                 &(struct discord_create_message){
                                     .content = "No users in economy! :c" },
                                 NULL);
         return;
      }

   qsort (cbot->bank_users.elems, cbot->bank_users.count,
          sizeof (struct bank_user_t), bank_users_cmp);

   char response[4096] = "__**=== BANK LEADERBOARD ===**__\n";
   char line[128];

   size_t top_limit
       = (cbot->bank_users.count > 10) ? 10 : cbot->bank_users.count;

   for (size_t i = 0; i < top_limit; i++)
      {
         struct bank_user_t *u = &cbot->bank_users.elems[i];

         snprintf (line, sizeof (line), "%zu. <@%llu> -- **%u.%02u**\n", i + 1,
                   (unsigned long long)u->user_id, u->balance / 100,
                   u->balance % 100);

         strcat (response, line);
      }

   struct discord_allowed_mention mentions = { .parse = false };

   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content          = response,
                                         .allowed_mentions = &mentions },
       NULL);
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
         not_enough_money (cbot, event, cmd);
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
           .allowed_mentions = &(struct discord_allowed_mention){ 0 },
       },
       NULL);
   return;

print_usage:
   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = (char *)usage }, NULL);
}

void
bank_cmd_boot (struct cbot_t *cbot, const struct discord_message *event,
               const char *cmd)
{
   char usage[128];
   snprintf (usage, sizeof (usage), "**[USAGE]**\n```\n%s <snowflake>\n```\n",
             cmd);

   struct cmd_args_t args;
   args_init (&args, event->content);
   u64snowflake target_id;

   if (!args_next_mention (&args, &target_id)
       && !args_next_snowflake (&args, &target_id))
      {
         goto print_usage;
      }

   struct bank_user_t *user = cbot_search_bank_user (cbot, target_id);
   if (user == NULL)
      {
         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){
                 .content = "Master, user is not registered!!" },
             NULL);
         return;
      }

   DREMOVE (cbot->bank_users, user);

   char response[128];
   snprintf (response, sizeof (response),
             "Master, <@!%llu> was booted successfully!", target_id);

   struct discord_allowed_mention mentions = { .parse = false };
   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content          = response,
                                         .allowed_mentions = &mentions },
       NULL);

   return;

print_usage:
   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = (char *)usage }, NULL);
}

void
bank_cmd_work (struct cbot_t *cbot, const struct discord_message *event,
               const char *cmd)
{
   struct bank_user_t *u = cbot_search_bank_user (cbot, event->author->id);

   if (!u)
      {
         bank_unregistered_msg (cbot, event, cmd);
         return;
      }

   time_t now = time (NULL);

   if (now < u->next_work_time)
      {
         char wait_msg[128];
         short wait_time = (short)(u->next_work_time - now);
         snprintf (wait_msg, sizeof (wait_msg),
                   "Hey <@!%llu>, you're too tired to work! Wait for **%hd** "
                   "more seconds. 🛑",
                   event->author->id, wait_time);

         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){ .content = wait_msg }, NULL);
         return;
      }

   uint32_t reward = ystar_between (&cbot->seed, 925, 3500);
   u->balance += reward;

   uint32_t cooldown = ystar_between (&cbot->seed, 3, 6);
   u->next_work_time = now + (time_t)cooldown;

   char response[256];
   snprintf (response, sizeof (response),
             "<@!%llu> did some work and earned **%u.%02u**! 💰",
             event->author->id, reward / 100, reward % 100);

   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = response }, NULL);
}

void
bank_cmd_gamble (struct cbot_t *cbot, const struct discord_message *event,
                 const char *cmd)
{
   char usage[128];
   snprintf (usage, sizeof (usage), "**[USAGE]**\n```\n%s <amount>\n```\n",
             cmd);

   const int MINIMUM_GAMBLING_AMOUNT = 2000;
   struct cmd_args_t args;
   args_init (&args, event->content);
   unsigned int amount;

   if (!args_next_amount (&args, &amount))
      {
         goto print_usage;
      }

   if (amount < MINIMUM_GAMBLING_AMOUNT)
      {
         char response[128];
         snprintf (
             response, sizeof (response),
             "Hey, <@!%llu>! You can't gamble less than **%u.%02u**. Are you "
             "scared? (¬‿¬)",
             event->author->id, MINIMUM_GAMBLING_AMOUNT / 100,
             MINIMUM_GAMBLING_AMOUNT % 100);

         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){ .content = response }, NULL);
         return;
      }

   struct bank_user_t *user = cbot_search_bank_user (cbot, event->author->id);
   if (!user)
      {
         bank_unregistered_msg (cbot, event, cmd);
      }

   if (user->balance < amount)
      {
         not_enough_money (cbot, event, cmd);
         return;
      }

   time_t now = time (NULL);
   if (now < user->next_gamble_time)
      {
         char wait_msg[128];
         long long wait_time = (long long)(user->next_gamble_time - now);
         snprintf (wait_msg, sizeof (wait_msg),
                   "Hey <@!%llu>, you're too tired to gamble! Wait for "
                   "**%lld** more seconds. 🛑",
                   event->author->id, wait_time);

         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){ .content = wait_msg }, NULL);
         return;
      }

   uint32_t cooldown      = ystar_between (&cbot->seed, 4, 8);
   user->next_gamble_time = now + (time_t)cooldown;

   int chance   = (int)ystar_between (&cbot->seed, 0, 2);
   bool has_won = (chance != 1); /* 2/3 chance of winning */
   int stolen_amount
       = (int)ystar_between (&cbot->seed, amount / 8 - 1, amount / 4 - 1);
   char response[256];

   struct bank_user_t *bot_user = cbot_search_bank_user (cbot, cbot->bot_id);

   if (has_won)
      {
         user->balance += amount - stolen_amount;
         snprintf (response, sizeof (response),
                   "<@!%llu> gambled and **WON %u.%02u** currency :O\n-# and "
                   "I stole %u.%02u from it hehe~",
                   event->author->id, amount / 100, amount % 100,
                   stolen_amount / 100, stolen_amount % 100);

         /* add the stolen amount to bot's own bank account if it has one */
         if (bot_user)
            {
               bot_user->balance += stolen_amount;
            }
      }
   else
      {
         user->balance -= amount;
         snprintf (response, sizeof (response),
                   "<@!%llu> gambled and **LOST %u.%02u** currency D:\n-# "
                   "It's all mine now >:]",
                   event->author->id, amount / 100, amount % 100);

         /* add the lost amount to bot's own bank account if it has one */
         if (bot_user)
            {
               bot_user->balance += amount;
            }
      }

   discord_create_message (cbot->client, event->channel_id,
                           &(struct discord_create_message){
                               .content = response,
                           },
                           NULL);
   return;

print_usage:
   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = (char *)usage }, NULL);
}
