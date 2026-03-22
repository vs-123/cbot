#include "cmds.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "discord.h"

#include "util.h"

static void skip_ws (const char **s);
static double parse_expr (const char **s);
static double parse_factor (const char **s);
static double parse_pow (const char **s);
static double parse_term (const char **s);
static double parse_expr (const char **s);
static double parse_expr (const char **s);

static void
skip_ws (const char **s)
{
   while (isspace (**s))
      {
         (*s)++;
      }
}

static double
parse_factor (const char **s)
{
   skip_ws (s);
   if (**s == '(')
      {
         (*s)++; // skip '('
         double v = parse_expr (s);
         skip_ws (s);
         if (**s == ')')
            {
               (*s)++; // skip ')'
            }
         return v;
      }
   if (strncmp (*s, "sqrt", 4) == 0)
      {
         *s += 4;
         return sqrt (parse_factor (s));
      }

   char *end;
   double v = strtod (*s, &end);
   *s       = end;

   skip_ws (s);
   if (**s == '!')
      {
         (*s)++;
         return tgamma (v + 1.0);
         /* because n! = tgamma(n + 1) */
      }
   return v;
}

static double
parse_pow (const char **s)
{
   double v = parse_factor (s);
   skip_ws (s);
   if (**s == '^')
      {
         (*s)++;
         v = pow (v, parse_pow (s));
      }
   return v;
}

static double
parse_term (const char **s)
{
   double v = parse_pow (s);
   while (1)
      {
         skip_ws (s);
         if (**s == '*')
            {
               (*s)++;
               v *= parse_pow (s);
            }
         else if (**s == '/')
            {
               (*s)++;
               v /= parse_pow (s);
            }
         else
            {
               break;
            }
      }
   return v;
}

static double
parse_expr (const char **s)
{
   double v = parse_term (s);
   while (1)
      {
         skip_ws (s);
         if (**s == '+')
            {
               (*s)++;
               v += parse_term (s);
            }
         else if (**s == '-')
            {
               (*s)++;
               v -= parse_term (s);
            }
         else
            {
               break;
            }
      }
   return v;
}

void
cmd_calc (struct cbot_t *cbot, const struct discord_message *event,
          const char *cmd)
{
   const char *expr = event->content + cbot->prefix_len + strlen(cmd);
   cbot_log("expr -- %s", expr);

   double result = parse_expr (&expr);

   char response[128];
   snprintf (response, sizeof (response), "<@!%llu>, I think it's **%.4f** :D",
             event->author->id, result);

   discord_create_message (
       cbot->client, event->channel_id,
       &(struct discord_create_message){ .content = response }, NULL);
}
