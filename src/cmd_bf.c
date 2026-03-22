#include "cbot.h"

#include <stdlib.h>
#include <string.h>

#include "discord.h"

#include "cmds.h"
#include "util.h"

#define MEMORY_TAPE      30000
#define MAX_INSTRUCTIONS 100000
#define BF_OUTPUT_SIZE   4096

typedef enum
{
   BF_ERR_BAD_LOOP,
   BF_ERR_TIMEOUT,
   BF_ERR_MEM,
   BF_ERR_OUT_OF_BOUNDS,
   BF_ERR_NONE,
} bf_error_t;

bf_error_t
run_bf (const char *bf, char *output, size_t out_max)
{
   size_t len = strlen (bf);
   if (len == 0)
      {
         return BF_ERR_NONE;
      }

   int *jump_table = (int *)malloc (sizeof (int) * len);
   int *stack      = (int *)malloc (sizeof (int) * len);
   if (!jump_table || !stack)
      {
         free (jump_table);
         free (stack);
         return BF_ERR_MEM;
      }

   int stack_ptr = 0;
   for (int i = 0; i < (int)len; i++)
      {
         jump_table[i] = -1;
         if (bf[i] == '[')
            {
               stack[stack_ptr++] = i;
            }
         else if (bf[i] == ']')
            {
               if (stack_ptr == 0)
                  {
                     free (stack);
                     free (jump_table);
                     return BF_ERR_BAD_LOOP;
                  }
               int open_idx         = stack[--stack_ptr];
               jump_table[open_idx] = i;
               jump_table[i]        = open_idx;
            }
      }
   if (stack_ptr > 0)
      {
         free (stack);
         free (jump_table);
         return BF_ERR_BAD_LOOP;
      }
   free (stack);

   unsigned char tape[MEMORY_TAPE] = { 0 };
   unsigned char *ptr              = tape;
   size_t out_idx                  = 0;
   unsigned int inst_count         = 0;

   for (int i = 0; i < (int)len; i++)
      {
         if (++inst_count > MAX_INSTRUCTIONS)
            {
               free (jump_table);
               return BF_ERR_TIMEOUT;
            }

         switch (bf[i])
            {
            case '>':
               if (ptr >= tape + MEMORY_TAPE - 1)
                  {
                     ptr = tape;
                  }
               else
                  {
                     ptr++;
                  }
               break;
            case '<':
               if (ptr <= tape)
                  {
                     ptr = tape + MEMORY_TAPE - 1;
                  }
               else
                  {
                     ptr--;
                  }
               break;
            case '+':
               (*ptr)++;
               break;
            case '-':
               (*ptr)--;
               break;
            case '.':
               if (out_idx < out_max - 1)
                  {
                     output[out_idx++] = (char)*ptr;
                  }
               else
                  {
                     free (jump_table);
                     return BF_ERR_OUT_OF_BOUNDS;
                  }
               break;
            case '[':
               if (*ptr == 0)
                  {
                     i = jump_table[i];
                  }
               break;
            case ']':
               if (*ptr != 0)
                  {
                     i = jump_table[i];
                  }
               break;
            default:
               break;
            }
      }

   output[out_idx] = '\0';
   free (jump_table);
   return BF_ERR_NONE;
}

void
cmd_bf (struct cbot_t *cbot, const struct discord_message *event,
        const char *cmd)
{
   const char *bf_code = event->content;
   char response[5120] = { 0 };

   while (*bf_code != '\0' && *bf_code != ' ' && *bf_code != '\n')
      {
         bf_code++;
      }

   if (*bf_code == '\0')
      {
         snprintf (response, sizeof (response), "No code? :(");
         discord_create_message (
             cbot->client, event->channel_id,
             &(struct discord_create_message){
                 .content = response,
                 .allowed_mentions
                 = &(struct discord_allowed_mention){ .parse = 0 } },
             NULL);
         return;
      }

   char bf_output[BF_OUTPUT_SIZE] = { 0 };
   bf_error_t err                 = run_bf (bf_code, bf_output, BF_OUTPUT_SIZE);

   switch (err)
      {
      case BF_ERR_NONE:
         if (bf_output[0] != '\0')
            {
               snprintf (response, sizeof (response),
                         "**[OUTPUT]**\n```\n%s\n```", bf_output);
            }
         else
            {
               snprintf (response, sizeof (response), "There was no output :(");
            }
         break;
      case BF_ERR_BAD_LOOP:
         snprintf (response, sizeof (response), "**[ERROR]** Bad loop >:3");
         break;
      case BF_ERR_TIMEOUT:
         snprintf (response, sizeof (response),
                   "**[ERROR]**\n Took too much time (¬_¬)");
         break;
      case BF_ERR_MEM:
         snprintf (response, sizeof (response),
                   "**[ERROR]**\n You think I have infinite memory?");
         break;
      case BF_ERR_OUT_OF_BOUNDS:
         snprintf (response, sizeof (response),
                   "**[ERROR]**\n Nice try, but that's not gonna work (¬_¬)");

         break;
      }

   cbot_log ("BF CODE -- %s", bf_code);

   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){
           .content = response,
           .allowed_mentions
           = &(struct discord_allowed_mention){ .parse = 0 } },
       NULL);
}
