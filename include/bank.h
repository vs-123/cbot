#ifndef BANK_H
#define BANK_H

#include "discord.h"

struct cbot_t;  /* including cbot.h will cause circular dependency */

struct bank_user_t;
struct bank_users_t;

struct bank_user_t
{
   u64snowflake user_id;
   unsigned int balance;
};

#ifndef NULL_BANK_USER
#define NULL_BANK_USER (struct bank_user_t){ 0 }
#endif

struct bank_users_t
{
   struct bank_user_t *elems;
   size_t capacity;
   size_t count;
};

int cbot_save_bank_users (const struct cbot_t *cbot);
int cbot_load_bank_users (struct cbot_t *cbot);

#endif /* BANK_H */
