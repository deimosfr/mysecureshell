/*
MySecureShell permit to add restriction to modified sftp-server
when using MySecureShell as shell.
Copyright (C) 2007 Sebastien Tardif

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation (version 2)

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "config.h"
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ip.h"
#include "parsing.h"
#include "user.h"
#include "SftpServer/Access.h"

static char	*user_name = NULL;
static int	restrictions = REST_ALL;

int		init_user_info()
{
  struct passwd	*info;

  InitAccess();
  if ((info = getpwuid(getuid())))
    {
      if ((user_name = strdup(info->pw_name)) == NULL)
	return (0);
      hash_set("User", (void *)strdup(info->pw_name));
      hash_set("Home", (void *)strdup(info->pw_dir));
      return (1);
    }
  return (0);
}

void	free_user_info()
{
  free(user_name);
}

int	is_for_user(const char *user, int verbose)
{
  if (user == NULL)
    return (0);
  if (strcmp(user, TAG_ALL) == 0 && restrictions != REST_USER)
    {
      if (verbose >= 2) (void )printf("--- Apply restrictions for all users ---\n");
      return (1);
    }
  if (strcmp(user, user_name) == 0)
    {
      if (verbose >= 2) (void )printf("--- Apply restrictions for user '%s' ---\n", user);
      restrictions = REST_USER;
      return (1);
    }
  return (0);
}

int		is_for_group(const char *group, int verbose)
{
  struct group	*grp;

  if (group == NULL || restrictions == REST_USER)
    return (0);
  if (strcmp(group, TAG_ALL) == 0 && restrictions == REST_ALL)
    {
      if (verbose >= 2) (void )printf("--- Apply restrictions for all groups ---\n");
      return (1);
    }
  if (restrictions <= REST_GROUP && (grp = getgrnam(group)) != NULL)
    if (UserIsInThisGroup(grp->gr_gid) == 1)
      {
	restrictions = REST_GROUP;
	if (verbose >= 2) (void )printf("--- Apply restrictions for group '%s' ---\n", group);
	return (1);
      }
  return (0);
}

int	is_for_virtualhost(const char *host, int port, int verbose)
{
  char	*current_host;
  int	current_port;
  
  current_host = (char *)hash_get("VIRTUALHOST_IP");
  current_port = hash_get_int("VIRTUALHOST_PORT");	
  if (current_host != NULL && (strcmp(host, current_host) == 0
			       || strcmp(current_host, TAG_ALL) == 0))
    if (current_port == 0 || port == current_port)
      {
	if (verbose >= 2)
	  (void )printf("--- Apply restriction for virtualhost '%s:%i' ---\n", current_host, current_port);
	return (1);
      }
  return (0);
}

int	is_for_rangeip(const char *range, int verbose)
{
  char	*bip, *ip;
  int	pos, size;

  if (range == NULL || restrictions == REST_USER)
    return (0);
  size = (int )((unsigned char )range[8]);
  ip = get_ip(0); //don't resolv dns
  bip = parse_range_ip(ip);
  pos = 0;
  while (size >= 8)
    {
      if (range[pos] <= bip[pos] && bip[pos] <= range[pos + 4])
	{
	  pos++;
	  size -= 8;
	}
      else
	goto error_is_for_rangeip;
    }
  if (size > 0)
    {
      bip[pos] = (unsigned char )bip[pos] >> (8 - size);
      bip[pos] = (unsigned char )bip[pos] << (8 - size);
      if (range[pos] < bip[pos] || bip[pos] > range[pos + 4])
	goto error_is_for_rangeip;
    }
  if (verbose >= 2)
    (void )printf("--- Apply restrictions for ip range '%i.%i.%i.%i-%i.%i.%i.%i/%i' ---\n",
		  (unsigned char )range[0], (unsigned char )range[1], (unsigned char )range[2],
		  (unsigned char )range[3], (unsigned char )range[4], (unsigned char )range[5],
		  (unsigned char )range[6], (unsigned char )range[7], (unsigned char )range[8]);
  free(bip);
  free(ip);
  return (1);

 error_is_for_rangeip:
  free(bip);
  free(ip);
  return (0);
}
