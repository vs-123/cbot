#include "bank.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "discord.h"

#include "cbot.h"
#include "util.h"

int
cbot_save_bank_users (const struct cbot_t *cbot)
{
   FILE *fp = fopen (cbot->bank_save_filename, "w");
   if (!fp)
      {
         return -1;
      }

   struct bank_user_t *bank_user = NULL;
   for (size_t i = 0; i < cbot->bank_users.count; ++i)
      {
         bank_user = &cbot->bank_users.elems[i];
         fprintf (fp, "%llu,%u\n", bank_user->user_id, bank_user->balance);
      }

   fclose (fp);
   return 0;
}

int
cbot_load_bank_users (struct cbot_t *cbot)
{
   FILE *fp = fopen (cbot->bank_save_filename, "r");
   assert (fp);
   if (!fp)
      {
         return -1;
      }

   u64snowflake user_id;
   unsigned int balance;

   cbot->bank_users.count = 0;

   while (fscanf (fp, "%llu,%u", &user_id, &balance) == 2)
      {
         DAPPEND (cbot->bank_users, ((struct bank_user_t){
                                        .user_id = user_id,
                                        .balance = balance,
                                    }));
      }

   fclose (fp);
   return 0;
}
