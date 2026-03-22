#include <stdio.h>
#include <time.h>

#include "discord.h"

#include "bank.h"
#include "cbot.h"
#include "cmds.h"

#define YSTAR_IMPLEMENTATION
#include "ystar.h"

struct cbot_t cbot = {
   .master_id          = 386862660483809280,

   .prefix             = "cb.",
   .prefix_len         = 3,

   .bank_users         = (struct bank_users_t){ 0 },
   .bank_save_filename = "bank_users.txt",
   .cmds       = (struct cmd_t[]){
      { "b.add", "Add amount to a bank user", &bank_cmd_add, true },
      { "b.bal", "View your or someone else's balance", &bank_cmd_bal, false },
      { "b.lb", "View leaderboard of top 10 bank users according to bank balance", &bank_cmd_lb, true },
      { "b.oreg", "Forcibly register someone into my banking system", &bank_cmd_oreg, true },
      { "b.reg", "Register yourself into my banking system", &bank_cmd_reg, false },
      { "b.sub", "Subtract amount from a bank user", &bank_cmd_sub, true },
      { "b.transfer", "Transfer your money to someone else", &bank_cmd_transfer, false },
      { "b.work", "Work for money", &bank_cmd_work, false },
      { "bf", "Run brainfuck code", &cmd_bf, false },
      { "die", "Shut me down", &cmd_die, true },
      { "help", "View a list of all commands you can run", &cmd_help, false },
      { "id", "View your or someone else's discord user ID", &cmd_id, false },
      { "rand", "Generate a random number between two given numbers", &cmd_rand, false },
      { "seed", "View the current seed of my PRNG", &cmd_seed, false },
      NULL_CMD,
   },
};

void
on_ready (struct discord *client, const struct discord_ready *event)
{
   cbot_on_ready (&cbot, client, event);
}

void
on_message (struct discord *client, const struct discord_message *event)
{
   cbot_on_message (&cbot, event);
}

void
on_interaction (struct discord *client, const struct discord_interaction *event)
{
   cbot_on_interaction (&cbot, event);
}

int
main (void)
{
   struct discord *client = discord_config_init ("config.json");
   cbot.client            = client;
   cbot.seed              = time (NULL);

   discord_add_intents (client, DISCORD_GATEWAY_MESSAGE_CONTENT);
   discord_set_on_ready (client, &on_ready);
   discord_set_on_message_create (client, &on_message);
   discord_set_on_interaction_create (client, &on_interaction);

   discord_run (client);

   if (client != NULL)
      {
         discord_cleanup (client);
      }

   return 0;
}
